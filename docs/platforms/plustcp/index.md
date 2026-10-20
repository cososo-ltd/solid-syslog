# FreeRTOS-Plus-TCP

`Platform/PlusTcp/` wraps FreeRTOS-Plus-TCP for networking on FreeRTOS targets
([FreeRTOS-Plus-TCP documentation](https://www.freertos.org/Documentation/03-Libraries/02-FreeRTOS-plus/02-FreeRTOS-plus-TCP/01-FreeRTOS-Plus-TCP)).

Fills the Resolver, Datagram and Stream [roles](../../roles/index.md), plus the
address handle they share.

## What it ships

## Requirements

FreeRTOS-Plus-TCP, selected at CMake time by
naming `PlusTcp` in `SOLIDSYSLOG_PLATFORMS`. The resolver wraps `FreeRTOS_getaddrinfo`, so
your `FreeRTOSIPConfig.h` needs `ipconfigUSE_DNS=1`.

## Security behaviour and obligations

### The transport carries syslog in clear

Neither the datagram nor the TCP stream provides confidentiality, integrity or
peer authentication. TLS is a separate role filled by a different platform - the
[platform × capability matrix](../index.md) shows which - layered over this
stream rather than replacing it.

### Resolution is trusted as the stack returns it

The resolver forwards what FreeRTOS-Plus-TCP answers. A deployment that cannot
trust its DNS should give the collector a numeric address rather than a name, so
that no resolution step exists to be poisoned.

### A record waits for its next hop to resolve

FreeRTOS-Plus-TCP does not queue a datagram while it resolves the next hop's
link address: `FreeRTOS_sendto` accepts it, and the stack drops it to send the
ARP request in its place. A burst sent before the reply arrives, such as the
records logged at start-up, would be lost with nothing reported. So before
each send the datagram asks the stack's ARP cache for the next hop, as the
[Datagram](../../api/structSolidSyslogDatagram.md) contract requires. A cached
entry answers at once. Otherwise the datagram sends one ARP request and checks
for the reply every tick, for up to `SOLIDSYSLOG_DATAGRAM_RESOLVE_WAIT_MS`
(100 ms by default, and never less than one tick), so it sends within a tick
of the reply arriving. If no reply arrives it fails the send; with a store the
record is kept for the next pass. The request is sent once per send and the
stack does not repeat it, so one lost during the wait goes again with the next
send.

The wait falls on the task that calls `SolidSyslog_Service`, or on the logging
task with an inline wiring. It applies to the first record to a next hop, and
again once its cache entry ages out under `ipconfigMAX_ARP_AGE`, not to
steady-state traffic; budget for it if you are logging inline from a task with
a deadline. A resolution that fails is reported once, as
`SOLIDSYSLOG_CAT_DATAGRAM_NEXT_HOP_UNRESOLVED` followed by the stack's lookup
result as `SOLIDSYSLOG_CAT_NATIVE_ERROR`: `eARPCacheMiss` when the cache has
no entry for the next hop, `eCantSendPacket` when it has one still waiting for
its reply, or when the stack cannot send to that next hop at all, as for a
destination off the subnet with no gateway configured. Until a resolution
succeeds again, sends ask for the next hop but do not wait and are not
reported again, so an unreachable collector costs one wait rather than one
per record.

The next hop is what the stack names: the collector when it is on the subnet,
otherwise the gateway. The supported shape for UDP is a collector or relay on
the target's own subnet, since a datagram that confirms its gateway's entry
says nothing about the hops beyond it. A collector further away is a case for
TCP: a connection waits for resolution and retransmits its opening, so the
first records after boot are held by the stream rather than dropped by the
stack.

The TCP stream does not confirm the next hop. On a cache miss its connect
sends an ARP request and yields once before the SYN, for 50 ms rounded down to
the resolution your `configTICK_RATE_HZ` gives; below 20 Hz that rounds to
zero ticks, and there is no yield. A connection that does not open within its
bound fails the send,
which is reported, rather than losing records silently; the sender connects
again on its next pass.

### An over-large datagram is delivered truncated

The adapter cannot tell an over-large datagram from any other send failure,
which the [Datagram](../../api/structSolidSyslogDatagram.md) contract permits,
and `MaxPayload` answers the limit the integrator states through the datagram
config's payload-limit callback, or `SOLIDSYSLOG_UDP_IPV4_PAYLOAD_LIMIT` where
none is stated, because the stack exposes no path MTU.

A record above that size is offered to `FreeRTOS_sendto` whole. If the stack
rejects it, the sender recognises a record that could not have fitted, trims it
on a UTF-8 codepoint boundary and sends again, so the record is delivered
short rather than held. Truncation is visible to the collector; the alternative
is silence.

No record reaches that size at the default `SOLIDSYSLOG_MAX_MESSAGE_SIZE`. It
applies where the tunable has been raised past what the datagram reports -
noting that the tunable is library-wide rather than per-transport, so a value
chosen for this path applies to every transport the instance uses.

### Dead-peer detection is the stack's, not ours

Keepalive is `ipconfigTCP_KEEP_ALIVE` in your `FreeRTOSIPConfig.h`, off unless
you enable it, with `ipconfigTCP_KEEP_ALIVE_INTERVAL` setting the idle period in
seconds for every TCP socket in your system. There is no per-socket control, and
the spacing of repeat probes and the number tolerated before the connection is
closed are fixed in the stack. The `SOLIDSYSLOG_TCP_KEEPALIVE_*` tunables are
therefore not read here. Enable it, and set the interval to the default of
`SOLIDSYSLOG_TCP_KEEPALIVE_IDLE_SECONDS` unless you have reason to choose
otherwise.

### The stack's configuration is yours

Buffer counts, socket limits and timer behaviour are set in your
`FreeRTOSIPConfig.h`. Sizing them for the number of adapter instances you create
is part of the integration, and exhaustion surfaces as a failure to send rather
than as a crash.
