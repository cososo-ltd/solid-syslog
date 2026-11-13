# CI pipeline

GitHub Actions runs these jobs on every push and pull request to `main`, in parallel,
except where a job `needs:` another and waits for it to finish.

## Jobs

A job name starts with the kind of testing it does, and each kind has a fixed order of
fields after it. Pack names are the platform registry tokens in lower case, so they match
`SOLIDSYSLOG_PLATFORMS` and `Platform/<Pack>/`. No field contains a hyphen, so a name
always splits cleanly on `-`.

| Kind | Means | Shape |
|---|---|---|
| `unit` | CppUTest over the fakes, the `junit` target | `unit-<host>-<compiler>[-<variant>]` |
| `integration` | CppUTest against a real upstream, in process | `integration-<host>-<pack>` |
| `bdd` | Behave end to end against a real collector | `bdd-<os>-<network>-<tls>-<file>-<collector>[-<variant>]`, leaving out what the OS pack provides itself |
| `build` | Compiles and runs no tests of its own | the fields of the BDD lane it feeds, without the collector, or `build-<standard>`, or `build-nucleo-<image>` for firmware built for the NUCLEO-F767ZI, which CI never runs |
| `smoke` | The FetchContent consumer build | `smoke-<os>[-<packs>]` |
| `analyze` | Static checks | `analyze-<tool>[-<scope>]` |
| `docs`, `tools` | The site, and tests of the repository's own tooling | |

A lane that does two jobs is named for what only it proves: `unit-linux-gcc-tunables` also
builds the Linux BDD target, and `build-linux-wolfssl` also runs the unit suite. Every
non-hosted OS runs under QEMU in CI, so no name says so.

| Job | Preset | Notes |
|---|---|---|
| `unit-linux-gcc` | `debug` | All fourteen packs a Linux host can build, on `cpputest-freertos`; also builds the Linux BDD target against the default tunables (every other host build of it uses a tunables override), and builds the tree once more with Plus-TCP deselected, the shape an lwIP-only integrator has. Test results annotated on PR |
| `unit-linux-clang` | `clang-debug` | Every pack `unit-linux-gcc` builds, built and tested with Clang 19 on `cpputest-clang`, which carries the upstream trees `cpputest-freertos` does at the same pins |
| `unit-windows-msvc` | `msvc-debug` | MSVC build on `windows-latest`, with its dependencies from vcpkg as the [Windows build](builds.md#windows-build---msvc-debug) describes; test results annotated on PR |
| `unit-linux-gcc-sanitize` | `sanitize` | All fourteen packs under ASan + UBSan, with UBSan halting on its first finding rather than recovering, and address-space randomisation off for the tests - test results annotated on PR |
| `unit-linux-gcc-coverage` | `coverage` | Every test executable registered with `junit` across all fourteen packs, lines and branches, gated at 99% each over all packs by `scripts/coverage_summary.py`; its per-pack table is the step summary, and `docs-build` publishes the HTML report with the site at [Coverage](testing/coverage.md) |
| `analyze-tidy` | `tidy` | clang-tidy - pass/fail with errors in job log |
| `analyze-cppcheck` | `cppcheck` | cppcheck static analysis, and the enforcing MISRA C:2012 addon |
| `analyze-codeql` | - | CodeQL over the library as a consumer builds it; findings in Security → Code scanning. Its own workflow (`codeql.yml`) - see *Code scanning* |
| `analyze-format` | - | clang-format dry-run; fails if any file needs reformatting |
| `analyze-iwyu` | `iwyu` | include-what-you-use over every pack a Linux host can build, on `cpputest-clang`; fails on missing or unused `#include` directives. Advisory - runs `continue-on-error` |
| `analyze-tidy-embedded` | `tidy` | clang-tidy on `cpputest-freertos`, whose upstream trees select the eight embedded packs the base `analyze-tidy` lane cannot reach: FreeRtos, PlusTcp, PlusFat, LwipRaw, LwipSocket, MbedTls, FatFs and LittleFs |
| `analyze-markdown` | - | `markdownlint-cli2` over every tracked `.md` file |
| `analyze-spdx` | - | `scripts/check_spdx_headers.py`: every file under `Core/` and `Platform/` carries the licence header `LICENSE.md` declares, and none claims anyone else's copyright |
| `integration-linux-openssl` | `debug` | Runs the in-process TLS integration tests against libssl (no network oracle) |
| `integration-linux-mbedtls` | `debug` | The same integration tests against Mbed TLS, exercising `SolidSyslogMbedTlsStream` and the Mbed TLS security policies |
| `integration-linux-wolfssl` | `debug` | The same integration tests against wolfSSL, twice: against the build the pack asks for and against the baseline build the image also carries |
| `integration-windows-openssl` | `msvc-debug` | Same TLS integration tests on `windows-latest` against libssl from vcpkg |
| `integration-linux-littlefs` | `debug` | Drives `SolidSyslogLittleFsFile` against a real LittleFS volume rather than a fake, because durability is the pack's whole claim and nothing short of the filesystem proves `lfs_file_sync` committed. Runs in `cpputest-freertos`, which carries `/opt/littlefs` |
| `build-c89-headers` | - | Compiles every public header standalone as ISO C89 with `-pedantic-errors`, via `scripts/check_headers_c89.py`. Also proves each header is self-contained, since a header needing a companion first fails here. Then `scripts/check_vxworks64_header_proof.py` asserts that `Bdd/Targets/VxWorks64/BddTargetVxWorks64Headers.c`, the C89 proof the VxWorks 6.4 project compiles and no lane can build, includes every public header of Core and that pack and nothing else of ours |
| `build-c99` | `c99`, `c99-platforms` | Builds Core alone at strict `-std=c99` (`CMAKE_C_EXTENSIONS=OFF`, no tests), then the POSIX and OpenSSL packs at C99 as a drift check. Proves the C99 conformance claim per PR |
| `unit-linux-gcc-tunables` | `tunable-override-debug` | Builds against a user tunables header to prove `SOLIDSYSLOG_USER_TUNABLES_FILE` overrides the defaults. Also builds the Linux BDD target, for the reason under [`@requires_message_size_1500`](bdd.md#feature-tags) |
| `bdd-linux-openssl-syslogng` | - | End-to-end BDD test via Docker Compose (`syslog-ng-linux` + `behave-linux`), Linux runner |
| `bdd-linux-openssl-syslogng-passthrough` | - | The same Linux target under `--buffer passthrough` - no service thread, the logger serviced on the thread that logs, which is what the hardening path's first wiring does. Runs every scenario not tagged `@buffered`, and the `@passthrough` scenarios no other lane can |
| `build-linux-wolfssl` | `tunable-override-wolfssl` | The Linux BDD target over the wolfSSL pack in place of OpenSSL, with the unit tests; uploads the target binary |
| `bdd-linux-wolfssl-syslogng` | - | The Linux scenarios against that target (`syslog-ng-linux` + `behave-linux-wolfssl`) |
| `bdd-windows-openssl-otel` | - | Windows-eligible BDD scenarios driven against an OTel Collector oracle |
| `build-freertos-plustcp-mbedtls-plusfat` | `freertos-cross` | ARM cross-build (Cortex-M3, mps2-an385) of the BDD target ELF over FreeRTOS-Plus-TCP; uploads it as an artifact. `scripts/check_build_provenance.py` then reads the build's dependency files and asserts every file the compiler read came from a tree the target's CMake declares it wires, inside the subtrees it wires, since the image pins every upstream tree for every build and carries a second FreeRTOS kernel bundled inside CMSIS-FreeRTOS as well as example copies of the configuration headers; the per-tree counts go to the step summary |
| `build-freertos-lwipraw-mbedtls-fatfs` | `freertos-cross-lwip` | The same cross-build over lwIP with ChaN FatFs (`FreeRtos;LwipRaw;MbedTls;FatFs;StdAtomic`), with the same provenance check |
| `bdd-freertos-plustcp-mbedtls-plusfat-syslogng` | - | Pulls the Plus-TCP target ELF, brings up the freertos compose pair (`syslog-ng-freertos` + `behave-freertos`); Behave drives the target through `qemu-system-arm`'s UART |
| `bdd-freertos-lwipraw-mbedtls-fatfs-syslogng` | - | The same scenarios against the lwIP target ELF |
| `build-cmsisrtos-lwipsocket-mbedtls-littlefs` | `cmsis-cross-lwip` | The cross-build of the CMSIS-RTOS2 BDD target, over CMSIS-RTOS2, LittleFS and the lwIP Sockets API (`CmsisRtos;LwipSocket;MbedTls;LittleFs;StdAtomic`) - the three packs it exists to prove, plus Mbed TLS; with the same provenance check, whose summary adds the pinned kernel's version beside the one CMSIS-FreeRTOS bundles |
| `bdd-cmsisrtos-lwipsocket-mbedtls-littlefs-syslogng` | - | The same scenarios again, against that target's ELF |
| `build-cmsisrtos-lwipsocket-wolfssl-littlefs` | `cmsis-cross-lwip-wolfssl` | The same target over the wolfSSL pack in place of Mbed TLS, with wolfSSL compiled from source under the target's `user_settings.h`; asserts from the ELF's symbols which TLS pack was linked, and runs the same provenance check |
| `bdd-cmsisrtos-lwipsocket-wolfssl-littlefs-syslogng` | - | The QEMU scenarios against that ELF, less the one cell the wolfSSL platform page lists as a shortfall |
| `build-nucleo-bringup` | `nucleo-f767zi-bringup` | ARM cross-build (Cortex-M7, hard float) of the NUCLEO-F767ZI bring-up firmware, which carries no SolidSyslog, over ST's HAL driver and CMSIS device files; with the same provenance check, and an assertion that the image's banner names the commit built. Uploads the ELF and a raw binary. Nothing in CI reaches the board: the image is flashed by hand and the run recorded against the target checklist in `Bdd/Targets/Boards/NucleoF767zi/README.md` |
| `build-nucleo-freertos-lwipraw` | `nucleo-f767zi-lwip` | ARM cross-build (Cortex-M7, hard float) of the FreeRTOS + lwIP Raw BDD target for the NUCLEO-F767ZI: the code `build-freertos-lwipraw-mbedtls-fatfs` builds for QEMU, over that board's half of the board seam and ST's HAL; with the same provenance check and banner assertion. Uploads the ELF. Its addresses are the configure's defaults, which reach nothing, so the lane proves the commit builds and a checklist run builds its own image for the test network |
| `smoke-linux` | - | Builds `ci/consumer-smoke/` as a FetchContent consumer, proving the documented integration path still works |
| `smoke-freertos-lwipraw` | - | The same consumer project cross-compiled for ARM with `LwipRaw;FreeRtos` |
| `analyze-manifest` | - | Regenerates the Core and per-platform source manifests and fails if they differ from the committed ones |
| `tools-vxworks64-runner` | - | Python `unittest` over the VxWorks 6.4 runner's job service: its queue, token, routes and TLS. The runner's Pester tests need Windows PowerShell 5.1 and run locally, recorded in the PR |
| `docs-build` | - | Builds the MkDocs + mkdoxy site with `mkdocs build --strict`, runs the documentation consistency, reference and issue-link checks and the hook tests, and rehearses the offline bundle; waits for every lane that uploads JUnit results and for `unit-linux-gcc-coverage`, and runs whether they passed or not, generating the [test report](testing/tests.md) from the one and staging the [coverage report](testing/coverage.md) from the other under the Testing pages, so a red run can be read. Every `junit-*` artifact also carries its lane's record of what it compiled against, written by the configure (`cmake/VersionRecord.cmake`), and `scripts/check_tested_versions.py` holds each platform page's `<!-- tested-against: ... -->` and `<!-- tested-with: ... -->` markers to it, so a page naming a version no lane built against, or a compiler no lane built it with, fails here; on `main`, `docs-deploy` publishes it to GitHub Pages |
| `analyze-actionlint` | - | Validates the workflow files themselves. In its own workflow, not a job here: a lane inside the file being validated cannot run when that file is the broken one |
| `docs-links` | - | Checks the documentation's external links with lychee. Its own workflow (`docs-links.yml`); advisory on pull requests, failing on its weekly schedule |
| `summary` | - | Aggregates the JUnit artifacts into a run summary. Declared `if: always()` and asserts nothing about the other jobs' results |

## Branch protection

Every job in `ci.yml` is a required status check except `docs-deploy`, which
only runs on `main`. Three more come from outside that file: `analyze-codeql` and
`CodeQL`, which code scanning contributes, and `analyze-actionlint`, which validates the
workflow files from a workflow of its own. A pull request cannot be merged unless
all of them pass. Direct pushes to `main` are blocked. Squash merge only.

Two qualifications on what "required" buys. The `analyze-iwyu` lane runs
`continue-on-error`, so it is a required context that reports success whatever IWYU
finds - required in form, advisory in substance. And feeding the `summary` aggregator
does not make a lane blocking: `summary` is declared `if: always()` and asserts nothing
about `needs.*.result`, so a new lane gates merges only once its own context is added
to the required list.

## What each lane exercises

The lane names say the platform and toolchain but not the adapter, so:

<!-- platforms: all -->

| Adapter | Where it is exercised |
|---|---|
| OpenSSL (`SolidSyslogOpenSslStream`, security policies) | `integration-linux-openssl`, `integration-windows-openssl` against real libssl |
| Mbed TLS (`SolidSyslogMbedTlsStream`, security policies) | `integration-linux-mbedtls` against real Mbed TLS; the three Mbed TLS QEMU BDD lanes over a real handshake |
| wolfSSL (`SolidSyslogWolfSslStream`, security policies) | Unit tests against the fake in every host lane that builds `Tests/`; `integration-linux-wolfssl` against both wolfSSL builds the image carries; `build-linux-wolfssl` and `bdd-linux-wolfssl-syslogng`, the Linux target without OpenSSL; `build-cmsisrtos-lwipsocket-wolfssl-littlefs` and `bdd-cmsisrtos-lwipsocket-wolfssl-littlefs-syslogng`, a wolfSSL built from source under QEMU |
| FreeRTOS-Plus-TCP | `unit-linux-gcc` and its sanitizer and coverage lanes against fakes; `bdd-freertos-plustcp-mbedtls-plusfat-syslogng` end to end under QEMU |
| lwIP (Raw API) | `unit-linux-gcc` and its sanitizer and coverage lanes against fakes; `bdd-freertos-lwipraw-mbedtls-fatfs-syslogng` end to end under QEMU; static analysis in the `analyze-*-embedded` lanes |
| lwIP (Sockets API) | `unit-linux-gcc` and its sanitizer and coverage lanes against fakes; `build-cmsisrtos-lwipsocket-mbedtls-littlefs` cross build; `bdd-cmsisrtos-lwipsocket-mbedtls-littlefs-syslogng` end to end under QEMU |
| FreeRTOS kernel | `unit-linux-gcc` and its sanitizer and coverage lanes against fakes; both FreeRTOS cross builds and both FreeRTOS QEMU BDD lanes |
| CMSIS-RTOS2 | Unit tests in every host lane that builds `Tests/`; `build-cmsisrtos-lwipsocket-mbedtls-littlefs` cross build; `bdd-cmsisrtos-lwipsocket-mbedtls-littlefs-syslogng` end to end under QEMU |
| VxWorks 6.4 | Unit tests at strict C99 against fakes in every host lane that builds `Tests/`. Target runs are outside CI: the toolchain and kernel are licensed, so the target checklist in `Bdd/Targets/VxWorks64/README.md` covers them |
| ChaN FatFs | Unit tested in `unit-linux-gcc` and its sanitizer and coverage lanes, analysed in the `analyze-*-embedded` lanes; store-and-forward scenarios run in `bdd-freertos-lwipraw-mbedtls-fatfs-syslogng` |
| FreeRTOS-Plus-FAT | Host-TDD against fakes in `unit-linux-gcc` and its sanitizer and coverage lanes, and built in the Plus-TCP cross lane; store-and-forward scenarios run in `bdd-freertos-plustcp-mbedtls-plusfat-syslogng` |
| LittleFS | `unit-linux-gcc` and its sanitizer and coverage lanes against fakes; `integration-linux-littlefs` against real littlefs; store-and-forward scenarios run in `bdd-cmsisrtos-lwipsocket-mbedtls-littlefs-syslogng` |
| POSIX, Windows | Every Linux host lane that builds `Tests/`, `build-linux-wolfssl`, and `unit-windows-msvc`, plus the host BDD lanes |
| C11 atomics | Probe-selected into `libSolidSyslog.a` in every Linux host lane that builds `Tests/` except `build-linux-wolfssl`, whose preset names it, as the three lwIP cross presets do. Not in `build-c99`, which has no C11 header to find |

<!-- /platforms -->

## Code scanning

`codeql.yml` runs CodeQL over the C sources on every push and pull request to `main`,
and weekly so that queries GitHub ships later are applied to unchanged code. Findings
appear under **Security → Code scanning**, not in the job log.

CodeQL analyses what the compiler compiled and nothing else, so the build it observes
is the analysis scope. The lane builds `ci/consumer-smoke/` -
the same FetchContent consumer documented in
[Adding it to your build](build-integration.md) - so the code analysed is the code an
integrator compiles. Consuming the library as a subproject also scopes the database
without any bespoke flags: `SOLIDSYSLOG_IS_TOP_LEVEL` is false, so the unit tests and
the BDD targets are never configured.

`SOLIDSYSLOG_PLATFORMS` is named explicitly rather than left to auto-detection, because
the list is authoritative: a platform it does not name is silently absent from the build,
and therefore from the analysis. An assertion step then fails the lane unless every named
platform produced compiled objects - proving what the compiler emitted rather than what
the configure step was asked for, since a platform that is selected but contributes no
sources would satisfy any check of the selection itself.

Triage follows the support tiers rather than the reported severity:

- **Tier 1 `Core/`** - treat as blocking; this is the shipped product.
- **Tier 2 `Platform/`** - fix on merit; a finding here does not hold up a release.

Accepted findings are dismissed in the Security tab with a reason, not suppressed with
in-source comments. The C sources already carry clang-tidy, cppcheck-MISRA and IWYU
suppression dialects, and a fourth would cost more in readability than it returns.

Code scanning contributes two required contexts, and both are needed:

- **`analyze-codeql`** - the Actions job. Proves the analysis ran and uploaded, so a
  lane that breaks or stops running blocks the merge rather than passing silently.
- **`CodeQL`** - the code-scanning results check. This is the one that fails when a
  pull request introduces a new alert. Without it, a PR could add findings and still
  merge green because the job itself succeeded.

Both were made required on a clean baseline: `security-extended` produced no alerts over
the consumer build it analyses, so nothing had to be grandfathered in.

A clean baseline is not the same as an absence of vulnerabilities. Much of
`security-extended` is taint tracking, and this database offers it no source: `Core/`
makes no library read calls of its own, and everything arrives through a vtable that
CodeQL's dataflow does not follow. Those queries cannot fire whatever the code does.
Read the result as *no local defects found* - buffer arithmetic, conversions,
comparisons, unchecked returns - rather than as proof that no injection or overflow
path exists.

## Release automation

[release-please](https://github.com/googleapis/release-please) runs on every push to `main`.
It reads commit messages (which must follow [Conventional Commits](https://www.conventionalcommits.org/))
and maintains a release PR that bumps the version and updates `CHANGELOG.md`.
Merging that PR creates the tag and a draft GitHub Release; publishing it is a
separate step, covered in the [release process](release-process.md).

## Permissions

Each job is granted only the permissions it needs. Every workflow declares
`permissions: contents: read` at the top level and no workflow grants a write scope
there, so a job added later starts read-only rather than inheriting a write token by
default. Write scopes are held by the jobs that need them: jobs publishing test
results add `checks: write`, and only `summary` adds `pull-requests: write` to comment
on the pull request; `docs-deploy` adds
`pages: write` and `id-token: write` to publish the documentation site to GitHub
Pages; `analyze-codeql` adds `security-events: write` to upload its results to the
Security tab; `sbom.yml`'s publish job adds `contents: write` and `id-token: write` to
attach keyless-signed assets to a release, as does `docs-bundle.yml`'s publish job for
the documentation bundle; and `release-please.yml`'s job declares `contents: write` and
`pull-requests: write`, though the release PR and the tag are made with a GitHub App
token rather than the job's own. `image-bump.yml`'s job stays read-only: the push and
the pull request it opens use the same App token, so the bump runs CI like any other.
