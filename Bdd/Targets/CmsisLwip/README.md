# CMSIS-RTOS2 + lwIP BDD target

`SolidSyslogBddTargetCmsis` is a runnable BDD target for QEMU `mps2-an385`
(Cortex-M3), and the home of the three platform packs E40, E36 and E35 added.

It exists so each of those packs could be swapped in one at a time against a
lane that was already green, which is what made a failure attributable to the
one thing that changed. The two existing QEMU targets keep their own lanes and
their own coverage; nothing is displaced.

It is selected by `SOLIDSYSLOG_BDD_TARGET=CMSIS_LWIP` (see the top-level
`CMakeLists.txt`), built by the `cmsis-cross-lwip` preset, and run on QEMU by the
`bdd-cmsisrtos-lwipsocket-mbedtls-littlefs-syslogng` CI lane against its own syslog-ng oracle.

## What it links

All three swaps are done, so the target now runs the stack it is named for:
`CmsisRtos`, `LittleFs` and `LwipSocket`, with `MbedTls` for TLS. Its own code
calls CMSIS-RTOS2 rather than the kernel beneath it, and the lwIP Sockets API
rather than the Raw one, so there is no marshal seam in `main.c`.

| Story | Swap |
|---|---|
| S40.04 | OS pack `FreeRtos` -> `CmsisRtos`, over a CMSIS-RTOS2 layer on the same kernel |
| S36.03 | File pack `FatFs` -> `LittleFs`, over the shared semihosting disk |
| S35.03 | Network pack `LwipRaw` -> `LwipSocket`, with `LWIP_SOCKET=1` |

Each swap changed one variable against a lane that was already green, so a
failure belonged to the pack that had just changed.

## The wolfSSL build

The `cmsis-cross-lwip-wolfssl` preset builds this same target over `WolfSsl` in
place of `MbedTls`, and the `bdd-cmsisrtos-lwipsocket-wolfssl-littlefs-syslogng` lane runs it. wolfSSL
is not a prebuilt library here: its sources are compiled into the image from
the tree at `WOLFSSL_SOURCE_DIR`, under [`wolfssl/user_settings.h`](wolfssl/user_settings.h),
which is how an embedded integrator builds it. That file says what each setting
is for.

What the target supplies around it:

- **Memory.** One static region of 128 KiB, in
  `BddTargetTlsSender_WolfSsl_InnerStream.c`, over wolfSSL's default buckets.
  Its heap hint is the stream's `Heap` and wolfSSL's global hint. The mutexes
  wolfSSL's CMSIS-RTOS2 port creates still come from the kernel heap.
- **Stack.** The handshake runs on the service thread and peaks near 22 KiB,
  so this build gives that thread 32 KiB where the Mbed TLS build has 8 KiB.
- **Seed.** Demo-only, as the Mbed TLS build's is, and announced at boot.
- **Clock.** The build instant advanced by uptime, so certificate validity is
  enforced with no RTC.
- **Credentials.** The same baked PEMs, handed to wolfSSL as buffers.

The lane runs the other QEMU lanes' tag filter.

```bash
cmake --preset cmsis-cross-lwip-wolfssl
cmake --build --preset cmsis-cross-lwip-wolfssl --target SolidSyslogBddTargetCmsis
docker compose -f ci/docker-compose.bdd.yml \
  up --abort-on-container-exit --exit-code-from behave-cmsis-lwip-wolfssl \
  behave-cmsis-lwip-wolfssl syslog-ng-cmsis-lwip-wolfssl
```

## Where the kernel lives

Everything tied to the particular kernel underneath - its sources, its
`FreeRTOSConfig.h`, its application hooks and the CMSIS-RTOS2 wrapper - is in
[`Kernel/FreeRtos/`](Kernel/FreeRtos/). A second kernel is a sibling directory
supplying the same `Kernel.cmake`, configuration, hooks and CMSIS device
header, and a one-line change to which `Kernel.cmake` the target includes.

Two things outside that directory still call the kernel directly, because they
sit beneath the wrapper and no CMSIS-RTOS2 port of them exists: lwIP's
`sys_arch` and the LAN9118 netif. They are what a genuinely different kernel
would cost.

## What it shares

Nothing here is a copy of the sibling target's runtime code. The LAN9118 netif
wrapper, the vendored Arm SMSC9220 driver and lwIP's `arch/cc.h` live in
[`../Common/lwip/`](../Common/lwip/) and are compiled by both targets; the
Cortex-M3 startup, the linker script, the semihosting disk and every `BddTarget*`
source come from [`../Common/`](../Common/) and `../FreeRtos/` as they already
did.

What is genuinely this target's own is its `CMakeLists.txt`, its `main.c`, its
`Kernel/` directory, and the configuration headers the swaps above change:
`lwipopts.h`, `mbedtls_user_config.h` and `solidsyslog_user_tunables.h`.

## Running it

```bash
cmake --preset cmsis-cross-lwip
cmake --build --preset cmsis-cross-lwip --target SolidSyslogBddTargetCmsis
```

Under Behave against the oracle, from the repository root:

```bash
docker compose -f ci/docker-compose.bdd.yml \
  up --abort-on-container-exit --exit-code-from behave-cmsis-lwip \
  behave-cmsis-lwip syslog-ng-cmsis-lwip
```

The lane runs the same tag filter as the two existing QEMU lanes, so it exercises
UDP, TCP, TLS and mutual TLS, and the `@store` suite that rides the `@tcp` tag.
