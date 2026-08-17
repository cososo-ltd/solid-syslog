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
Open and the DNS resolver's bounded wait both need a sleep, injected as a
`SolidSyslogSleepFunction`. Create it without one and you get the shared Null
object back, with nothing reported: the wiring looks like it worked and no record
is ever delivered.

Your `lwipopts.h` must enable the features the adapter wraps:

| Setting | For |
|---|---|
| `LWIP_RAW=1` | the Raw API |
| `LWIP_UDP=1` | the UDP datagram |
| `LWIP_TCP=1` | the TCP stream |
| `LWIP_DNS=1` | the DNS resolver only |

Also set `ARP_QUEUEING=1` (else the first datagram to an unresolved peer is
dropped) and `LWIP_TCP_KEEPALIVE=1`, and size `PBUF_POOL_SIZE` /
`MEMP_NUM_TCP_PCB` / `MEMP_NUM_UDP_PCB` to your instance counts. `IP_FRAG`
decides what becomes of a record too large for the path — see
[an over-large record has three possible fates](#an-over-large-record-has-three-possible-fates)
below.

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

### An over-large record has three possible fates

The datagram reports the IPv6-safe payload of 1232 bytes from `MaxPayload` and
cannot tell an over-large datagram from any other send failure, which the
[Datagram](../../api/structSolidSyslogDatagram.md) contract permits. Because the
sender only trims a record after being told it was too large, one over that size
reaches lwIP whole, and what happens next is `IP_FRAG`'s decision rather than the
adapter's:

- **`IP_FRAG=1`**, lwIP's default: lwIP attempts to fragment the datagram and
  submits the fragments to your interface. Allocating them can fail, and
  submission is not delivery — this is the case RFC 5426 §3.2 warns about, where
  a lost fragment costs the whole record and some collectors and middleboxes
  drop fragments outright.
- **`IP_FRAG=0`**: lwIP compiles the length check out of its send path
  altogether and hands the over-length packet to your driver. A driver that drops
  it and answers `ERR_OK` loses the record while the store counts it delivered; a
  driver that answers an error fails the send, and a failed send is treated as
  transient, so the store re-offers the same record on every pass and nothing
  behind it is delivered.

No record can reach that size at the default `SOLIDSYSLOG_MAX_MESSAGE_SIZE`, so
all three fates are reachable only where the tunable has been raised past the
payload the datagram reports. Until
[#736](https://github.com/cososo-ltd/solid-syslog/issues/736) lands, keep it
inside that payload — noting that it is library-wide rather than per-transport,
so a value chosen for this path applies to every transport the instance uses.

### Dead-peer detection runs at lwIP's defaults

The stream enables keepalive and leaves the timings to the stack, so a silent
peer is first probed after lwIP's default two hours and the connection is
declared dead around eleven minutes after that. Those defaults are compile-time
and stack-wide, so changing them means `TCP_KEEPIDLE_DEFAULT` and its siblings in
your `lwipopts.h` — which moves every TCP connection in your system, not only
this one. `LWIP_TCP_KEEPALIVE=1` does not alter the timings; it makes the
interval and probe count per-connection fields, which is what
[#743](https://github.com/cososo-ltd/solid-syslog/issues/743) needs to give the
library its own setting and apply it here.

This governs the idle case only. A connection actually carrying records notices
a dead peer sooner: lwIP's send buffer fills, the write fails, and the stream
closes itself so the sender reconnects.

### Resolution is trusted as the stack returns it

The DNS resolver forwards what lwIP answers. A deployment that cannot trust its
DNS should give the collector a numeric address, so that no resolution step
exists to be poisoned.

### Pool sizing is yours, and exhaustion is silent at the stack

`PBUF_POOL_SIZE`, `MEMP_NUM_TCP_PCB` and `MEMP_NUM_UDP_PCB` must cover the
instances you create alongside everything else using the stack. lwIP reports
nothing when it runs out; what you see is the send failing, which reaches you as
a delivery failure through the error handler rather than as anything naming the
pool that was exhausted.
