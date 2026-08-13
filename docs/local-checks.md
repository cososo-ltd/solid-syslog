# Pre-PR local checks

The full CI suite takes ~10–15 minutes wall-clock. Running every check
locally before pushing trades human time for one extra reviewer-confidence
margin we mostly don't need. This document defines what to run locally and
when, so the wait stays under a few minutes and CI catches the rest.

## Tiers

| Tier | When | What | Wall-clock |
|---|---|---|---|
| **A** — fast feedback | Every commit on the branch | `cmake --build --preset debug --target junit` for whatever preset matches the diff (gcc / clang / freertos-host) | ~30–60 s |
| **B** — pre-push | First push to the branch and any push that changes production source | A + format reflowed includes + `misra_renumber.py`, plus `check_spdx_headers.py` when a file was added | ~2–3 min |
| **C** — none | — | — | — |
| **CI** — everything else | After push | `tidy`, `sanitize`, `coverage`, Windows, BDD, integration, FreeRTOS host/cross, advisory IWYU, MISRA on cpputest | runs in parallel; results in ~10–15 min |

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

## Running Tier B

### MISRA — fix line-number drift

When edits shift production lines, `misra_suppressions.txt` entries go
stale. Fix in one step:

```bash
# In any container that has cppcheck (all of them do):
scripts/misra_renumber.py            # show proposed renumbers
scripts/misra_renumber.py --apply    # write back updated suppressions
```

The script bails on genuine new findings (mismatched counts per
rule+file); those need manual review. See the script's docstring.

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

CI runs both lanes advisory, findings appear in the `iwyu-report` and
`iwyu-report-freertos-plustcp` artifacts and don't block the build.

### Licence headers

Every file under `Core/` and `Platform/` opens with the SPDX header, and none
of them may claim anyone else's copyright:

```bash
python3 scripts/check_spdx_headers.py
```

Both the copyright line and the licence expression are read out of `LICENSE.md`
at run time, so there is nothing to keep in step by hand — the check fails if
the tree and the licence disagree.

The second half is a tripwire rather than a style rule. `Core/` and `Platform/`
contain no third-party code, and that invariant is what makes it safe to stamp
our copyright across every file in them. If it fires, the question is whether
the file belongs in the shipped library at all — there is deliberately no
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

- `tidy`, `sanitize`, `coverage`: minutes each, all gated by CI
- `c99`: the `build-linux-c99` lane builds the library at the C99 language
  standard on every PR. If it fails, either fix the construct or, if it
  genuinely belongs to a C11-only component, gate that component the way
  `Platform/StdAtomic` is gated. See [builds.md](builds.md#c99-portability--c99)
- Windows MSVC + BDD + integration: depend on tools you may not have
- BDD-linux-syslog-ng, BDD-windows-otel, BDD-freertos-qemu: heavy
  multi-container stacks

If CI surfaces a finding you missed locally, fix in another commit on the
same branch, cheaper than running every CI lane on every push.
