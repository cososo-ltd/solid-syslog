# FreeRTOS + lwIP (Raw API) BDD target

`SolidSyslogBddTargetLwip` is a runnable FreeRTOS-on-lwIP BDD target for QEMU
`mps2-an385` (Cortex-M3). It proves the `Platform/LwipRaw/` adapter tree
(`Address`, `DnsResolver`, `Datagram`, `TcpStream`, `Marshal`) end-to-end against
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
  Each `send N` over the UART emits N RFC 5424 datagrams to the oracle —
  addressed **by name** (`syslog-ng`, resolved to `10.0.2.2` by
  `SolidSyslogLwipRawDnsResolver`; see [Name resolution](#name-resolution)) on
  port `5514` via `SolidSyslogUdpSender` over `SolidSyslogLwipRawDatagram`.
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

UDP (S28.09) + TCP (S28.10) + TLS/mTLS (S28.11) + DNS-by-name (S28.08). The
`SwitchingSender` carries a real UDP sender (`SolidSyslogLwipRawDatagram`), a
real RFC 6587 octet-framed TCP sender (`SolidSyslogStreamSender` over
`SolidSyslogLwipRawTcpStream`), and a real TLS sender (`SolidSyslogMbedTlsStream`
over a second `SolidSyslogLwipRawTcpStream`); `tls` and `mtls` share the one TLS
slot, dispatched by destination port at Connect time. `lwipopts.h` runs
`NO_SYS=0` (tcpip thread + the contrib FreeRTOS `sys_arch`); `FreeRTOSConfig.h`
mirrors the +TCP networking config.

## Name resolution

The oracle is addressed **by name** (`syslog-ng`) via
`SolidSyslogLwipRawDnsResolver`. lwIP's `DNS_LOCAL_HOSTLIST` (see `lwipopts.h`)
maps that name statically to `10.0.2.2`; a hostlist hit returns synchronously,
so the resolve completes on-device without a DNS server.

> **Known limitation — over-the-wire DNS is not exercised here.** This target
> drives only the resolver's *synchronous local-hostlist* branch. The QEMU slirp
>
> - docker topology cannot hand the guest a reachable address for the `syslog-ng`
> docker alias over real DNS — slirp's forwarder (`10.0.2.3`) resolves it to a
> docker-bridge IP the guest has no route to; only `10.0.2.2` (slirp NAT → the
> shared-namespace host loopback) reaches the oracle. The async / over-the-wire /
> timeout branches are unit-tested in
> `Tests/Lwip/SolidSyslogLwipRawDnsResolverTest`, consistent with the project's
> integration-over-BDD stance for paths the harness can't realistically drive.
> See [`docs/integrating-lwip.md`](../../../docs/integrating-lwip.md#dns).

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
