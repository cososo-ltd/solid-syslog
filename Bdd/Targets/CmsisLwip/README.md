# CMSIS-RTOS2 + lwIP BDD target

`SolidSyslogBddTargetCmsis` is the BDD target for the `CmsisRtos`, `LwipSocket`
and `LittleFs` packs, with `MbedTls` for TLS. Its own code calls CMSIS-RTOS2
and not the kernel beneath it, and the lwIP Sockets API and not the Raw one,
so `main.c` has no marshal.

`SOLIDSYSLOG_BDD_TARGET=CMSIS_LWIP` selects it and the `cmsis-cross-lwip`
preset builds it. The `bdd-cmsisrtos-lwipsocket-mbedtls-littlefs-syslogng` CI
lane runs it under QEMU against its own syslog-ng oracle, with the tag filter
of the other QEMU lanes: UDP, TCP, TLS, mutual TLS and the `@store` scenarios.

[`docs/builds.md`](../../../docs/builds.md) has the build commands, and
[`Bdd/README.md`](../../README.md#running-in-ci) the compose pair that runs
Behave against each build:

```bash
docker compose -f ci/docker-compose.bdd.yml \
  up --abort-on-container-exit --exit-code-from behave-cmsis-lwip \
  behave-cmsis-lwip syslog-ng-cmsis-lwip
```

## The wolfSSL build

The `cmsis-cross-lwip-wolfssl` preset builds this target over `WolfSsl` in
place of `MbedTls`, and the
`bdd-cmsisrtos-lwipsocket-wolfssl-littlefs-syslogng` lane runs it, on the
`behave-cmsis-lwip-wolfssl` and `syslog-ng-cmsis-lwip-wolfssl` pair. wolfSSL is
not a prebuilt library here. Its sources are compiled into the image from the
tree at `WOLFSSL_SOURCE_DIR`, under
[`wolfssl/user_settings.h`](wolfssl/user_settings.h), as an embedded integrator
would build it. That file says what each setting is for.

What the target supplies around it:

- Memory: one static region of 128 KiB, in
  `BddTargetTlsSender_WolfSsl_InnerStream.c`, over wolfSSL's default buckets.
  Its heap hint is the stream's `Heap` and wolfSSL's global hint. The mutexes
  wolfSSL's CMSIS-RTOS2 port creates still come from the kernel heap.
- Stack: the handshake runs on the service thread and peaks near 22 KiB, so
  this build gives that thread 32 KiB where the Mbed TLS build has 8 KiB.
- Seed: for test use only, as the Mbed TLS build's is, and announced at boot.
- Clock: the build time advanced by uptime, so certificate validity is
  enforced with no RTC.
- Credentials: the same built-in PEMs, handed to wolfSSL as buffers.

## The raw flash build

The `cmsis-cross-lwip-norflash` preset builds this target as
`SOLIDSYSLOG_BDD_TARGET=CMSIS_LWIP_NOR_FLASH`, over Mbed TLS. That image keeps
its store directly on a NOR flash, through the `SpiNorFlash` pack and Core's
flash block device, and has no `LittleFs` in it. The two builds differ in one
place: `CMakeLists.txt` takes `BddTargetCmsisStore_LittleFs.c` for the first
and `BddTargetCmsisStore_NorFlash.c` for the second.

The flash is the board's. The QEMU machine has none, so its board directory
[simulates the part](../Boards/Mps2An385/README.md#flash) behind the pack's
bus. The `bdd-cmsisrtos-lwipsocket-mbedtls-norflash-syslogng` lane runs the
`@store` scenarios against this build, on the `behave-cmsis-lwip-norflash` and
`syslog-ng-cmsis-lwip-norflash` pair.

## Where the kernel lives

Everything tied to the kernel underneath is in
[`Kernel/FreeRtos/`](Kernel/FreeRtos/), or named from there: its sources, its
`FreeRTOSConfig.h` and the CMSIS-RTOS2 wrapper. The kernel's application hooks,
and the part of its configuration that follows from the board, are files in
[`../Common/`](../Common/) that the other FreeRTOS targets share. A second
kernel would be a sibling directory supplying the same `Kernel.cmake`,
configuration and hooks, and a one-line change to which `Kernel.cmake` the
target includes.

lwIP's `sys_arch` and the board's netif still call the kernel directly. They
sit beneath the wrapper, and no CMSIS-RTOS2 port of them exists.

## What it shares

lwIP's `arch/cc.h` is in [`../Common/lwip/`](../Common/lwip/) and every shared
`BddTarget*` source in [`../Common/`](../Common/). The startup, the linker
script, the console, the disk and the netif belong to the board, behind the
board seam: see [`../Boards/Mps2An385/`](../Boards/Mps2An385/README.md) for
the one CI runs on.

The target's own files are its `CMakeLists.txt`, its `main.c`, its `Kernel/`
directory, the `BddTargetCmsisStore` files that say which store a build has,
and its configuration headers: `lwipopts.h`, `mbedtls_user_config.h` and
`solidsyslog_user_tunables.h`.

## On the NUCLEO-F767ZI

The `nucleo-f767zi-cmsis-lwip` preset builds this target for ST's
NUCLEO-F767ZI, over Mbed TLS. Nothing in this directory changes for it: the
board supplies its own half of the seam, and the kernel configuration takes the
core clock and the interrupt priority bits from the board.

The `nucleo-f767zi-cmsis-lwip-norflash` preset builds
[the raw flash build](#the-raw-flash-build) for the same board, with its store
directly on the board's NOR flash.

CI builds both images and runs neither. The board's
[README](../Boards/NucleoF767zi/README.md) has the build, the run and the
target checklist.
