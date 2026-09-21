# lwIP (Sockets API) setup

Wiring the adapters so a sender can resolve its destination and send to it.
[lwIP (Sockets API)](index.md) covers what the pack fills and what your build
must enable; the config fields are documented on the structs themselves. This
page is the wiring.

## What your lwipopts.h must say

```c
#define NO_SYS          0
#define LWIP_SOCKET     1
#define LWIP_NETCONN    1
#define LWIP_DNS        1
#define LWIP_UDP        1   /* the datagram */
#define LWIP_TCP        1   /* the stream */
#define LWIP_SOCKET_SELECT 1 /* the stream's bounded connect; on by default */
```

Your `arch/cc.h` must also provide `errno` and its codes, the way lwIP asks of
a port that does not set `LWIP_PROVIDE_ERRNO`, because the transports read it
after a refused call.

Turning the sockets layer on grows the upstream build as well as the
configuration: lwIP compiles `api/sockets.c`, `api/api_lib.c`, `api/api_msg.c`,
`api/netbuf.c` and `api/netdb.c`, which cost on the order of 12 KB of flash on a
Cortex-M3. Budget for it before choosing this tier.

## Drawing the pieces

```c
#include "SolidSyslogLwipSocketAddress.h"
#include "SolidSyslogLwipSocketDatagram.h"
#include "SolidSyslogLwipSocketResolver.h"
#include "SolidSyslogLwipSocketTcpStream.h"

struct SolidSyslogAddress*  address  = SolidSyslogLwipSocketAddress_Create();
struct SolidSyslogResolver* resolver = SolidSyslogLwipSocketResolver_Create();

/* For syslog over UDP. */
struct SolidSyslogDatagram* datagram = SolidSyslogLwipSocketDatagram_Create();

/* For syslog over TCP, and as the byte transport under a TLS stream. NULL
   leaves the connect deadline at the SOLIDSYSLOG_TCP_CONNECT_TIMEOUT_MS
   tunable; supply a getter where the deadline has to move at runtime. */
struct SolidSyslogStream* stream = SolidSyslogLwipSocketTcpStream_Create(NULL);
```

Only the stream takes a configuration, and then only to tune its connect
deadline; nothing else in the pack has anything only you can decide. One
address, one resolver and one transport serve one sender. The pool sizes that
bound how many you may draw are in
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

Neither transport waits on a peer, so this is about the resolve alone: it blocks
the calling task, and that task must not be the tcpip thread. The call
hands the lookup to that thread and waits for it, so asking from inside a
callback lwIP invoked waits for a thread that is waiting for you.

Call `SolidSyslog_Service` from an ordinary task, and the resolve that happens
inside it is safe.

## Which build this pack suits

One where lwIP is built with `NO_SYS=0` and a task can afford to block while the
stack answers. That is what buys the absence of a marshal seam and of any
per-call hop. Where your build cannot host the sockets layer, the
[platform x capability matrix](../index.md) is the place to choose from.
