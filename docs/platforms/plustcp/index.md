# FreeRTOS-Plus-TCP

`Platform/PlusTcp/` wraps FreeRTOS-Plus-TCP for networking on FreeRTOS targets
([FreeRTOS-Plus-TCP documentation](https://www.freertos.org/Documentation/03-Libraries/02-FreeRTOS-plus/02-FreeRTOS-plus-TCP/01-FreeRTOS-Plus-TCP)).

Fills the Resolver, Datagram and Stream [roles](../../roles/index.md), plus the
address handle they share.

## What it ships

## Requirements

FreeRTOS-Plus-TCP, selected at CMake time with
naming `PlusTcp` in `SOLIDSYSLOG_PLATFORMS`. The resolver wraps `FreeRTOS_getaddrinfo`, so
your `FreeRTOSIPConfig.h` needs `ipconfigUSE_DNS=1`.

## Security behaviour and obligations

### The transport carries syslog in clear

Neither the datagram nor the TCP stream provides confidentiality, integrity or
peer authentication. TLS is a separate role filled by a different platform — the
[platform × capability matrix](../index.md) shows which — layered over this
stream rather than replacing it.

### Resolution is trusted as the stack returns it

The resolver forwards what FreeRTOS-Plus-TCP answers. A deployment that cannot
trust its DNS should give the collector a numeric address rather than a name, so
that no resolution step exists to be poisoned.

### A first send to an unresolved peer stalls the calling task

FreeRTOS-Plus-TCP does not queue datagrams while ARP resolves — a
`FreeRTOS_sendto` to a peer that is not in the ARP cache is dropped at the IP
layer. The datagram adapter therefore issues an ARP probe on a cache miss and
then waits, in a `vTaskDelay` of its own, so the reply can land before it sends.
The wait is the adapter's rather than the stack's, and is 50 ms rounded to the
resolution your `configTICK_RATE_HZ` gives.

That delay is paid by whichever task made the call: the application's own thread
on an inline wiring, or the servicing thread on a buffered one. It applies to the
first datagram sent to a peer, and again once its cache entry ages out under
`ipconfigMAX_ARP_AGE`, not to steady-state traffic. Budget for it if you are
logging inline from a task with a deadline.

### An over-large datagram blocks the queue

The adapter reports the IPv6-safe payload of 1232 bytes from `MaxPayload` and
cannot tell an over-large datagram from any other send failure, which the
[Datagram](../../api/structSolidSyslogDatagram.md) contract permits. The
consequence is on the caller's side, and it is not simply a dropped record.

Because the sender only trims a record after being told it was too large, one
over that size is offered to `FreeRTOS_sendto` whole. If the stack rejects it,
the send fails, and a failed send is treated as transient: the store keeps the
record at its cursor and offers the same one on every servicing pass. Nothing
behind it is delivered.

No record can reach that size at the default `SOLIDSYSLOG_MAX_MESSAGE_SIZE`, so
this is a hazard only where the tunable has been raised past the payload the
adapter reports. Until
[#736](https://github.com/cososo-ltd/solid-syslog/issues/736) lands, keep it
inside that payload — noting that it is library-wide rather than per-transport,
so a value chosen for this path applies to every transport the instance uses.

### The stack's configuration is yours

Buffer counts, socket limits and timer behaviour are set in your
`FreeRTOSIPConfig.h`. Sizing them for the number of adapter instances you create
is part of the integration, and exhaustion surfaces as a failure to send rather
than as a crash.
