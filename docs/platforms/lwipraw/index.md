# lwIP (Raw API)

`Platform/LwipRaw/` wraps the Raw API of lwIP
([lwIP documentation](https://www.nongnu.org/lwip/2_1_x/index.html)). Its `.c`
files compile against your `lwipopts.h`, so the adapter inherits your stack's
configuration.

Fills the Resolver, Datagram and Stream [roles](../../roles/index.md), plus the
address handle they share. A TLS platform layers over the TCP stream; the
[platform × capability matrix](../index.md) shows which provide it.

## What it ships

## The marshal

Not a role: [`SolidSyslogLwipRaw_SetMarshal`](../../api/SolidSyslogLwipRawMarshal_8h.md)
is a process-global seam, not a component you wire into the config. Every lwIP call
the Datagram and TcpStream make is routed through one marshal hop.

- `NO_SYS=1` — bare metal, one execution context. Do nothing; the default
  direct-call marshal is correct.
- `NO_SYS=0` — an RTOS tcpip thread
  ([lwIP multithreading documentation](https://www.nongnu.org/lwip/2_1_x/multithreading.html)).
  Call `SolidSyslogLwipRaw_SetMarshal(fn)` once at boot, before creating any
  adapter, passing a function that runs its callback on the core-owning thread.

The marshal must invoke its callback synchronously — the adapter reads results the
moment the hop returns. A `LOCK_TCPIP_CORE` / `UNLOCK_TCPIP_CORE` pair satisfies
that directly. Posting to lwIP's mailbox does not, in any of its forms:
`tcpip_callback_with_block` blocks until the message is accepted, not until the
callback runs, so a mailbox marshal has to wait for completion itself.

## Requirements

The source calls lwIP only — no direct OS calls. The TCP stream's synchronous
Open needs a bounded sleep, injected as a `SolidSyslogSleepFunction`.

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

## Security behaviour and obligations

### The transport carries syslog in clear

Neither the datagram nor the TCP stream provides confidentiality, integrity or
peer authentication. TLS is a separate role filled by a different platform — the
[platform × capability matrix](../index.md) shows which — layered over this
stream rather than replacing it.

### The marshal is a correctness requirement, not a tuning knob

On a build with an lwIP thread (`NO_SYS=0`), every call this adapter makes must
reach the core-owning context, and it must do so synchronously because results
are read the moment the hop returns. An asynchronous marshal, or none at all,
corrupts lwIP's internal state rather than failing cleanly. Install it once at
boot, before any adapter is created.

### Resolution is trusted as the stack returns it

The DNS resolver forwards what lwIP answers. A deployment that cannot trust its
DNS should give the collector a numeric address, so that no resolution step
exists to be poisoned.

### Pool sizing is yours, and exhaustion is silent at the stack

`PBUF_POOL_SIZE`, `MEMP_NUM_TCP_PCB` and `MEMP_NUM_UDP_PCB` must cover the
instances you create alongside everything else using the stack. Under-sizing
shows as dropped records rather than as an error from lwIP.
