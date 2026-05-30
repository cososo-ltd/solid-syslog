# FreeRTOS + lwIP (Raw API) BDD target

`SolidSyslogBddTargetLwip` is a runnable FreeRTOS-on-lwIP BDD target for QEMU
`mps2-an385` (Cortex-M3). It proves the `Platform/LwipRaw/` adapter tree
(`Address`, `Resolver`, `Datagram`, `TcpStream`, `Marshal`) end-to-end against
lwIP core under `NO_SYS=0`, **with zero dependency on `Platform/PlusTcp/` or
FreeRTOS-Plus-TCP** (S28.09, epic E28). It is the worked counterpart to the
FreeRTOS-Plus-TCP target in [`../FreeRtos/`](../FreeRtos/) — same interactive /
service-task shape, Common code, startup, linker, and CMSDK UART, with the
network backend swapped PlusTcp → LwipRaw.

It is selected by `SOLIDSYSLOG_FREERTOS_NET=LWIP` (see the top-level
`CMakeLists.txt`), built by the `freertos-cross-lwip` preset, and run on QEMU by
the advisory `bdd-freertos-qemu-lwip` CI lane against the shared syslog-ng
oracle.

## What it does

- `main.c`: installs the `tcpip_callback` marshal
  (`SolidSyslogLwipRaw_SetMarshal`, S28.06), calls `tcpip_init`, then brings the
  netif up on the tcpip thread (`netif_add` + static IP `10.0.2.15` + link-up).
  Each `send N` over the UART emits N RFC 5424 datagrams to
  `{10.0.2.2, 5514}` via `SolidSyslogUdpSender` over `SolidSyslogLwipRawDatagram`.
- `netif/EthernetIf.c`: a hand-written lwIP `netif` driver over the vendored Arm
  LAN9118 (SMSC9220) low-level driver. RX is driven by the IRQ-13 `EthernetISR`
  through a task notification; TX sends pbufs via `smsc9220_send_by_chunks`.
- `netif/smsc9220/smsc9220_eth_drv.{c,h}`, `netif/smsc9220/smsc9220_emac_config.h`:
  the Arm low-level driver, vendored from FreeRTOS-Plus-TCP's MPS2_AN385 network
  interface (Apache-2.0; copyright and license headers preserved). Kept in its own
  `smsc9220/` subdirectory with a `DisableFormat` `.clang-format` so `analyze-format`
  leaves the third-party source untouched. Our snapshot predates upstream's
  `-Wconversion` cleanup (PR #1245) and carries three known defects that are benign
  on QEMU but would bite on real silicon — see
  [`netif/smsc9220/KNOWN-LIMITATIONS.md`](netif/smsc9220/KNOWN-LIMITATIONS.md) for the
  provenance snapshot and the defect/fix details.

## Scope

UDP (S28.09) + TCP (S28.10). The `SwitchingSender` carries a real UDP sender
(`SolidSyslogLwipRawDatagram`) and a real RFC 6587 octet-framed TCP sender
(`SolidSyslogStreamSender` over `SolidSyslogLwipRawTcpStream`); the TLS slot
stays wired to the shared `NullSender` so `set transport tls` resolves cleanly
(drops on the floor) until S28.11 wires the LwipRaw TLS sender. `lwipopts.h`
runs `NO_SYS=0` (tcpip thread + the contrib FreeRTOS `sys_arch`);
`FreeRTOSConfig.h` mirrors the +TCP networking config.

## Build

```sh
cmake --preset freertos-cross-lwip
cmake --build --preset freertos-cross-lwip --target SolidSyslogBddTargetLwip
```

Requires the `cpputest-freertos-cross` container (arm-none-eabi toolchain,
`FREERTOS_KERNEL_PATH=/opt/freertos/kernel`, `LWIP_PATH=/opt/lwip`).

## Run on QEMU

```sh
docker compose -f ci/docker-compose.bdd.yml up \
  --abort-on-container-exit --exit-code-from behave-freertos-lwip \
  behave-freertos-lwip syslog-ng-freertos-lwip
```
