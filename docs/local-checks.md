# Pre-PR local checks

CI runs every lane in parallel and returns in minutes. Running the same checks
locally before pushing trades human time for a reviewer-confidence margin we
mostly don't need. This document defines what to run locally and when, so the
wait before a push stays short and CI catches the rest.

## Tiers

| Tier | When | What | Wall-clock |
|---|---|---|---|
| **A** - fast feedback | Every commit on the branch | `cmake --build --preset debug --target junit` for whatever preset matches the diff (gcc / clang / freertos-host) | ~30-60 s |
| **B** - pre-push | First push to the branch and any push that changes production source | A + format reflowed includes + `misra_renumber.py`, plus `check_spdx_headers.py` when a file was added and the manifests when a platform gained a source | ~3-4 min |
| **CI** - everything else | After push | `tidy` (except when adding a new pack), `sanitize`, `coverage`, Windows, BDD, integration, FreeRTOS host/cross, advisory IWYU, MISRA on cpputest | runs in parallel |

IWYU is advisory. The lanes still run on every PR and
the report is uploaded as an artifact, but findings no longer fail the
build. Sweep the IWYU artifact when you do a release cleanup; do not
treat it as a per-PR blocker.

Format-on-save in the editor handles formatting per-edit, so no separate
`analyze-format` step locally. If you skip an editor with format-on-save,
add a `clang-format -i` sweep over touched files to Tier A.

## Path-gating Tier B

Tier B does MISRA-line-drift cleanup, so scope it to what changed:

- Touched only `Tests/`, `Bdd/Targets/`, `docs/`, `cmake/`, or `*.md`:
  skip Tier B entirely. Push and let CI run.
- Touched any `Core/Source/`, `Platform/*/Source/`, or public-header file:
  run `clang-format -i` over touched files and
  `scripts/misra_renumber.py --apply` to update the suppressions.
- Added any file under `Core/` or `Platform/`: run
  `python3 scripts/check_spdx_headers.py`. It is a sub-second file scan, so it
  costs nothing to run on every push if you would rather not think about it.
- Added a whole new pack under `Platform/`: run clang-tidy over it, below. This
  is the one case where the tidy lanes are worth pre-empting rather than leaving
  to CI.
- Added or removed a `.c` under `Core/Source/` or `Platform/*/Source/`, or
  changed a pack's `target_sources`: regenerate the manifests, below.
  `docs/generated/` is checked in and `verify-manifest` both diffs it and
  compares it against the tree, so either kind of change makes that lane red on
  its own.

## Running Tier B

### MISRA - fix line-number drift

When edits shift production lines, `misra_suppressions.txt` entries go
stale. Fix in one step:

```bash
# In any container that has cppcheck (all of them do):
scripts/misra_renumber.py            # show proposed renumbers
scripts/misra_renumber.py --apply    # write back updated suppressions
```

The script bails on genuine new findings (mismatched counts per
rule+file); those need manual review. See the script's docstring.

### Manifests - regenerate after changing the sources

`docs/generated/<Platform>-manifest.txt` lists what each pack compiles, and is
what a non-CMake integrator builds from. The `verify-manifest` lane checks it
two ways, and both have to pass:

1. It regenerates every manifest and fails on any difference. A file added to
   `target_sources` makes one stale this way.
2. It runs `scripts/check_manifest.py` over each one, comparing the file lists
   against the source directories on disk rather than against the build targets.
   A `.c` sitting in `Platform/<Pack>/Source/` that no target lists is invisible
   to step 1 - regenerating produces no diff - and is caught here.

Run CI's own loop rather than editing the files. It is configure-only and takes
about a minute:

```bash
docker compose -f .devcontainer/docker-compose.yml run --rm freertos-host bash -c '
scrubbed() { env -u LWIP_PATH -u FREERTOS_KERNEL_PATH -u MBEDTLS_DIR \
    -u FATFS_PATH -u FREERTOS_PLUS_FAT_PATH -u FREERTOS_PLUS_TCP_PATH "$@"; }
scrubbed cmake -S . -B build/manifest-core -DSOLIDSYSLOG_BUILD_TESTING=OFF \
  -DSOLIDSYSLOG_MANIFEST_SCOPE=core \
  -DSOLIDSYSLOG_MANIFEST_OUTPUT="$(pwd)/docs/generated/core-manifest.txt"
for platform in $(python3 scripts/check_manifest.py --list-platforms); do
  [ "$platform" = "Windows" ] && continue
  scrubbed cmake -S . -B "build/manifest-$platform" -DSOLIDSYSLOG_BUILD_TESTING=OFF \
    -DSOLIDSYSLOG_MANIFEST_SCOPE=platform -DSOLIDSYSLOG_PLATFORMS="$platform" \
    -DSOLIDSYSLOG_MANIFEST_PLATFORMS="$platform" \
    -DSOLIDSYSLOG_MANIFEST_OUTPUT="$(pwd)/docs/generated/$platform-manifest.txt"
done
cmake -S . -B build/manifest -DSOLIDSYSLOG_BUILD_TESTING=OFF \
  -DSOLIDSYSLOG_PLATFORMS="LwipRaw;FreeRtos;MbedTls;FatFs;StdAtomic" \
  -DSOLIDSYSLOG_MANIFEST_PLATFORMS="LwipRaw;MbedTls;FreeRtos;FatFs;StdAtomic" \
  -DSOLIDSYSLOG_MANIFEST_OUTPUT="$(pwd)/docs/generated/beta-stack-manifest.txt"
for manifest in docs/generated/*-manifest.txt; do
  python3 scripts/check_manifest.py "$manifest" || exit 1
done'
git diff --stat docs/generated/
```

Three things the command is doing deliberately. The environment scrub makes a
pack appear in a manifest because it was named rather than because the image
happens to carry its upstream tree. `Windows` is skipped because it is
probe-kind and cannot be selected on Linux - `build-windows-msvc` writes that
fragment. And the last command is not regeneration but assertion: it is step 2
above, which a clean `git diff` does not imply.

The beta-stack manifest covers `Core/Source` plus the platforms named in it, so
a source added to any of those drifts it as well as its own. It is regenerated
unconditionally above rather than left to judgement, because deciding it does
not apply is how it goes stale.

Read the commands from the `verify-manifest` job in `.github/workflows/ci.yml`
rather than from here if the two ever disagree - the workflow is what runs.

### clang-tidy - only when adding a new pack

`tidy` is otherwise CI's job, and the row below still says so. A new pack is the
exception: it is a new directory of new files, and clang-tidy has more to say
about those than about an edit to code it has already accepted. S36.01 pushed a
new pack, went red on both `analyze-tidy-freertos-*` lanes, and cost a round
trip for three findings that a local run would have shown in two minutes.

```bash
# In an image that has the upstream trees the pack needs - freertos-host carries
# all of them. A dedicated build dir keeps the tidy cache out of build/debug.
docker compose -f .devcontainer/docker-compose.yml run --rm -T freertos-host bash -c '
  cmake -S . -B build/tidy-newpack -DCMAKE_BUILD_TYPE=Debug \
        -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++ -DENABLE_CLANG_TIDY=ON
  cmake --build build/tidy-newpack --target junit'
```

Two findings recur for a new pack, and both have an established answer rather
than a suppression:

- **A public header naming an upstream struct type** trips
  `readability-identifier-naming`, because third-party tags follow nobody's
  house style. Add a pack-level `.clang-tidy` extending `StructIgnoredRegexp`
  to that upstream's namespace, as `Platform/MbedTls/.clang-tidy` and
  `Platform/LittleFs/.clang-tidy` do. Their comments explain the catch: option
  values do not merge across the hierarchy, so the root whitelist has to be
  restated alongside the addition.
- **A test fixture subscripting an array with a non-constant index** trips
  `cppcoreguidelines-pro-bounds-constant-array-index`. Walk the array with a
  pointer, or a range-for, rather than indexing it.

### IWYU (optional, advisory)

If you want a local look before push, the lane is still wired:

```bash
docker compose -f .devcontainer/docker-compose.yml run --rm clang \
  bash -c 'cmake --preset iwyu && cmake --build --preset iwyu --target iwyu'
```

For FreeRTOS / Plus-TCP / lwIP / MbedTLS / FatFs trees, use `freertos-host`
with the clang-19 overrides instead:

```bash
docker compose -f .devcontainer/docker-compose.yml run --rm freertos-host \
  bash -c 'cmake --preset iwyu \
    -DCMAKE_C_COMPILER=clang-19 -DCMAKE_CXX_COMPILER=clang++-19 \
    && cmake --build --preset iwyu --target iwyu'
```

CI runs all three IWYU lanes advisory; findings appear in the `iwyu-report`,
`iwyu-report-freertos-plustcp` and `iwyu-report-freertos-lwip` artifacts and
don't block the build.

### Licence headers

Every file under `Core/` and `Platform/` opens with the SPDX header, and none
of them may claim anyone else's copyright:

```bash
python3 scripts/check_spdx_headers.py
```

Both the copyright line and the licence expression are read out of `LICENSE.md`
at run time, so there is nothing to keep in step by hand - the check fails if
the tree and the licence disagree.

The second half is a tripwire rather than a style rule. `Core/` and `Platform/`
contain no third-party code, and that invariant is what makes it safe to stamp
our copyright across every file in them. If it fires, the question is whether
the file belongs in the shipped library at all - there is deliberately no
allowlist.

## Markdown

Markdown is linted in CI by the `analyze-markdown` lane (markdownlint-cli2
v0.22.1), wired into `summary`. The rules live in `.markdownlint-cli2.jsonc`,
our conventions (line-length and table-column-style off, fenced-code language
required); `CHANGELOG.md` and the verbatim licence texts under `LICENSES/`
are ignored.

If you touch any `.md`, lint the files you changed before pushing. Same pinned
engine as CI and CodeRabbit, via Docker (no Node needed):

```bash
# Collect the .md files changed on this branch. git's exit status is captured
# rather than consumed by a process substitution: if it fails — an unfetched
# origin/main, say — the array would silently be empty, both commands below
# would skip, and the check would report success having linted nothing.
# --diff-filter=ACMRT drops deletions, which would otherwise reach the linter as
# missing files, and the array keeps paths containing spaces intact:
diff_output=$(git diff --name-only --diff-filter=ACMRT origin/main...HEAD -- '*.md') \
  || echo 'Cannot list changed Markdown — is origin/main fetched?' >&2

changed=()
[[ -n "$diff_output" ]] && mapfile -t changed <<<"$diff_output"

# --no-globs is required: the "globs" entry in .markdownlint-cli2.jsonc is
# combined with any paths given on the command line, so without it the whole
# tree is linted whatever you pass. The "ignores" entry still applies.
if ((${#changed[@]})); then
  docker run --rm -v "$PWD:/workdir" \
    davidanson/markdownlint-cli2:v0.22.1 --no-globs "${changed[@]}"
fi

# Auto-fix the mechanical rules (blank lines, trailing space, list style, ...):
if ((${#changed[@]})); then
  docker run --rm -v "$PWD:/workdir" \
    davidanson/markdownlint-cli2:v0.22.1 --fix --no-globs "${changed[@]}"
fi
```

CI lints the whole tree, so a rule change or a config edit can surface findings
in files this branch did not touch. Run the no-argument form when you change
`.markdownlint-cli2.jsonc` itself.

With Node available, `npx markdownlint-cli2@0.22.1` is equivalent. Fenced-code
languages (MD040) and a few structural rules are not auto-fixable; tag or
adjust those by hand.

## What CI runs and you should not run locally

- `sanitize`, `coverage`: minutes each, both gated by CI
- `tidy`: gated by CI too, with one exception - a PR adding a new
  `Platform/` pack, where it is worth running first (above)
- `c99`: the `build-linux-c99` lane builds the library at the C99 language
  standard on every PR. If it fails, either fix the construct or, if it
  genuinely belongs to a C11-only component, gate that component the way
  `Platform/StdAtomic` is gated. See [builds.md](builds.md#c99-portability--c99)
- Windows MSVC + BDD + integration: depend on tools you may not have
- BDD-linux-syslog-ng, BDD-windows-otel, BDD-freertos-qemu: heavy
  multi-container stacks

If CI surfaces a finding you missed locally, fix in another commit on the
same branch, cheaper than running every CI lane on every push.
