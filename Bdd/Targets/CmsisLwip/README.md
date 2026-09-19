# CMSIS-RTOS2 + lwIP BDD target

`SolidSyslogBddTargetCmsis` is a runnable BDD target for QEMU `mps2-an385`
(Cortex-M3), and the home for the three platform packs E40, E36 and E35 add.

It exists so each of those packs can be swapped in one at a time against a lane
that is already green, which is what makes a failure attributable to the one
thing that changed. The two existing QEMU targets keep their own lanes and their
own coverage; nothing is displaced.

It is selected by `SOLIDSYSLOG_BDD_TARGET=CMSIS_LWIP` (see the top-level
`CMakeLists.txt`), built by the `cmsis-cross-lwip` preset, and run on QEMU by the
`bdd-cmsis-qemu-lwip` CI lane against its own syslog-ng oracle.

## It is named for where it lands, not for what it links

As delivered by S40.01 this target links the **proven** stack - the `FreeRtos` OS
pack, `LwipRaw`, `MbedTls` and `FatFs` - and is a clone of the sibling lwIP
target in [`../FreeRtosLwip/`](../FreeRtosLwip/). No new platform code is in it,
which is the point: the lane passes from the first commit, so every later failure
belongs to the pack that just changed.

The swaps that give it its name:

| Story | Swap |
|---|---|
| S40.04 | OS pack `FreeRtos` -> `CmsisRtos`, over a CMSIS-RTOS2 layer on the same kernel |
| S36.03 | File pack `FatFs` -> `LittleFs`, over the shared semihosting disk |
| S35.03 | Network pack `LwipRaw` -> `LwipSocket`, with `LWIP_SOCKET=1` |

Until S40.04 lands, the name is a statement of intent. The `CMakeLists.txt`
header says the same thing where someone reading the build will see it.

## What it shares

Nothing here is a copy of the sibling target's runtime code. The LAN9118 netif
wrapper, the vendored Arm SMSC9220 driver and lwIP's `arch/cc.h` live in
[`../Common/lwip/`](../Common/lwip/) and are compiled by both targets; the
Cortex-M3 startup, the linker script, the semihosting disk and every `BddTarget*`
source come from [`../Common/`](../Common/) and `../FreeRtos/` as they already
did.

What is genuinely this target's own is its `CMakeLists.txt`, `main.c`, and the
three configuration headers - `lwipopts.h`, `FreeRTOSConfig.h` and
`solidsyslog_user_tunables.h` - because those are what the swaps above change.

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
