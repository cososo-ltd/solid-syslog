# Building and Testing

> This is the contributor / maintainer build doc: the CMake preset catalogue
> for developing the library and reproducing CI lanes locally. If you are
> consuming SolidSyslog in your own product (CMake or non-CMake), start at
> [Getting started](getting-started.md) instead.

All builds use CMake presets. Output goes to `build/<preset>/`.

## TDD loop — `debug` / `clang-debug`

The everyday build for writing and running tests. The active preset depends on the devcontainer
service in use (`debug` for `gcc`, `clang-debug` for `clang`).

```bash
cmake --preset $BUILD_PRESET
cmake --build --preset $BUILD_PRESET --target junit
```

In VS Code, Ctrl+Shift+B runs the build and test and reports pass/fail in the terminal.

## Clang build — `clang-debug`

Builds with Clang 19 as a second compiler, catching portability issues not caught by GCC.

When using the `gcc` devcontainer (normal development), run from a host terminal:

```bash
docker compose -f .devcontainer/docker-compose.yml run --rm clang cmake --preset clang-debug
docker compose -f .devcontainer/docker-compose.yml run --rm clang cmake --build --preset clang-debug --target junit
```

When using the `clang` devcontainer, Ctrl+Shift+B builds with `clang-debug` directly.
See [Container images](containers.md) for how to switch.

## C99 portability — `c99`

Compiles the library with `CMAKE_C_STANDARD=99` (and `HAVE_STDATOMIC_H=OFF`,
`BUILD_TESTING=OFF`) to enforce the C99 baseline: catches accidental use of
later-standard features in production source. Library only; tests are not built.

```bash
cmake --preset c99
cmake --build --preset c99
```

## Sanitizers — `sanitize`

Catches memory errors, use-after-free, and undefined behaviour at runtime.

```bash
cmake --preset sanitize
cmake --build --preset sanitize --target junit
```

## Coverage — `coverage`

Generates an HTML coverage report for the library source.

```bash
cmake --preset coverage
cmake --build --preset coverage --target coverage
```

Open `build/coverage/coverage_report/index.html` to view results.
The CI gate is 90% line and branch. The target is 100%.

## Static analysis — `tidy`

Runs clang-tidy on all source files. All warnings are errors.
Checks are configured in [.clang-tidy](../.clang-tidy).

```bash
cmake --preset tidy
cmake --build --preset tidy
```

## cppcheck — `cppcheck`

Runs cppcheck static analysis on all source files.

```bash
cmake --preset cppcheck
cmake --build --preset cppcheck
```

## Include-what-you-use — `iwyu` (advisory)

Runs include-what-you-use over the source set to flag missing or unused
`#include` directives. It inherits `clang-debug`, so use the `clang` (or
`cpputest-freertos`) image, not the `gcc` image.

```bash
cmake --preset iwyu
cmake --build --preset iwyu --target iwyu
```

Note the `--target iwyu`: building the bare preset does not run the tool.

IWYU is advisory, not a gate. The CI lanes (`analyze-iwyu`,
`analyze-iwyu-freertos-plustcp`, `analyze-iwyu-freertos-lwip`) run on every PR but
do not block the build; their findings land in the `iwyu-report*` artifacts. Sweep
those at release cleanup. See [local-checks.md](local-checks.md) for the FreeRTOS
variants and the full pre-PR check budget.

## Windows build — `msvc-debug`

Builds with MSVC as a portability check against GCC and Clang. Requires a Windows
environment with MSVC, CMake 3.25+, and vcpkg with CppUTest installed. The `VCPKG_ROOT`
environment variable must point to the vcpkg installation.

```bash
cmake --preset msvc-debug
cmake --build --preset msvc-debug --target junit
```

On GitHub Actions (`windows-latest`), CppUTest is installed via `vcpkg install cpputest`
and `VCPKG_ROOT` is set automatically.

POSIX-specific code (senders, message queue buffer, clock, hostname, PID) is excluded
by the existing `SOLIDSYSLOG_POSIX` CMake guards. The core library and portable tests
build and pass with MSVC.

## Release — `release`

Optimised build with no instrumentation. Used for the install target.

```bash
cmake --preset release
cmake --build --preset release --target junit
```

## FreeRTOS cross — `freertos-cross`

ARM cross-build for FreeRTOS targets running under `qemu-system-arm`
(Cortex-M3, mps2-an385). Uses the `freertos-target` devcontainer service
or a host with `arm-none-eabi-gcc` + `qemu-system-arm` on `PATH`.

```bash
cmake --preset freertos-cross
cmake --build --preset freertos-cross --target SolidSyslogBddTarget
```

The ELF lands at
`build/freertos-cross/Bdd/Targets/FreeRtos/SolidSyslogBddTarget.elf`.
See [`Bdd/Targets/FreeRtos/README.md`](../Bdd/Targets/FreeRtos/README.md) for run /
GDB-attach instructions and [`Bdd/README.md`](../Bdd/README.md) for driving
it under Behave + the syslog-ng oracle.

## FreeRTOS + lwIP cross — `freertos-cross-lwip`

The lwIP-networking twin of `freertos-cross`: same ARM Cortex-M3 / mps2-an385
cross-build, but with `SOLIDSYSLOG_FREERTOS_NET=LWIP` (instead of the default
FreeRTOS-Plus-TCP) and the `Bdd/Targets/FreeRtosLwip/` tunables. Drives the
`bdd-freertos-qemu-lwip` CI lane.

```bash
cmake --preset freertos-cross-lwip
cmake --build --preset freertos-cross-lwip --target SolidSyslogBddTargetLwip
```

The ELF lands at
`build/freertos-cross-lwip/Bdd/Targets/FreeRtosLwip/SolidSyslogBddTargetLwip.elf`.

## Installing the library

```bash
cmake --preset release
cmake --build --preset release
cmake --install build/release --prefix /your/install/path
```

This installs the static library to `lib/` and the public headers to `include/`.

## BDD tests — Behave

End-to-end tests run against per-target oracle pairs. The Linux pair uses the
`behave-linux` devcontainer service; switch to it by changing
`"service": "behave-linux"` in `.devcontainer/devcontainer.json` and
rebuilding, or run from the gcc container:

```bash
behave Bdd/features/
```

In the behave-linux container, Ctrl+Shift+B runs `behave Bdd/features/` automatically.

For the FreeRTOS pair (cross-build the BDD target ELF, then drive QEMU through
Behave inside `freertos-target`), see [`Bdd/README.md`](../Bdd/README.md).

See [BDD testing](bdd.md) for architecture details and the `BDD_TARGET` /
`@freertoswip` contract.

## JUnit XML output

The `junit` target runs the tests and writes a JUnit-format XML file to the build directory.
Used by the VS Code test explorer and the CI pipeline.

```bash
cmake --build --preset <preset> --target junit
```
