# Container Images

## Images in use

| Image | Tag | Used by |
|---|---|---|
| `ghcr.io/cososo-ltd/cpputest` | `sha-6715942` | devcontainer (`gcc` service), most CI jobs. Now ships `include-what-you-use 0.23 (clang_19)` matching the `cpputest-clang` build, so local `iwyu` runs work in the gcc container |
| `ghcr.io/cososo-ltd/cpputest-clang` | `sha-5905aea` | `clang` compose service, `build-linux-clang` CI job, `analyze-iwyu` CI job |
| `ghcr.io/cososo-ltd/cpputest-freertos` | `sha-ad10bf2` | `freertos-host` compose service, `build-freertos-host-tdd-plustcp` CI job — adds FreeRTOS-Kernel / Plus-TCP / Plus-FAT / lwIP / FatFs / Mbed TLS sources for host-TDD of FreeRTOS adapters against fakes; inherits IWYU from the rebased `cpputest` base, enabling the freertos-aware `analyze-iwyu` lane |
| `ghcr.io/cososo-ltd/cpputest-freertos-cross` | `sha-ad10bf2` | `freertos-target` compose service, `build-freertos-target-plustcp` CI job, `behave-freertos` BDD service, `bdd-freertos-qemu-plustcp` CI job — adds `gcc-arm-none-eabi`, `libnewlib-arm-none-eabi`, `gdb-multiarch` (aliased as `arm-none-eabi-gdb`), `qemu-system-arm`, `python3` + `behave`, FreeRTOS-Kernel / Plus-TCP / Plus-FAT / lwIP / FatFs sources at `/opt/freertos-kernel` / `/opt/freertos-plus-tcp` / `/opt/freertos/plus-fat` / `/opt/lwip` / `/opt/fatfs` for cross builds, on-QEMU runs, and BDD scenarios driving a QEMU target |
| `balabit/syslog-ng` | `4.8.2` | `syslog-ng-linux` and `syslog-ng-freertos` services — BDD test oracles, one per target pair. Pinned to the 4.8 LTS line; 4.11.0 (`latest` as of 2026-02-24) regressed by aborting on `STATS` over the control socket, which crashed the oracle and cascaded to the dev-container network when `freertos-target` shares the namespace. |
| `ghcr.io/cososo-ltd/behave` | `sha-be8da62` | `behave-linux` service — Debian trixie + Python 3.12 + Behave for Linux BDD scenarios. The FreeRTOS BDD runner uses the `cpputest-freertos-cross` image instead (which carries QEMU + Behave). |

## Docker Compose setup

The devcontainer uses Docker Compose (`.devcontainer/docker-compose.yml`).
VS Code connects to the `gcc` service (GCC). The `clang` service is on-demand only —
it starts when you explicitly run a command against it and stops when done.

BDD testing pairs each target with its own `syslog-ng` oracle so jobs running
in parallel (or developers switching containers) never interfere:

| Target | Behave runner | Oracle |
|---|---|---|
| Linux | `behave-linux` | `syslog-ng-linux` |
| FreeRTOS | inside `freertos-target` (image carries both QEMU and Behave) | `syslog-ng-freertos` |

The `gcc` service depends on `syslog-ng-linux` so it starts automatically with the
devcontainer; the `freertos-target` service depends on `syslog-ng-freertos` and shares
its network namespace via `network_mode: service:syslog-ng-freertos`, so QEMU's
slirp gateway `10.0.2.2` NATs to the pair's loopback where `syslog-ng-freertos` is
listening on `0.0.0.0:5514`. Both oracles also alias as the bare hostname `syslog-ng`
on their network so the existing BDD target wiring (`Bdd/Targets/Linux/BddTarget*Config.c`,
the BDD step helpers) keeps resolving without per-target host overrides. The
pairs never run together, so the alias collision is academic.

As more cross-compilation targets are added, each gets its own oracle pair in the
same shape (`syslog-ng-<target>` + a runner service or in-container Behave).

## FreeRTOS networking backend selection

`cpputest-freertos` and `cpputest-freertos-cross` both ship the
FreeRTOS-Plus-TCP and lwIP source trees side-by-side. The library picks
between them at CMake time via `SOLIDSYSLOG_FREERTOS_NET`:

| Value | Behaviour |
|---|---|
| `PLUSTCP` (default) | Cross-builds the full FreeRTOS-Plus-TCP BDD target (`Bdd/Targets/FreeRtos/`). Current first-class backend. |
| `LWIP` | Cross-builds the `Bdd/Targets/FreeRtosLwip/` link-probe — a stub proving `Platform/LwipRaw/` links against lwIP core for FreeRTOS/ARM with no PlusTcp dependency. The worked netif + QEMU UDP BDD integration is not yet wired. |
| `BOTH` | Cross-builds the `PLUSTCP` target only and emits a `STATUS` message noting the lwIP backend builds in isolation under `LWIP`. Lets dev-container users keep `BOTH` set without breaking their build. |

CI runs `PLUSTCP` (required) and `LWIP` (advisory `build-freertos-target-lwip`
lane) in isolation; the `BOTH` value is a dev-container convenience and
is not exercised by CI.

## Running the clang build locally

From a host terminal (not inside the devcontainer):

```bash
docker compose -f .devcontainer/docker-compose.yml run --rm clang \
    cmake --preset clang-debug

docker compose -f .devcontainer/docker-compose.yml run --rm clang \
    cmake --build --preset clang-debug --target junit
```

## Updating an image

When a new image tag is available:

1. Build and push the new image in the container image repo
2. Update the SHA tag in all files that reference it (see table below), plus `docs/containers.md`
3. Rebuild the devcontainer (`Ctrl+Shift+P` → "Dev Containers: Rebuild Container") and verify locally
4. Raise a PR — use `chore: bump container image to <sha>` as the title

| Image | Files to update |
|---|---|
| `cpputest` | `.devcontainer/docker-compose.yml`, `.github/workflows/ci.yml`, `docs/containers.md` |
| `cpputest-clang` | `.devcontainer/docker-compose.yml`, `.github/workflows/ci.yml`, `docs/containers.md` |
| `cpputest-freertos` | `.devcontainer/docker-compose.yml`, `.github/workflows/ci.yml`, `docs/containers.md` |
| `cpputest-freertos-cross` | `.devcontainer/docker-compose.yml`, `.github/workflows/ci.yml`, `ci/docker-compose.bdd.yml`, `docs/containers.md` |
| `behave` | `.devcontainer/docker-compose.yml`, `ci/docker-compose.bdd.yml`, `docs/bdd.md`, `docs/containers.md` |

The `cpputest-freertos` and `cpputest-freertos-cross` images both come from
[CppUTestFreertosDocker](https://github.com/cososo-ltd/CppUTestFreertosDocker).
A single push to that repo's `main` rebuilds and publishes both images at
the same `sha-<short>` tag — always update both rows together.

All references to a given image must use the same tag. Never update one without the others.

## Switching to a different container as the devcontainer

The available services and the build preset each one drives:

| Service | Use case | `BUILD_PRESET` |
|---|---|---|
| `gcc` | Primary C/C++ development (default) | `debug` |
| `clang` | Clang-specific debugging / portability | `clang-debug` |
| `freertos-host` | TDD of FreeRTOS adapters against host-side fakes | `debug` |
| `freertos-target` | Cross builds, on-QEMU runs, GDB attach (Cortex-M3, mps2-an385), BDD against the QEMU target | `freertos-cross` |
| `behave-linux` | Linux BDD scenario development (Python + Behave) | (none — cmake skipped) |

To switch:

1. In `.devcontainer/devcontainer.json`, change `"service": "gcc"` to the target service name (e.g. `"freertos-target"`).
2. `Ctrl+Shift+P` → "Dev Containers: Rebuild Container".
3. Work normally — `Ctrl+Shift+B` and all other tasks pick up the right preset via `$BUILD_PRESET`.

When done, revert `"service"` back to `"gcc"` and rebuild again.

The same VS Code keys work across every service:

- `Ctrl+Shift+B` runs the `build and test` task, which adapts to the
  active `BUILD_PRESET`. Under `freertos-cross` it builds the hello-world
  ELF; under `debug` / `clang-debug` it builds and runs `SolidSyslogTests`;
  with `BUILD_PRESET` empty (the `behave` service) it runs `behave`.
- `F5` debugs:
  - `Debug SolidSyslogTests (host)` — works in `gcc`, `clang`, and
    `freertos-host` (path resolves via `${env:BUILD_PRESET}`). Builds first
    via the same `build and test` task and stops at `main`.
  - `Debug FreeRTOS BDD Target (QEMU)` — works in `freertos-target`
    (cortex-debug + arm-none-eabi-gdb + qemu-system-arm). Stops at `main`
    via `runToEntryPoint`.
  - After switching the devcontainer service, pick the matching config from
    the Run-and-Debug dropdown once. VS Code remembers the last-picked
    config per workspace (not per container), so the previous choice
    survives a container rebuild — a stale selection will fail with the
    wrong debugger type.
- `Ctrl+Shift+P` → "Tasks: Run Task" → `run on QEMU (FreeRTOS)` — one-shot
  QEMU run for sanity-checking the build, output to the integrated
  terminal. Use only in the `freertos-target` service.

For the FreeRTOS BDD target, see [Bdd/Targets/FreeRtos/README.md](../Bdd/Targets/FreeRtos/README.md) for build / run / GDB-attach instructions.
