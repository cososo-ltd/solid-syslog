# Pre-PR local checks

The full CI suite takes ~10–15 minutes wall-clock. Running every check
locally before pushing trades human time for one extra reviewer-confidence
margin we mostly don't need. This document defines what to run locally and
when, so the wait stays under a few minutes and CI catches the rest.

## Tiers

| Tier | When | What | Wall-clock |
|---|---|---|---|
| **A** — fast feedback | Every commit on the branch | `cmake --build --preset debug --target junit` for whatever preset matches the diff (gcc / clang / freertos-host) | ~30–60 s |
| **B** — pre-push | First push to the branch and any push that changes production source | A + `iwyu` + format reflowed includes + `misra_renumber.py` | ~3–5 min |
| **C** — none | — | — | — |
| **CI** — everything else | After push | `tidy`, `sanitize`, `coverage`, Windows, BDD, integration, FreeRTOS host/cross, IWYU on cpputest-clang, MISRA on cpputest | runs in parallel; results in ~10–15 min |

Format-on-save in the editor handles formatting per-edit, so no separate
`analyze-format` step locally. If you skip an editor with format-on-save,
add a `clang-format -i` sweep over touched files to Tier A.

## Path-gating Tier B

Tier B does the expensive bits (IWYU, MISRA), so scope it to what changed:

- **Touched only `Tests/`, `Bdd/Targets/`, `docs/`, `cmake/`, or `*.md`** —
  skip Tier B entirely. Push and let CI run.
- **Touched any `Core/Source/`, `Platform/*/Source/`, or public-header file**
  — run Tier B against the container that compiles that tree (see below).

## Running Tier B

The IWYU lane runs against clang's parser. cpputest-freertos and cpputest
both ship the same IWYU binary now (clang_19 branch), so pick the container
whose env vars include the trees you touched:

### Touched only Core / Posix / Windows / OpenSSL trees

```bash
# Inside the gcc devcontainer (Ctrl+Shift+B-able), or:
docker compose -f .devcontainer/docker-compose.yml run --rm clang \
  bash -c 'cmake --preset iwyu && cmake --build --preset iwyu --target iwyu'
```

Read the full IWYU output — head/tail truncation hides findings. After
fixing IWYU's complaints, format the file (`clang-format -i path`) — the
include reorder can put forward-decls on lines that need reflowing.

### Touched any FreeRTOS / Plus-TCP / lwIP / MbedTLS / FatFs tree

```bash
docker compose -f .devcontainer/docker-compose.yml run --rm freertos-host \
  bash -c 'cmake --preset iwyu \
    -DCMAKE_C_COMPILER=clang-19 -DCMAKE_CXX_COMPILER=clang++-19 \
    && cmake --build --preset iwyu --target iwyu'
```

The `-DCMAKE_C_COMPILER=clang-19` overrides match the
`analyze-iwyu-freertos-plustcp` CI lane — the gcc base image carries
`clang-19` but no `clang` alternative.

### MISRA — after IWYU, fix line-number drift

When IWYU's include reorder shifts production lines, `misra_suppressions.txt`
entries go stale. Fix in one step:

```bash
# In any container that has cppcheck (all of them do):
scripts/misra_renumber.py            # show proposed renumbers
scripts/misra_renumber.py --apply    # write back updated suppressions
```

The script bails on genuine new findings (mismatched counts per
rule+file) — those need manual review. See the script's docstring.

## What CI runs and you should not run locally

- `tidy`, `sanitize`, `coverage` — minutes each, all gated by CI
- Windows MSVC + BDD + integration — depend on tools you may not have
- BDD-linux-syslog-ng, BDD-windows-otel, BDD-freertos-qemu — heavy
  multi-container stacks
- The cpputest-clang IWYU lane and the cpputest-freertos IWYU/tidy lanes —
  CI runs both; you only need one locally for the trees you touched

If CI surfaces a finding you missed locally, fix in another commit on the
same branch — cheaper than running every CI lane on every push.
