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
| `coverage-linux-gcc` | `coverage` | Summary in Actions UI; HTML report deployed to GitHub Pages on merge to `main` |
| `analyze-tidy` | `tidy` | clang-tidy — pass/fail with errors in job log |
| `analyze-cppcheck` | `cppcheck` | cppcheck static analysis |
| `analyze-format` | — | clang-format dry-run; fails if any file needs reformatting |
| `analyze-iwyu` | `iwyu` | include-what-you-use; fails on missing or unused `#include` directives |
| `integration-linux-openssl` | `debug` | Runs the in-process TLS integration tests against libssl (no network oracle) |
| `bdd-linux-syslog-ng` | — | End-to-end BDD test via Docker Compose (syslog-ng + Behave), Linux runner |
| `bdd-windows-otel` | — | Windows-eligible BDD scenarios driven against an OTel Collector oracle |

## Branch protection

All jobs are required status checks. A PR cannot be merged unless all checks pass.
Direct pushes to `main` are blocked. Squash merge only.

## Release automation

[release-please](https://github.com/googleapis/release-please) runs on every push to `main`.
It reads commit messages (which must follow [Conventional Commits](https://www.conventionalcommits.org/))
and maintains a release PR that bumps the version and updates `CHANGELOG.md`.
Merging that PR creates a GitHub Release and tag.

## Permissions

Each job is granted only the permissions it needs. The default token scope is
`contents: read`. Jobs that publish test results additionally hold `checks: write`
and `pull-requests: write`. The coverage job additionally holds `pages: write` and
`id-token: write` for GitHub Pages deployment.
