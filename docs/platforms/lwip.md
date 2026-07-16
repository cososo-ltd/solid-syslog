# lwIP (Raw API)

`Platform/LwipRaw/` wraps the Raw API of lwIP
([lwIP documentation](https://www.nongnu.org/lwip/2_1_x/index.html)). Its `.c`
files compile against your `lwipopts.h`, so the adapter inherits your stack's
configuration.

Fills the Resolver, Datagram and Stream [roles](../roles/index.md), plus the
address handle they share. Layer [Mbed TLS](mbedtls.md) over the TCP stream for
TLS.

## What it ships

| Class | Role |
|---|---|
| [`SolidSyslogLwipRawAddress`](../api/SolidSyslogLwipRawAddress_8h.md) | destination handle |
| [`SolidSyslogLwipRawResolver`](../api/SolidSyslogLwipRawResolver_8h.md) | numeric IPv4 resolver |
| [`SolidSyslogLwipRawDnsResolver`](../api/SolidSyslogLwipRawDnsResolver_8h.md) | DNS resolver (`LWIP_DNS=1`) |
| [`SolidSyslogLwipRawDatagram`](../api/SolidSyslogLwipRawDatagram_8h.md) | UDP sender |
| [`SolidSyslogLwipRawTcpStream`](../api/SolidSyslogLwipRawTcpStream_8h.md) | TCP byte transport |

The source calls lwIP only — no direct OS calls. The TCP stream's synchronous
Open needs a bounded sleep, injected as a `SolidSyslogSleepFunction`.

## The marshal

Not a role: [`SolidSyslogLwipRaw_SetMarshal`](../api/SolidSyslogLwipRawMarshal_8h.md)
is a process-global seam, not a component you wire into the config. Every lwIP call
the Datagram and TcpStream make is routed through one marshal hop.

- `NO_SYS=1` — bare metal, one execution context. Do nothing; the default
  direct-call marshal is correct.
- `NO_SYS=0` — an RTOS tcpip thread
  ([lwIP multithreading documentation](https://www.nongnu.org/lwip/2_1_x/multithreading.html)).
  Call `SolidSyslogLwipRaw_SetMarshal(fn)` once at boot, before creating any
  adapter, passing a function that runs its callback on the core-owning thread.

The marshal must invoke its callback synchronously — the adapter reads results the
moment the hop returns. `tcpip_callback_with_block(…, 1)` or a `LOCK_TCPIP_CORE` /
`UNLOCK_TCPIP_CORE` pair satisfy that; a bare `tcpip_callback` does not. Worked
example: [`Bdd/Targets/FreeRtosLwip/main.c`](../../Bdd/Targets/FreeRtosLwip/main.c).

## Requirements

Your `lwipopts.h` must enable the features the adapter wraps:

| Setting | For |
|---|---|
| `LWIP_RAW=1` | the Raw API |
| `LWIP_UDP=1` | the UDP datagram |
| `LWIP_TCP=1` | the TCP stream |
| `LWIP_DNS=1` | the DNS resolver only |

Also set `ARP_QUEUEING=1` (else the first datagram to an unresolved peer is
dropped) and `LWIP_TCP_KEEPALIVE=1`, and size `PBUF_POOL_SIZE` /
`MEMP_NUM_TCP_PCB` / `MEMP_NUM_UDP_PCB` to your instance counts.

Full setup — config, marshal, DNS — is [Integrating lwIP](../integrating-lwip.md).
