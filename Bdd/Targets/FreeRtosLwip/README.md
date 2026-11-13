# FreeRTOS + lwIP (Raw API) BDD target

`SolidSyslogBddTargetLwip` is a runnable FreeRTOS-on-lwIP BDD target. It proves the `Platform/LwipRaw/` adapter tree
(`Address`, `DnsResolver`, `Datagram`, `TcpStream`, `Marshal`) end-to-end against
lwIP core under `NO_SYS=0`, **with zero dependency on `Platform/PlusTcp/` or
FreeRTOS-Plus-TCP** (S28.09, epic E28). It is the worked counterpart to the
FreeRTOS-Plus-TCP target in [`../FreeRtos/`](../FreeRtos/) — same interactive /
service-task shape, Common code and board, with the network backend swapped
PlusTcp → LwipRaw.

It is selected by `SOLIDSYSLOG_BDD_TARGET=FREERTOS_LWIP` (see the top-level
`CMakeLists.txt`), built by the `freertos-cross-lwip` preset, and run on QEMU by
the `bdd-freertos-lwipraw-mbedtls-fatfs-syslogng` CI lane against the shared syslog-ng
oracle.

## What it does

- `main.c`: installs the `tcpip_callback` marshal
  (`SolidSyslogLwipRaw_SetMarshal`, S28.06), calls `tcpip_init`, then brings the
  netif up on the tcpip thread (`netif_add` + a static IP).
  Each `send N` over the console emits N RFC 5424 datagrams to the oracle —
  addressed **by name** (`syslog-ng`, resolved by
  `SolidSyslogLwipRawDnsResolver`; see [Name resolution](#name-resolution)) on
  port `5514` via `SolidSyslogUdpSender` over `SolidSyslogLwipRawDatagram`.
- The lwIP `netif` is the board's, behind `BddTargetBoardNetif.h`, and so is
  its link: see [`../Boards/Mps2An385/`](../Boards/Mps2An385/README.md) for the
  one CI runs on, and [`../Boards/NucleoF767zi/`](../Boards/NucleoF767zi/README.md)
  for the one run by hand.

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
maps that name statically to the address the board reaches the oracle at; a
hostlist hit returns synchronously,
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
> See [lwIP setup — DNS](../../../docs/platforms/lwipraw/setup.md#dns).

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
