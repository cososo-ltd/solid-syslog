# Pre-PR local checks

The full CI suite takes ~10–15 minutes wall-clock. Running every check
locally before pushing trades human time for one extra reviewer-confidence
margin we mostly don't need. This document defines what to run locally and
when, so the wait stays under a few minutes and CI catches the rest.

## Tiers

| Tier | When | What | Wall-clock |
|---|---|---|---|
| **A** — fast feedback | Every commit on the branch | `cmake --build --preset debug --target junit` for whatever preset matches the diff (gcc / clang / freertos-host) | ~30–60 s |
| **B** — pre-push | First push to the branch and any push that changes production source | A + format reflowed includes + `misra_renumber.py` | ~2–3 min |
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

## Markdown

Markdown is linted in CI by the `analyze-markdown` lane (markdownlint-cli2
v0.22.1), wired into `summary`. The rules live in `.markdownlint-cli2.jsonc`,
our conventions (line-length and table-column-style off, fenced-code language
required); `CHANGELOG.md` and `LICENSE.md` are ignored.

If you touch any `.md`, run it locally first. Same pinned engine as CI and
CodeRabbit, via Docker (no Node needed):

```bash
# Lint (reads .markdownlint-cli2.jsonc automatically):
docker run --rm -v "$PWD:/workdir" davidanson/markdownlint-cli2:v0.22.1

# Auto-fix the mechanical rules (blank lines, trailing space, list style, ...):
docker run --rm -v "$PWD:/workdir" davidanson/markdownlint-cli2:v0.22.1 --fix
```

With Node available, `npx markdownlint-cli2@0.22.1` is equivalent. Fenced-code
languages (MD040) and a few structural rules are not auto-fixable; tag or
adjust those by hand.

## Pre-release checks

Some properties drift too rarely to be worth a per-PR CI lane but must hold
at release. Run these once while preparing a release, not on every branch.

### C99 language portability

The library is meant to stay C99-capable, with C11 atomics as an optional
add-on (the `SolidSyslogStdAtomicCounter`; a strict-C99 target falls back to
`SolidSyslogWindowsAtomicCounter` or `SolidSyslogNullAtomicCounter`). Nothing
per-PR enforces this, so verify it before a release with a one-shot build of
the library at the C99 language standard:

```bash
cmake --preset c99 && cmake --build --preset c99
```

The `c99` preset builds at `-std=gnu99` (C99 *language* with the platform's
normal library feature-test macros, so POSIX adapters still see
`clock_gettime` etc.) and declares `HAVE_STDATOMIC_H=OFF`, so the optional
C11 atomics counter is excluded exactly as it would be on a real C99 target.
The build runs under the project's standing `-Wpedantic -Werror`, so any C11
language construct that has crept into the portable code (`_Static_assert`,
`_Atomic`, statement-expressions, …) fails the build and names the file:line.

A clean build means the portable surface is still C99. If it fails, either
fix the construct or, if it genuinely belongs to a C11-only component,
gate that component the way `Platform/Atomics` is gated.

## What CI runs and you should not run locally

- `tidy`, `sanitize`, `coverage`: minutes each, all gated by CI
- Windows MSVC + BDD + integration: depend on tools you may not have
- BDD-linux-syslog-ng, BDD-windows-otel, BDD-freertos-qemu: heavy
  multi-container stacks

If CI surfaces a finding you missed locally, fix in another commit on the
same branch, cheaper than running every CI lane on every push.
