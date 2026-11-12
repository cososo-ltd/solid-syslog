# Container images

## Images in use

Every reference in `.github/workflows/ci.yml`,
`.github/workflows/docs-bundle.yml`, `.github/workflows/lint-workflows.yml`,
`.devcontainer/docker-compose.yml` and `ci/docker-compose.bdd.yml` is pinned by
digest - `<repo>@sha256:...`. The tag
below is the readable handle; the digest in the files is what actually resolves.
The tag is kept alongside each reference: as a trailing comment on a `container:`
or Compose `image:` key, and in the comment above the step for a `docker run`
invocation, whose line continuation cannot carry one. Digests are deliberately
not repeated here, so there is one authoritative copy per reference and nothing
to drift.

For the same reason the table says what each image is for rather than listing
what uses it. Those files already state that executably, and a prose copy
of the answer can only fall behind them - which it twice did, once when the lwIP
lanes arrived and once when the consumer-smoke ones did. Grep the image name to
find its consumers.

| Image | Tag | What it is for |
|---|---|---|
| `ghcr.io/cososo-ltd/cpputest` | `sha-9a3a71e` | The GCC base: CppUTest, CMake and the analysis tooling for host builds, tests and the static-analysis lanes. Ships `include-what-you-use 0.23 (clang_19)` matching the `cpputest-clang` build, so an `iwyu` run behaves the same in either. Carries wolfSSL twice as static installs, `/opt/wolfssl` (`WOLFSSL_DIR`) with the options the wolfSSL pack asks for and `/opt/wolfssl-baseline` (`WOLFSSL_BASELINE_DIR`) from a bare `./configure`, so the pack is proved against the build an integrator has as well as the one it prefers |
| `ghcr.io/cososo-ltd/cpputest-clang` | `sha-5905aea` | The Clang counterpart to the base, for the portability build and the IWYU lane |
| `ghcr.io/cososo-ltd/cpputest-freertos` | `sha-0bc855f` | Host-TDD of the embedded adapters against fakes, and the integration suites that need a real upstream. Adds the FreeRTOS-Kernel, Plus-TCP, Plus-FAT, lwIP, FatFs, LittleFS, Mbed TLS, CMSIS_6 and CMSIS-FreeRTOS sources and the wolfSSL source tree, nothing of it built, and inherits IWYU and the built wolfSSL host libraries from the `cpputest` base, so it builds every pack a Linux host can |
| `ghcr.io/cososo-ltd/cpputest-freertos-cross` | `sha-0bc855f` | Cross-building the QEMU BDD targets and running them: everything the host image carries, plus `gcc-arm-none-eabi`, `libnewlib-arm-none-eabi`, `gdb-multiarch` (aliased as `arm-none-eabi-gdb`), `qemu-system-arm`, and `python3` + `behave` so one image both builds a target and drives it. Upstream sources sit at `/opt/freertos/kernel`, `/opt/freertos/plus-tcp`, `/opt/freertos/plus-fat`, `/opt/lwip`, `/opt/fatfs`, `/opt/littlefs`, `/opt/mbedtls`, `/opt/wolfssl-src`, `/opt/cmsis` and `/opt/cmsis-freertos` |
| `balabit/syslog-ng` | `4.8.2` | The BDD test oracle, one instance per target so lanes cannot contend. Pinned to the 4.8 LTS line; 4.11.0 (`latest` as of 2026-02-24) regressed by aborting on `STATS` over the control socket, which crashed the oracle and cascaded to the dev-container network when a target shares the namespace |
| `ghcr.io/cososo-ltd/behave` | `sha-be8da62` | Driving BDD scenarios against a host-built example: Debian trixie, Python 3.12 and Behave. A QEMU target is driven from `cpputest-freertos-cross` instead, which carries Behave alongside the emulator |
| `davidanson/markdownlint-cli2` | `v0.22.1` | Linting Markdown, in the `analyze-markdown` lane and in the pre-push command `docs/local-checks.md` gives. Upstream, so the tag is the release rather than a source commit |
| `rhysd/actionlint` | `1.7.7` | Validating the workflow files, in the lane that has to live outside `ci.yml` to survive a break in it. Upstream, same tag form |
| `ghcr.io/cososo-ltd/mkdocs-mkdoxy` | `sha-34173c0` | Building the documentation site: Doxygen 1.9.4, mkdocs 1.6.1, mkdocs-material 9.7.6, mkdoxy 1.2.8 and mkdocs-github-admonitions-plugin 0.1.1. mkdoxy renders `Core/Interface/*.h` as native Material API pages |

## Docker Compose setup

The devcontainer uses Docker Compose (`.devcontainer/docker-compose.yml`).
VS Code connects to the `gcc` service (GCC). The `clang` service is on-demand only:
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
FreeRTOS-Plus-TCP and lwIP source trees side-by-side, and a default build
selects **both** - you can work on either networking stack, and run both
stacks' unit tests, without reconfiguring. The configure says so:

```text
-- SolidSyslog tests: both FreeRTOS networking stacks selected — lwIP and Plus-TCP suites build together.
```

To work in one stack only, deselect the other with its own switch
(`-DSOLIDSYSLOG_PLUSTCP=OFF`, or `-DSOLIDSYSLOG_LWIPRAW=OFF` with
`-DSOLIDSYSLOG_LWIPSOCKET=OFF`), which drops that platform and its tests
together. The lwIP lint lanes do exactly this. The switch holds under the default
`Auto` selection; with a named list, leave the platform out of the list instead.

Which BDD ELF a *cross* build produces is separate, and maintainer-only -
`SOLIDSYSLOG_BDD_TARGET=FREERTOS_PLUSTCP` (default), `FREERTOS_LWIP` or
`CMSIS_LWIP`. It selects a test artefact, not a platform - platforms are named
in `SOLIDSYSLOG_PLATFORMS` like everything else.

CI runs each cross target in isolation, and all four of
`build-freertos-plustcp-mbedtls-plusfat`, `build-freertos-lwipraw-mbedtls-fatfs`,
`build-cmsisrtos-lwipsocket-mbedtls-littlefs` and
`build-cmsisrtos-lwipsocket-wolfssl-littlefs` are required checks.

## Running the clang build locally

From a host terminal (not inside the devcontainer):

```bash
docker compose -f .devcontainer/docker-compose.yml run --rm clang \
    cmake --preset clang-debug

docker compose -f .devcontainer/docker-compose.yml run --rm clang \
    cmake --build --preset clang-debug --target junit
```

## Updating an image

A bump is a pull request titled `chore: bump <image> image to sha-<short>`,
naming both images when one commit published two, that moves every reference
to the image onto the digest of the new tag. Every lane runs on the new image
there, so a green run is the evidence and the maintainer merges. It is the one
step in the chain anyone takes; the image repositories merge their own bumps.
The chain that opens it unaided:

1. Renovate opens a pull request in the image repository when something its
   image pins moves: an upstream release, the base image's digest, or a
   GitHub Action. The repository's README says how it finds each pin and
   which updates it holds back
   ([CppUTestFreertosDocker](https://github.com/cososo-ltd/CppUTestFreertosDocker#keeping-the-pins-current),
   [CppUTestDocker](https://github.com/cososo-ltd/CppUTestDocker#keeping-the-pins-current)).
2. The publishing workflow builds and smokes the image on that pull request.
   Green, it merges itself; the README says what the smoke proves and what
   merges
   ([CppUTestFreertosDocker](https://github.com/cososo-ltd/CppUTestFreertosDocker#what-merges-itself),
   [CppUTestDocker](https://github.com/cososo-ltd/CppUTestDocker#what-merges-itself)).
3. The merge publishes the image at a new `sha-<short>` tag, and the workflow
   sends a `repository_dispatch` event, `image-published`, to this repository,
   naming the images that commit published and their tag. The token is minted
   from the release GitHub App, which is installed here.
4. `image-bump.yml` resolves the tag to its digest, runs
   `scripts/bump_image_digest.py` for each image named, and opens the pull
   request with the App token, because one opened with `GITHUB_TOKEN` does not
   run CI. A tag the tree is already on opens nothing. When the tag is what
   the image repository's `main` publishes, any other open bump for the same
   images is closed as superseded; a run for an older tag closes nothing.

A change to the `cpputest` base flows through the images built on it the same
way. `cpputest-freertos` names its base as `cpputest:latest` pinned by digest,
so when `cpputest` publishes, Renovate in CppUTestFreertosDocker opens the
digest bump, which merges itself and publishes both FreeRTOS images.

What still comes to the maintainer from an image repository arrives as an
assignment, which is a GitHub notification: a Renovate pull request whose
build or smoke fails, and any major, which never merges itself. A publish
that fails on `main` after a green pull request sends no event at all, so
`image-drift.yml` checks weekly: it fails when an image's pin here lags its
repository's `main` and no open bump pull request names that image at the
newer tag, using `scripts/check_image_drift.py`.

The `cpputest`, `cpputest-freertos` and `cpputest-freertos-cross` images send
the event. The others reach step 4 by hand.

### By hand

Run the **Image Bump** workflow from the Actions tab with the image names and
the tag, and it does step 4. That is the path for `cpputest-clang`, `behave`
and `mkdocs-mkdoxy`, whose repositories do not send the event, and the
fallback when a dispatch did not arrive. Or do the same locally:

1. Resolve the new tag to its digest:

   ```bash
   docker buildx imagetools inspect ghcr.io/cososo-ltd/<image>:sha-<new> \
     --format '{{.Manifest.Digest}}'
   ```

   Pin exactly what this command prints - the image index digest for a
   multi-architecture image, or the single manifest digest for a
   single-platform one. Do not substitute a per-platform digest dug out of
   `--raw`: for an index that would nail the reference to one architecture.
   Of the images here, only `balabit/syslog-ng` is genuinely multi-arch
   (`linux/amd64` and `linux/arm64`); the rest are `linux/amd64` only.

2. Move every reference, the tag comment beside each and the tag in the table
   above:

   ```bash
   python3 scripts/bump_image_digest.py ghcr.io/cososo-ltd/<image> sha-<new> <digest>
   ```

   The script walks the digest-pinned files and refuses a tree whose
   references for that image already disagree, so an uneven earlier bump is
   caught before it is compounded. It takes the image as the files reference
   it, so `balabit/syslog-ng` with its release tag works the same way.

3. Rebuild the devcontainer (`Ctrl+Shift+P` → "Dev Containers: Rebuild Container")
   and verify locally.
4. Raise the pull request under the title above.

`syslog-ng`, `markdownlint-cli2` and `actionlint` are upstream images rather than
ours, published by [balabit](https://hub.docker.com/r/balabit/syslog-ng),
[DavidAnson](https://hub.docker.com/r/davidanson/markdownlint-cli2) and
[rhysd](https://hub.docker.com/r/rhysd/actionlint). There is no publish step and
no event: the version is chosen from what upstream published and pinned by hand.
Read the 4.8 LTS pinning rationale in the first table before moving `syslog-ng`.
`docs/local-checks.md` invokes `markdownlint-cli2` by tag, deliberately: it is a
command a developer types, where a digest is unreadable and the engine version
is the thing worth stating, so a `markdownlint-cli2` bump edits that line too.

The `cpputest-freertos` and `cpputest-freertos-cross` images both come from
[CppUTestFreertosDocker](https://github.com/cososo-ltd/CppUTestFreertosDocker).
A single push to that repo's `main` rebuilds and publishes both images at
the same `sha-<short>` tag, and its event names both, so one pull request
moves both rows.

The `mkdocs-mkdoxy` image comes from
[MkdocsMkdoxyDocker](https://github.com/cososo-ltd/MkdocsMkdoxyDocker); a push to
that repo's `main` rebuilds and publishes it at a new `sha-<short>` tag.
It is a CI-only image, not a devcontainer service, so the "rebuild the
devcontainer and verify" step above does not exercise it. Verify a bump by
running the same command the `docs-build` job runs, from the repo root:

```bash
docker run --rm -v "$PWD:/docs" \
  ghcr.io/cososo-ltd/mkdocs-mkdoxy@sha256:<new> mkdocs build --strict
```

## Switching to a different container as the devcontainer

The available services and the build preset each one drives:

| Service | Use case | `BUILD_PRESET` |
|---|---|---|
| `gcc` | Primary C/C++ development (default) | `debug` |
| `clang` | Clang-specific debugging / portability | `clang-debug` |
| `freertos-host` | TDD of FreeRTOS adapters against host-side fakes | `debug` |
| `freertos-target` | Cross builds, on-QEMU runs, GDB attach (Cortex-M3, mps2-an385), BDD against the QEMU target | `freertos-cross` |
| `behave-linux` | Linux BDD scenario development (Python + Behave) | (none - cmake skipped) |

To switch:

1. In `.devcontainer/devcontainer.json`, change `"service": "gcc"` to the target service name (e.g. `"freertos-target"`).
2. `Ctrl+Shift+P` → "Dev Containers: Rebuild Container".
3. Work normally; `Ctrl+Shift+B` and all other tasks pick up the right preset via `$BUILD_PRESET`.

When done, revert `"service"` back to `"gcc"` and rebuild again.

The same VS Code keys work across every service:

- `Ctrl+Shift+B` runs the `build and test` task, which adapts to the
  active `BUILD_PRESET`. Under `freertos-cross` it builds the
  `SolidSyslogBddTarget` ELF; under `debug` / `clang-debug` it builds and runs
  `SolidSyslogTests`; with `BUILD_PRESET` empty (the `behave-linux` service) it
  runs `behave`.
- `F5` debugs:
  - `Debug SolidSyslogTests (host)`: works in `gcc`, `clang`, and
    `freertos-host` (path resolves via `${env:BUILD_PRESET}`). Builds first
    via the same `build and test` task and stops at `main`.
  - `Debug FreeRTOS BDD Target (QEMU)`: works in `freertos-target`
    (cortex-debug + arm-none-eabi-gdb + qemu-system-arm). Stops at `main`
    via `runToEntryPoint`.
  - After switching the devcontainer service, pick the matching config from
    the Run-and-Debug dropdown once. VS Code remembers the last-picked
    config per workspace (not per container), so the previous choice
    survives a container rebuild; a stale selection will fail with the
    wrong debugger type.
- `Ctrl+Shift+P` → "Tasks: Run Task" → `run on QEMU (FreeRTOS)`: one-shot
  QEMU run for sanity-checking the build, output to the integrated
  terminal. Use only in the `freertos-target` service.

For the FreeRTOS BDD target, see `Bdd/Targets/FreeRtos/README.md` for build / run / GDB-attach instructions.
