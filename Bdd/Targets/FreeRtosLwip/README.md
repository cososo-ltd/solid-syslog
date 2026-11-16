# FreeRTOS + lwIP (Raw API) BDD target

`SolidSyslogBddTargetLwip` is the BDD target for the `LwipRaw` pack: its
`Address`, `DnsResolver`, `Datagram`, `TcpStream` and `Marshal`, against lwIP
core under `NO_SYS=0`, with no dependency on FreeRTOS-Plus-TCP. Its store is
on `FatFs`. It has the same interactive and Service tasks, common code and
board as the FreeRTOS-Plus-TCP target in
[`../FreeRtos/`](../FreeRtos/README.md), which describes the console commands.

`SOLIDSYSLOG_BDD_TARGET=FREERTOS_LWIP` selects it and the `freertos-cross-lwip`
preset builds it. The `bdd-freertos-lwipraw-mbedtls-fatfs-syslogng` CI lane
runs it under QEMU against its own syslog-ng oracle.
[`docs/builds.md`](../../../docs/builds.md) has the build commands. Behave
runs against it with:

```sh
docker compose -f ci/docker-compose.bdd.yml up \
  --abort-on-container-exit --exit-code-from behave-freertos-lwip \
  behave-freertos-lwip syslog-ng-freertos-lwip
```

## What it does

`main.c` installs the `tcpip_callback` marshal with
`SolidSyslogLwipRaw_SetMarshal`, calls `tcpip_init`, then brings the netif up
on the tcpip thread with a static address. `lwipopts.h` sets `NO_SYS=0`: lwIP
runs its own tcpip thread over the contrib FreeRTOS `sys_arch`.

The switching sender has three senders: UDP over `SolidSyslogLwipRawDatagram`,
RFC 6587 octet-counted TCP through `SolidSyslogStreamSender` over
`SolidSyslogLwipRawTcpStream`, and TLS through `SolidSyslogMbedTlsStream` over
a second `SolidSyslogLwipRawTcpStream`. TLS and mutual TLS share the third
sender and differ in the port they connect to.

The lwIP netif and its link are the board's, behind `BddTargetBoardNetif.h`:
see [`../Boards/Mps2An385/`](../Boards/Mps2An385/README.md) for the board CI
runs on, and [`../Boards/NucleoF767zi/`](../Boards/NucleoF767zi/README.md) for
the one run by hand.

## Name resolution

The oracle is addressed by name, `syslog-ng`, through
`SolidSyslogLwipRawDnsResolver`. lwIP's `DNS_LOCAL_HOSTLIST` in `lwipopts.h`
maps that name to the address at which the board reaches the oracle. A
hostlist hit returns synchronously, so the resolve completes on the device
with no DNS server.

This target therefore exercises only the resolver's synchronous branch. Under
QEMU, slirp's DNS forwarder resolves the `syslog-ng` docker alias to a
docker-bridge address the guest has no route to, so a query over the wire
cannot return a usable answer. The asynchronous, over-the-wire and timeout
branches are unit-tested in `Tests/Lwip/SolidSyslogLwipRawDnsResolverTest.cpp`.
See [lwIP setup](../../../docs/platforms/lwipraw/setup.md#resolving-by-name).
