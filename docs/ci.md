# CI Pipeline

GitHub Actions runs all jobs in parallel on every push and pull request to `main`.

## Jobs

Job names follow the pattern `<category>-<platform>-<toolchain-or-feature>` so additional
compilers and BDD targets (e.g. an ARM cross-compile or FreeRTOS QEMU oracle) slot in
without renaming what's already there.

| Job | Preset | Notes |
|---|---|---|
| `build-linux-gcc` | `debug` | Test results annotated on PR |
| `build-linux-clang` | `clang-debug` | Second compiler check using Clang 19 |
| `build-windows-msvc` | `msvc-debug` | MSVC build on `windows-latest`; CppUTest via vcpkg; test results annotated on PR |
| `sanitize-linux-gcc` | `sanitize` | ASan + UBSan — test results annotated on PR |
| `coverage-linux-gcc` | `coverage` | Summary in Actions UI; HTML report uploaded as a downloadable run artifact |
| `analyze-tidy` | `tidy` | clang-tidy — pass/fail with errors in job log |
| `analyze-cppcheck` | `cppcheck` | cppcheck static analysis |
| `analyze-codeql` | — | CodeQL over the library as a consumer builds it; findings in Security → Code scanning. Its own workflow (`codeql.yml`) — see *Code scanning* |
| `analyze-format` | — | clang-format dry-run; fails if any file needs reformatting |
| `analyze-iwyu` | `iwyu` | include-what-you-use; fails on missing or unused `#include` directives |
| `integration-linux-openssl` | `debug` | Runs the in-process TLS integration tests against libssl (no network oracle) |
| `integration-windows-openssl` | `msvc-debug` | Same TLS integration tests on `windows-latest` against libssl from vcpkg |
| `bdd-linux-syslog-ng` | — | End-to-end BDD test via Docker Compose (`syslog-ng-linux` + `behave-linux`), Linux runner |
| `bdd-windows-otel` | — | Windows-eligible BDD scenarios driven against an OTel Collector oracle |
| `build-freertos-host-tdd` | `debug` | Host-TDD of FreeRTOS adapters against fakes; runs inside `cpputest-freertos` (FreeRTOS upstream sources at fixed paths) |
| `build-freertos-target` | `freertos-cross` | ARM cross-build (Cortex-M3, mps2-an385) of the BDD target ELF; uploads it as an artifact for `bdd-freertos-qemu` |
| `bdd-freertos-qemu` | — | Pulls the BDD target ELF artifact, brings up the freertos compose pair (`syslog-ng-freertos` + `behave-freertos`); Behave drives the target through `qemu-system-arm`'s UART |
| `docs-build` | — | Builds the MkDocs + mkdoxy site with `mkdocs build --strict`; on `main`, `deploy-docs-pages` publishes it to GitHub Pages |

## Branch protection

Every job in `ci.yml` is a required status check, as are the two contexts code
scanning contributes. A PR cannot be merged unless all checks pass. Direct pushes
to `main` are blocked. Squash merge only.

## Code scanning

`codeql.yml` runs CodeQL over the C sources on every push and pull request to `main`,
and weekly so that queries GitHub ships later are applied to unchanged code. Findings
appear under **Security → Code scanning**, not in the job log.

CodeQL analyses what the compiler compiled and nothing else, so the build it observes
is the analysis scope. The lane builds [`ci/consumer-smoke/`](../ci/consumer-smoke/) —
the same FetchContent consumer documented in
[Adding it to your build](build-integration.md) — so the code analysed is the code an
integrator compiles. Consuming the library as a subproject also scopes the database
without any bespoke flags: `SOLIDSYSLOG_IS_TOP_LEVEL` is false, so the unit tests and
the BDD targets are never configured.

`SOLIDSYSLOG_PLATFORMS` is named explicitly rather than left to auto-detection, and an
assertion step fails the lane if a named platform is not selected. The list is
authoritative: a pack it does not name is silently absent from the build, and therefore
from the analysis.

Triage follows the support tiers rather than the reported severity:

- **Tier 1 `Core/`** — treat as blocking; this is the shipped product.
- **Tier 2 `Platform/`** — fix on merit; a finding here does not hold up a release.

Accepted findings are dismissed in the Security tab with a reason, not suppressed with
in-source comments. The C sources already carry clang-tidy, cppcheck-MISRA and IWYU
suppression dialects, and a fourth would cost more in readability than it returns.

Code scanning contributes two required contexts, and both are needed:

- **`analyze-codeql`** — the Actions job. Proves the analysis ran and uploaded, so a
  lane that breaks or stops running blocks the merge rather than passing silently.
- **`CodeQL`** — the code-scanning results check. This is the one that fails when a
  pull request introduces a new alert. Without it, a PR could add findings and still
  merge green because the job itself succeeded.

Both were made required on a clean baseline: `security-extended` over 58,144 lines of
C produced no alerts, so nothing had to be grandfathered in.

A clean baseline is not the same as an absence of vulnerabilities. Much of
`security-extended` is taint tracking, and this database offers it no source: `Core/`
makes no library read calls of its own, and everything arrives through a vtable that
CodeQL's dataflow does not follow. Those queries cannot fire whatever the code does.
Read the result as *no local defects found* — buffer arithmetic, conversions,
comparisons, unchecked returns — rather than as proof that no injection or overflow
path exists.

## Release automation

[release-please](https://github.com/googleapis/release-please) runs on every push to `main`.
It reads commit messages (which must follow [Conventional Commits](https://www.conventionalcommits.org/))
and maintains a release PR that bumps the version and updates `CHANGELOG.md`.
Merging that PR creates a GitHub Release and tag.

## Permissions

Each job is granted only the permissions it needs. The default token scope is
`contents: read`. Jobs that publish test results additionally hold `checks: write`
and `pull-requests: write`. The `deploy-docs-pages` job additionally holds
`pages: write` and `id-token: write` to publish the documentation site to GitHub Pages.
`analyze-codeql` additionally holds `security-events: write` to upload its results to
the Security tab.
