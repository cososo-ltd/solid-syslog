# lwIP (Sockets API) setup

Wiring the adapters so a sender can resolve and address its destination.
[lwIP (Sockets API)](index.md) covers what the pack fills and what your build
must enable; the config fields are documented on the structs themselves. This
page is the wiring.

## What your lwipopts.h must say

```c
#define NO_SYS          0
#define LWIP_SOCKET     1
#define LWIP_NETCONN    1
#define LWIP_DNS        1
```

Turning the sockets layer on grows the upstream build as well as the
configuration: lwIP compiles `api/sockets.c`, `api/api_lib.c`, `api/api_msg.c`,
`api/netbuf.c` and `api/netdb.c`, which cost on the order of 12 KB of flash on a
Cortex-M3. Budget for it before choosing this tier.

## Drawing the pieces

```c
#include "SolidSyslogLwipSocketAddress.h"
#include "SolidSyslogLwipSocketResolver.h"

struct SolidSyslogAddress*  address  = SolidSyslogLwipSocketAddress_Create();
struct SolidSyslogResolver* resolver = SolidSyslogLwipSocketResolver_Create();
```

Neither takes a configuration, because neither has anything only you can decide.
One address and one resolver serve one sender. The pool sizes that bound how
many you may draw are in
[Adding it to your build](../../build-integration.md#tunables); drawing past them
hands back a Null object and reports `CRITICAL`, so a sender wired from an
exhausted pool delivers nothing rather than misbehaving.

## Resolving by name

The resolver takes the destination host as given and asks lwIP for an IPv4
address, applying your port to the answer. A numeric literal resolves without
touching the network; a name needs a DNS server your stack can reach.

Where there is no DNS server, `DNS_LOCAL_HOSTLIST` maps names statically and
resolves entirely on-device:

```c
#define LWIP_DNS                1
#define DNS_LOCAL_HOSTLIST      1
#define DNS_LOCAL_HOSTLIST_INIT \
    { DNS_LOCAL_HOSTLIST_ELEM("collector", IPADDR4_INIT_BYTES(10, 0, 2, 2)) }
```

## The one thing that will catch you out

A resolve blocks the calling task, and it must not be the tcpip thread. The call
hands the lookup to that thread and waits for it, so asking from inside a
callback lwIP invoked waits for a thread that is waiting for you.

Call `SolidSyslog_Service` from an ordinary task, and the resolve that happens
inside it is safe.

## Which build this pack suits

One where lwIP is built with `NO_SYS=0` and a task can afford to block while the
stack answers. That is what buys the absence of a marshal seam and of any
per-call hop. Where your build cannot host the sockets layer, the
[platform x capability matrix](../index.md) is the place to choose from.
