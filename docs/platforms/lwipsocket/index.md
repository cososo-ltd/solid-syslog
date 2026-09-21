# lwIP (Sockets API)

`Platform/LwipSocket/` wraps the Sockets API of lwIP
([lwIP documentation](https://www.nongnu.org/lwip/2_1_x/index.html)). Its `.c`
files compile against your `lwipopts.h`, so the adapter inherits your stack's
configuration.

Fills the Resolver [role](../../roles/index.md), plus the address handle a
transport reads back to send. The
[platform x capability matrix](../index.md) shows which platform fills what.

## What it ships

## What your build must enable

`LWIP_SOCKET=1`, and `LWIP_NETCONN=1` with it, because lwIP builds its sockets
layer on netconn. Both require `NO_SYS=0` and a `sys_arch` port. The resolver
additionally needs `LWIP_DNS=1`; asking for the class without it is a link
error rather than a silent no-op.

The adapters call the `lwip_`-prefixed entry points rather than the unprefixed
macros, so your `LWIP_COMPAT_SOCKETS` setting does not matter to them.

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

The address and the resolver are IPv4. The resolver asks lwIP for an IPv4
address and refuses anything else rather than storing a destination a transport
cannot send to, reporting through the error handler when it does.

On a stack built without IPv4 that request is rejected outright, so every
resolve fails and reports. A deployment on such a stack needs a different
resolver, not a different configuration of this one.

## When it does not work

Install a handler before you start, and expect these three answers rather than
one. [Error severity](../../error-severity.md) covers what each level means.

- **A lookup that does not answer raises nothing.** An unknown name, or no reply
  from the DNS server, fails the resolve and reports no event. What you see is
  the delivery failure the sender raises once records stop getting through.
- **A lookup refused for its address family raises `ERROR`.** The destination
  cannot be served as asked, and retrying will not change that.
- **A `CRITICAL` at create time** means the component fell back to its Null
  object, so nothing will be delivered at all.
