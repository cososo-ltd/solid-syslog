# lwIP (Sockets API)

`Platform/LwipSocket/` wraps the Sockets API of lwIP
([lwIP documentation](https://www.nongnu.org/lwip/2_1_x/index.html)). Its `.c`
files compile against your `lwipopts.h`, so the adapter inherits your stack's
configuration.

Fills the Datagram, Stream and Resolver [roles](../../roles/index.md), plus the
address handle a transport reads back to send. The
[platform x capability matrix](../index.md) shows which platform fills what.

## What it ships

| Class | Fills |
|---|---|
| `SolidSyslogLwipSocketAddress` | the resolved destination a transport sends to |
| `SolidSyslogLwipSocketResolver` | Resolver, over `lwip_getaddrinfo` |
| `SolidSyslogLwipSocketDatagram` | Datagram, for syslog over UDP |
| `SolidSyslogLwipSocketTcpStream` | Stream, for syslog over TCP and as the byte transport under TLS |

Each header's own brief states what its class does; the API reference indexes
them all.

## What your build must enable

`LWIP_SOCKET=1`, and `LWIP_NETCONN=1` with it, because lwIP builds its sockets
layer on netconn. Both require `NO_SYS=0` and a `sys_arch` port. Then the
feature each class wraps:

| Setting | For |
|---|---|
| `LWIP_DNS=1` | the resolver |
| `LWIP_UDP=1` | the UDP datagram |
| `LWIP_TCP=1` and `LWIP_SOCKET_SELECT=1` | the TCP stream |

Asking for a class whose feature is off is a link error rather than a silent
no-op. `LWIP_SOCKET_SELECT` defaults to on, and the stream's bounded connect is
what needs it, so a build that turns it off links everything here except the
stream.

Your port must also make `errno` and its codes available, because the transports
read it to tell one refusal from another. lwIP offers three ways to say where it
comes from - `LWIP_PROVIDE_ERRNO` for lwIP's own definitions,
`LWIP_ERRNO_STDINCLUDE` for your C library's `<errno.h>`, or `LWIP_ERRNO_INCLUDE`
naming a header of your own - and leaves it to your `arch/cc.h` if you set none
of them.

The adapters call the `lwip_`-prefixed entry points rather than the unprefixed
macros, so your `LWIP_COMPAT_SOCKETS` setting does not matter to them.

## Nothing here waits on a peer

The stream takes a non-blocking socket, and refuses one the stack will not make
non-blocking rather than proceed with it. Its connect is bounded by the deadline
its config supplies rather than by the stack's own retransmission budget, and
its send and read answer immediately, so a wedged peer costs a failed call
rather than a stalled task.

A send establishes that the peer has not closed its end before it writes. A
socket stays writable after a peer closes, so without that check the stack
would take a record nothing can deliver and the record would be gone; instead
the send fails, the stream closes itself, and your store replays the record on
the next connection.

The datagram leaves its socket as the stack makes it, which costs nothing: a UDP
send has no peer to wait for, and returns once the stack has taken the datagram
or refused it.

The resolve below is the one call in the pack that waits on the network.

## Dead-peer detection is yours to size

The stream turns keepalive on for its own connection and sets the idle period
from `SOLIDSYSLOG_TCP_KEEPALIVE_IDLE_SECONDS`, so the tunables govern this
connection and no other in your system.

How much of that the stack honours depends on one `lwipopts.h` setting.
`LWIP_TCP_KEEPALIVE=1` makes the probe interval and count settable too, and all
three tunables apply. Without it those two are the stack's compile-time
constants and only the idle period is yours. Either way a silent peer is first
probed when the idle tunable elapses, and a connection actually carrying records
notices sooner: the write fails and the stream closes itself so the sender
reconnects.

## No marshal, and no hop

The Sockets API is thread-safe, so every adapter call runs on the calling task.
There is no seam to install at boot and no per-operation hop to pay. That is
the reason to reach for this pack on a build that can host it.

## Blocking is the stack's to size

A resolve that is not answered from the cache blocks the calling task while lwIP
asks the network. The adapter adds no deadline of its own: lwIP gives up after
`DNS_MAX_RETRIES` attempts paced by `DNS_TMR_INTERVAL` and fails the call, so the
longest wait is the one your `lwipopts.h` describes. Under `NO_SYS=0` the tcpip
thread runs the timer that enforces it.

Do not call a resolve from the tcpip thread itself. The call waits for that
thread to answer, so asking it from inside a callback lwIP invoked deadlocks.

## Limits

The pack is IPv4. The resolver asks lwIP for an IPv4 address and refuses
anything else rather than storing a destination a transport cannot send to,
reporting through the error handler when it does.

lwIP's sockets layer exposes no path MTU, so the datagram answers the
unknown-path payload for IPv4 rather than a figure it cannot stand behind. A
record the stack says is too long is reported as oversize rather than as a
failure, which is what lets the sender retry it trimmed to that payload instead
of attempting it whole again.

On a stack built without IPv4 that request is rejected outright, so every
resolve fails and reports. A deployment on such a stack needs a different
resolver, not a different configuration of this one.

## When it does not work

Install a handler before you start, and expect several answers rather than
one. [Error severity](../../error-severity.md) covers what each level means.

- **A lookup that does not answer raises nothing.** An unknown name, or no reply
  from the DNS server, fails the resolve and reports no event. What you see is
  the delivery failure the sender raises once records stop getting through.
- **A lookup refused for its address family raises `ERROR`.** The destination
  cannot be served as asked, and retrying will not change that.
- **A connect that fails raises one event per attempt**, `ERROR` where this
  device could not obtain or start the connection and `WARNING` where the
  destination answered with something other than a connection, or did not
  answer inside the budget. The detail code names which step gave up.
- **A socket option the stack declines raises `WARNING`**, once per connection
  attempt however many were declined. The connection carries records; what the
  option bought, usually prompt detection of a dead peer, does not.
- **A `CRITICAL` at create time** means the component fell back to its Null
  object, so nothing will be delivered at all.
