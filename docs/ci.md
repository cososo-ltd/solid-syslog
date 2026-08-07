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
| `analyze-iwyu` | `iwyu` | include-what-you-use; fails on missing or unused `#include` directives. Advisory — runs `continue-on-error` |
| `analyze-tidy-freertos-plustcp` | `tidy` | clang-tidy over the FreeRTOS / FreeRTOS-Plus-TCP / Mbed TLS / Plus-FAT trees, which the base `analyze-tidy` lane cannot reach |
| `analyze-tidy-freertos-lwip` | `tidy` | The same, over the lwIP and ChaN FatFs trees |
| `analyze-iwyu-freertos-plustcp` | `iwyu` | IWYU over the same FreeRTOS-Plus-TCP set. Advisory |
| `analyze-iwyu-freertos-lwip` | `iwyu` | IWYU over the same lwIP set. Advisory |
| `analyze-markdown` | — | `markdownlint-cli2` over every tracked `.md` file |
| `integration-linux-openssl` | `debug` | Runs the in-process TLS integration tests against libssl (no network oracle) |
| `integration-linux-mbedtls` | `debug` | The same integration tests against Mbed TLS, exercising `SolidSyslogMbedTlsStream` and the Mbed TLS security policies |
| `integration-windows-openssl` | `msvc-debug` | Same TLS integration tests on `windows-latest` against libssl from vcpkg |
| `build-linux-c99` | `c99`, `c99-platforms` | Builds Core alone at strict `-std=c99` (`CMAKE_C_EXTENSIONS=OFF`, no tests), then the POSIX and OpenSSL packs at C99 as a drift check. Proves the C99 conformance claim per PR |
| `build-linux-tunable-override` | `tunable-override-debug` | Builds against a user tunables header to prove `SOLIDSYSLOG_USER_TUNABLES_FILE` overrides the defaults |
| `bdd-linux-syslog-ng` | — | End-to-end BDD test via Docker Compose (`syslog-ng-linux` + `behave-linux`), Linux runner |
| `bdd-windows-otel` | — | Windows-eligible BDD scenarios driven against an OTel Collector oracle |
| `build-freertos-host-tdd-plustcp` | `debug` | Host-TDD of the FreeRTOS, FreeRTOS-Plus-TCP, Plus-FAT, FatFs and Mbed TLS adapters against fakes; runs inside `cpputest-freertos` (upstream sources at fixed paths) |
| `build-freertos-target-plustcp` | `freertos-cross` | ARM cross-build (Cortex-M3, mps2-an385) of the BDD target ELF over FreeRTOS-Plus-TCP; uploads it as an artifact |
| `build-freertos-target-lwip` | `freertos-cross-lwip` | The same cross-build over lwIP with ChaN FatFs (`FreeRtos;LwipRaw;MbedTls;FatFs;Atomics`) |
| `bdd-freertos-qemu-plustcp` | — | Pulls the Plus-TCP target ELF, brings up the freertos compose pair (`syslog-ng-freertos` + `behave-freertos`); Behave drives the target through `qemu-system-arm`'s UART |
| `bdd-freertos-qemu-lwip` | — | The same scenarios against the lwIP target ELF |
| `consumer-smoke-linux` | — | Builds [`ci/consumer-smoke/`](../ci/consumer-smoke/) as a FetchContent consumer, proving the documented integration path still works |
| `consumer-smoke-freertos-cross` | — | The same consumer project cross-compiled for ARM with `LwipRaw;FreeRtos` |
| `verify-manifest` | — | Regenerates the Core and per-platform source manifests and fails if they differ from the committed ones |
| `docs-build` | — | Builds the MkDocs + mkdoxy site with `mkdocs build --strict`; on `main`, `deploy-docs-pages` publishes it to GitHub Pages |
| `summary` | — | Aggregates the JUnit artifacts into a run summary. Declared `if: always()` and asserts nothing about the other jobs' results |

## Branch protection

Every job in `ci.yml` is a required status check except `deploy-docs-pages`, which
only runs on `main`, and so are the two contexts code scanning contributes —
`analyze-codeql` and `CodeQL`. That is 33 required contexts. A PR cannot be merged
unless all of them pass. Direct pushes to `main` are blocked. Squash merge only.

Two qualifications on what "required" buys. The `analyze-iwyu*` lanes run
`continue-on-error`, so they are required contexts that report success whatever IWYU
finds — required in form, advisory in substance. And feeding the `summary` aggregator
does not make a lane blocking: `summary` is declared `if: always()` and asserts nothing
about `needs.*.result`, so a new lane gates merges only once its own context is added
to the required list.

## What each lane exercises

The lane names say the platform and toolchain but not the adapter, so:

| Adapter | Where it is exercised |
|---|---|
| OpenSSL (`SolidSyslogTlsStream`, security policies) | `integration-linux-openssl`, `integration-windows-openssl` against real libssl |
| Mbed TLS (`SolidSyslogMbedTlsStream`, security policies) | `integration-linux-mbedtls` against real Mbed TLS; both FreeRTOS QEMU BDD lanes over a real handshake |
| FreeRTOS-Plus-TCP | `build-freertos-host-tdd-plustcp` against fakes; `bdd-freertos-qemu-plustcp` end to end under QEMU |
| lwIP | `bdd-freertos-qemu-lwip` end to end under QEMU; static analysis via the `*-freertos-lwip` lanes |
| ChaN FatFs | Built and analysed in the lwIP lanes; store-and-forward scenarios run in `bdd-freertos-qemu-lwip` |
| FreeRTOS-Plus-FAT | Host-TDD against fakes in `build-freertos-host-tdd-plustcp`, and built in the Plus-TCP cross lanes |
| POSIX, Windows | The `build-linux-*` and `build-windows-msvc` lanes, plus both host BDD lanes |

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

Each job is granted only the permissions it needs. Every workflow declares
`permissions: contents: read` at the top level and no workflow grants a write scope
there, so a job added later starts read-only rather than inheriting a write token by
default. Write scopes are held by the jobs that need them: jobs publishing test
results add `checks: write` and `pull-requests: write`; `deploy-docs-pages` adds
`pages: write` and `id-token: write` to publish the documentation site to GitHub
Pages; `analyze-codeql` adds `security-events: write` to upload its results to the
Security tab; `sbom.yml`'s publish job adds `contents: write` and `id-token: write` to
attach keyless-signed assets to a release; and `release-please.yml`'s job adds
`contents: write` and `pull-requests: write` to maintain the release PR and create the
tag.
