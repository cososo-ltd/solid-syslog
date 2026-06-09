# Integrating SolidSyslog with lwIP (Raw API)

`Platform/LwipRaw/` wraps lwIP's Raw API to provide the same
`SolidSyslogDatagram` / `SolidSyslogStream` / `SolidSyslogAddress` /
`SolidSyslogResolver` vtables the rest of the library composes against.
It is the right choice when:

- Your target runs lwIP — bare-metal, FreeRTOS, Zephyr, ThreadX, NuttX
  — including `NO_SYS=1` deployments where sockets/NETCONN aren't
  available.
- You want to share TCP/IP between SolidSyslog and other lwIP-using
  subsystems (HTTP server, MQTT client, OTA updater) without
  duplicating the stack.
- You're on FreeRTOS but prefer lwIP to FreeRTOS-Plus-TCP for licence
  or sizing reasons. Both backends ship in the same library; pick at
  CMake time with `SOLIDSYSLOG_FREERTOS_NET=LWIP`.

This document covers what *you*, the integrator, plug in. It does not
re-teach lwIP — for that, see the
[upstream lwIP documentation](https://www.nongnu.org/lwip/2_1_x/index.html).

---

## What ships in `Platform/LwipRaw/`

| Class | Wraps | Purpose |
|---|---|---|
| `SolidSyslogLwipRawAddress` | `ip_addr_t` + `u16_t` port | Destination handle the Resolver writes into and the Datagram/TcpStream read from. |
| `SolidSyslogLwipRawResolver` | `ipaddr_aton` | Synchronous numeric IPv4 parsing. Rejects DNS names — use `SolidSyslogLwipRawDnsResolver` for those. Needs no `LWIP_DNS`. |
| `SolidSyslogLwipRawDnsResolver` | `dns_gethostbyname` | DNS name resolution (superset — numeric literals, the DNS cache, and the local hostlist all resolve too). Bridges lwIP's async DNS to the synchronous `Resolve()` contract via a bounded spin (see [DNS](#dns) below). Requires `LWIP_DNS=1`. |
| `SolidSyslogLwipRawDatagram` | `udp_new` / `udp_sendto` / `udp_remove` | UDP sender. Zero-copy `PBUF_REF` send. |
| `SolidSyslogLwipRawTcpStream` | `tcp_new` / `tcp_connect` / `tcp_write` / `tcp_output` / `tcp_recv` / `tcp_recved` / `tcp_close` / `tcp_abort` | TCP byte transport. Bounded synchronous Open. Bounded RX pbuf queue. |

`Platform/LwipRaw/Source/` is **OS-agnostic** — it wraps lwIP only and
contains zero direct calls to FreeRTOS, POSIX, Win32, Zephyr, or any
other host primitive. The one host primitive the TcpStream needs (a
bounded sleep for the synchronous-Open spin loop) is abstracted behind
the `SolidSyslogSleepFunction` typedef and supplied by you at
configure time.

mbedTLS layering is unchanged — `SolidSyslogMbedTlsStream` consumes
`SolidSyslogLwipRawTcpStream` as its byte transport without
modification. See [`docs/integrating-mbedtls.md`](integrating-mbedtls.md)
for the TLS side.

---

## `NO_SYS=1` vs `NO_SYS=0`

lwIP supports both threading models. SolidSyslog supports both — the
adapter code is the same; the difference is entirely in how *you*
drive lwIP forward, and which **marshal** you install.

### The marshal seam

Every Raw API call the adapters make — `udp_sendto`, `tcp_write`,
`pbuf_alloc`, `tcp_close`, … — is routed through a single hop:

```c
#include "SolidSyslogLwipRawMarshal.h"

SolidSyslogLwipRaw_SetMarshal(MyMarshal);   /* once, at boot */
```

The default (nothing installed, or `SolidSyslogLwipRaw_SetMarshal(NULL)`)
is a **direct call** — the adapter calls lwIP on the calling thread. That
is exactly right for `NO_SYS=1`. For `NO_SYS=0` you install a marshal that
hops onto the thread owning the lwIP core.

> **Earlier guidance was wrong.** Prior versions of this guide told you to
> "marshal at the SolidSyslog API boundary" — wrap `SolidSyslog_Service()`
> in `tcpip_callback()`. Don't. That puts file I/O, mbedTLS crypto, the
> CircularBuffer mutex, the StreamSender's formatter work, and the Resolver
> parse — none of which touch lwIP — on the tcpip thread, starving lwIP's
> timer / RX path under load. The correct boundary is the **individual lwIP
> Raw API call**, which is what the marshal seam gives you.

**Contract.** The marshal MUST invoke its callback *synchronously* — before
the marshal function returns. The adapter reads results the callback wrote
immediately after the hop returns. `tcpip_callback_with_block(.., block=1)`
honours this; a bare `tcpip_callback(..)` does not (it queues and returns).

One global slot serves the whole process — there is one lwIP instance and
one tcpip thread, so per-instance marshals would be flexibility without use.

### `NO_SYS=1` (bare-metal main-loop)

Install nothing — the default direct-call marshal is correct. There is one
execution context and no core to protect.


Your `main()` is a forever-loop that, on each pass, calls
`sys_check_timeouts()` and drives the RX path
(`netif->input()` / `ethernetif_input()` / whichever your BSP wires).
Every lwIP Raw API call must happen on that same thread.

SolidSyslog's `Service` loop fits this naturally — call it from your
main loop alongside `sys_check_timeouts()`. The TcpStream's bounded
synchronous-Open spin loop calls your injected `Sleep` callback
between polls; under `NO_SYS=1` your Sleep implementation should
**tick the lwIP machinery** while it waits:

```c
void MyLwipSleep(int milliseconds)
{
    uint32_t deadline = MyTimebase_NowMs() + (uint32_t) milliseconds;
    while (MyTimebase_NowMs() < deadline)
    {
        sys_check_timeouts();
        MyNetif_DrivePolledRx(); /* your BSP's RX pump */
    }
}
```

Without this, `tcp_connect`'s `connected_cb` never fires (lwIP can't
advance its state machine while you sleep), and Open times out.

### `NO_SYS=0` (tcpip thread) — option A: `tcpip_callback`

lwIP runs a dedicated `tcpip` thread that owns its state machine. Install a
marshal that posts each adapter callback to it and **blocks until it runs**:

```c
#include "lwip/tcpip.h"
#include "SolidSyslogLwipRawMarshal.h"

struct MarshalHop
{
    SolidSyslogLwipRawCallback callback;
    void*                      context;
};

static void RunHop(void* ctx)
{
    struct MarshalHop* hop = (struct MarshalHop*) ctx;
    hop->callback(hop->context);
}

void MyTcpipMarshal(SolidSyslogLwipRawCallback callback, void* context)
{
    /* Re-entry guard: if we are ALREADY on the tcpip thread (e.g. the adapter
     * was called from inside a SolidSyslog callback that lwIP itself invoked),
     * posting-and-blocking would deadlock. Run directly instead. */
    if (sys_current_task_is_tcpip_thread()) /* your port's predicate */
    {
        callback(context);
        return;
    }

    struct MarshalHop hop = {callback, context};
    /* block=1 makes this synchronous — required by the marshal contract. */
    (void) tcpip_callback_with_block(RunHop, &hop, 1);
}

/* ... at boot ... */
SolidSyslogLwipRaw_SetMarshal(MyTcpipMarshal);
```

`tcpip_callback_with_block` needs an OS mailbox sized for the blocking post —
ensure `TCPIP_MBOX_SIZE` is adequate. lwIP does not expose a portable
"am I on the tcpip thread?" predicate; most ports compare the current task
handle against the one passed to `tcpip_init`.

### `NO_SYS=0` (tcpip thread) — option B: core locking

If you compiled lwIP with `LWIP_TCPIP_CORE_LOCKING=1`, take the core lock
around the hop instead of posting to the mailbox — lower latency, no context
switch:

```c
#include "lwip/tcpip.h"
#include "SolidSyslogLwipRawMarshal.h"

void MyCoreLockMarshal(SolidSyslogLwipRawCallback callback, void* context)
{
    LOCK_TCPIP_CORE();
    callback(context);
    UNLOCK_TCPIP_CORE();
}

/* ... at boot ... */
SolidSyslogLwipRaw_SetMarshal(MyCoreLockMarshal);
```

`LOCK_TCPIP_CORE` is a recursive lock on most ports, so this is safe even if
the adapter is reached from a context that already holds it.

### The `Sleep` callback under `NO_SYS=0`

TcpStream's bounded-Open spin runs on *your* thread (never the tcpip thread),
so its `Sleep` is just a yield — typically `vTaskDelay` on FreeRTOS:

```c
void MyLwipSleep(int milliseconds)
{
    vTaskDelay(pdMS_TO_TICKS((uint32_t) milliseconds));
}
```

The tcpip thread runs concurrently and processes the SYN/SYN-ACK exchange
while you yield. A worked `NO_SYS=0` + `tcpip_callback` integration ships as
the FreeRtosLwip BDD target (S28.07).

---

## `lwipopts.h` expectations

The adapter wraps a specific subset of lwIP — your `lwipopts.h` needs
those features compiled in. Defaults that already cover us are noted;
features you must enable are flagged.

| Setting | Required | Notes |
|---|---|---|
| `LWIP_RAW=1` | **Yes** | The whole point — Raw API. |
| `LWIP_UDP=1` | **Yes (Datagram)** | Wraps `udp_*`. |
| `LWIP_TCP=1` | **Yes (TcpStream)** | Wraps `tcp_*`. |
| `LWIP_DNS` | DNS-dependent | Required for `SolidSyslogLwipRawDnsResolver`. The numeric `SolidSyslogLwipRawResolver` needs nothing. See [DNS](#dns). |
| `DNS_LOCAL_HOSTLIST` | Optional | A static name→address map consulted before any DNS server, returned synchronously. Pins names without a server (handy for fixed deployments and for test topologies a real resolver can't reach). See [DNS](#dns). |
| `LWIP_TCPIP_CORE_LOCKING` | Marshal-dependent | Only needed if you install the core-locking marshal (option B above). The default `tcpip_callback` marshal (option A) does not require it. `NO_SYS=1` never needs it. |
| `ARP_QUEUEING=1` | **Recommended** | lwIP default. With it, the first datagram to an unresolved peer is `pbuf_clone`d into PBUF_RAM and queued behind the ARP request — when the reply lands, the packet ships. With `ARP_QUEUEING=0` the first datagram is silently dropped at the IP layer; cold-start logging loses messages. |
| `LWIP_TCP_KEEPALIVE=1` | **Recommended** | Without this, the `SOF_KEEPALIVE` bit the adapter sets on every pcb is a no-op. Tune `TCP_KEEPIDLE_DEFAULT` / `TCP_KEEPINTVL_DEFAULT` / `TCP_KEEPCNT_DEFAULT` for your deadline budget. |
| `TCP_MSS` | Per-platform | Default `536` (RFC-conservative). Bump to `1460` on Ethernet links if your MTU is 1500 and you want fewer segments per syslog record. |
| `PBUF_POOL_SIZE` | Per-traffic | Size the pool generously. The Datagram path borrows one `MEMP_PBUF` header per send; the TcpStream RX path borrows one pbuf per segment until `Stream_Read` drains it. |
| `MEMP_NUM_TCP_PCB` | Per-deployment | At least the number of concurrent `SolidSyslogLwipRawTcpStream` instances (default pool = 2 for the TLS-over-plain-TCP pair). |
| `MEMP_NUM_UDP_PCB` | Per-deployment | At least the number of concurrent `SolidSyslogLwipRawDatagram` instances (default pool = 1). |

---

## Wiring example — bare-metal `NO_SYS=1`

```c
#include "SolidSyslog.h"
#include "SolidSyslogConfig.h"
#include "SolidSyslogLwipRawAddress.h"
#include "SolidSyslogLwipRawDatagram.h"
#include "SolidSyslogLwipRawResolver.h"
#include "SolidSyslogLwipRawTcpStream.h"
#include "SolidSyslogPassthroughBuffer.h"
#include "SolidSyslogStreamSender.h"
#include "SolidSyslogUdpSender.h"

extern void MyLwipSleep(int milliseconds);  /* sys_check_timeouts + RX pump */

static struct SolidSyslog*       g_syslog;
static struct SolidSyslogBuffer* g_buffer;

void LogPipelineInit(void)
{
    struct SolidSyslogResolver*  resolver  = SolidSyslogLwipRawResolver_Create();
    struct SolidSyslogAddress*   udpAddr   = SolidSyslogLwipRawAddress_Create();
    struct SolidSyslogDatagram*  datagram  = SolidSyslogLwipRawDatagram_Create();

    struct SolidSyslogUdpSenderConfig udpCfg = {
        .Resolver         = resolver,
        .Datagram         = datagram,
        .Address          = udpAddr,
        .Endpoint         = MyEndpoint,         /* your SolidSyslogEndpointFunction */
        .EndpointVersion  = MyEndpointVersion,
    };
    struct SolidSyslogSender* sender = SolidSyslogUdpSender_Create(&udpCfg);

    g_buffer = SolidSyslogPassthroughBuffer_Create(sender);

    struct SolidSyslogConfig syslogCfg = {
        .Hostname  = MyHostname,
        .AppName   = MyAppName,
        .ProcessId = MyProcessId,
        .Clock     = MyClock,
        .Buffer    = g_buffer,
    };
    g_syslog = SolidSyslog_Create(&syslogCfg);
}

void MainLoop(void)
{
    for (;;)
    {
        sys_check_timeouts();
        MyNetif_DrivePolledRx();
        SolidSyslog_Service(g_syslog);
        /* … rest of your application … */
    }
}
```

For TCP, swap the UDP sender for a `SolidSyslogStreamSender` whose
`Stream` is a `SolidSyslogLwipRawTcpStream` built with your `Sleep`:

```c
struct SolidSyslogLwipRawTcpStreamConfig streamCfg = {
    .GetConnectTimeoutMs   = NULL,        /* falls back to SOLIDSYSLOG_TCP_CONNECT_TIMEOUT_MS */
    .ConnectTimeoutContext = NULL,
    .Sleep                 = MyLwipSleep, /* required */
};
struct SolidSyslogStream* tcpStream = SolidSyslogLwipRawTcpStream_Create(&streamCfg);
```

---

## Adapter-specific notes

### Datagram — pbuf strategy

`SolidSyslogLwipRawDatagram` uses `PBUF_REF`: a single pbuf header is
allocated per `SendTo`, its `payload` is pointed at the caller's
buffer, `udp_sendto` is called, and the header is `pbuf_free`d before
return. Zero copy on the hot path.

This is safe across ARP queueing because lwIP's `etharp_query` does
`pbuf_clone(…, PBUF_RAM, q)` — it copies the referenced payload into
a private RAM pbuf before queueing, so the caller's buffer only needs
to live for the `udp_sendto` call itself (which is the synchronous
guarantee `SolidSyslogDatagram_SendTo` already provides).

### TcpStream — `tcp_write` strategy

`SolidSyslogLwipRawTcpStream` uses `TCP_WRITE_FLAG_COPY`: lwIP copies
your bytes into its own pbufs before `tcp_write` returns. This costs
one `memcpy` per send but honours the synchronous `Stream_Send(buf,
len)` lifetime contract — caller buffers are free at return,
regardless of when the peer ACKs.

`tcp_output` is called after every successful `tcp_write` to nudge
transmission. If `tcp_output` returns `ERR_MEM`, the data is already
in `pcb->snd_buf` — lwIP will retry on the next `tcp_tmr` tick and
the wrapper reports Send-success (lwIP owns the bytes, exactly
matching POSIX's "kernel accepted the data into the send buffer"
semantics).

### TcpStream — synchronous Open via spin-with-sleep

`tcp_connect` is asynchronous: it returns immediately and lwIP fires
the registered `connected_cb` when the SYN/SYN-ACK exchange
completes. `SolidSyslogStream_Open` is synchronous. The wrapper
bridges by spinning on a `Connected` flag set by its `connected_cb`,
sleeping `SOLIDSYSLOG_LWIP_RAW_TCP_CONNECT_POLL_MS` (default 10 ms)
between checks via your injected `Sleep`, bounded by the
`GetConnectTimeoutMs` getter (default `SOLIDSYSLOG_TCP_CONNECT_TIMEOUT_MS`
= 200 ms — install a runtime getter per the S12.17 pattern if you
need to vary it).

Timeout → `tcp_abort` on the pcb, Open returns `false`. Errored
callback → `tcp_abort`, Open returns `false`. Immediate non-`ERR_OK`
from `tcp_connect` → `tcp_abort`, Open returns `false`.

### TcpStream — RX queue

lwIP's `tcp_recv` callback fires when bytes arrive. The wrapper owns
a bounded ring of pbuf pointers sized by `SOLIDSYSLOG_LWIP_RAW_TCP_RX_QUEUE_SIZE`
(default 8). Each `Stream_Read` drains bytes from the head pbuf,
calls `tcp_recved(pcb, n)` to ACK back to lwIP's receive window, and
`pbuf_free`s the head when fully drained. Queue full → the
callback returns non-`ERR_OK` so lwIP retains the pbuf and replays
the callback later (lwIP's flow-control hook).

> **Chained pbufs.** lwIP hands a single received segment as a **pbuf
> chain** (`p->tot_len > p->len`, `p->next != NULL`) whenever the
> payload spans more than one pool pbuf — normal once a segment exceeds
> one `PBUF_POOL_BUFSIZE`, and influenced by the peer / on-path
> segmentation. The wrapper drains the *whole chain* keyed off
> `tot_len` (via `pbuf_copy_partial`, walking `p->next`) across one or
> more `Stream_Read` calls, and `pbuf_free`s the head — which frees
> every link — only once the chain is fully consumed. Integrators
> supplying their own `SolidSyslogStream` byte transport must honour the
> same contract: never read only `head->len` and then free the chain, or
> the tail bytes are lost (this corrupts a stacked TLS record stream).

The default-8 queue size is sized for the typical mTLS handshake
flight (ServerHello + Certificate + ServerKeyExchange +
ServerHelloDone is 2–4 segments). Bump it for streaming server
responses; lower it if your `MEMP_NUM_PBUF` is constrained and you
need lwIP to backpressure sooner.

### TcpStream — lifecycle ownership

The wrapper owns its `tcp_pcb` end-to-end. Three things to know:

1. **`tcp_err` releases the pcb upstream.** When lwIP fires
   `tcp_err` for a fatal event (RST, OOM, ABRT), the pcb is gone
   from lwIP's side *before* the callback runs. The wrapper's
   `tcp_err` handler nulls its internal `Pcb` field and sets an
   `Errored` flag. The next `Stream_Send` returns `false`; the next
   `Stream_Read` returns `-1`. Crucially, calling `tcp_close` on a
   pcb that was already released by `tcp_err` is a **use-after-free
   in lwIP** — the wrapper guards against this with a `Pcb != NULL`
   check before `tcp_close`. **You never see this rule** unless you
   bypass the abstraction and poke at lwIP pcbs directly through
   your own code — don't.

2. **Peer FIN (`tcp_recv` with `p == NULL`) drains before EOF.**
   The half-close sets `Errored`; the next `Stream_Read` that finds
   the queue empty returns `-1` and internally `tcp_close`s the
   pcb. Already-queued bytes drain first.

3. **`Close` is idempotent.** Second `Close` is a no-op. `Destroy`
   internally calls `Close` (which drains the RX queue's pbufs and
   then `tcp_close`s if the pcb is still around), then overwrites
   the abstract base with `SolidSyslogNullStream` so use-after-
   destroy is a safe no-op rather than a NULL-fn-pointer crash.

---

## Tunables

All tunables live in `Core/Interface/SolidSyslogTunablesDefaults.h`.
Override by `#define`ing them in a user-tunables header passed via
the `SOLIDSYSLOG_USER_TUNABLES_FILE` CMake variable.

| Tunable | Default | Adjust when |
|---|---|---|
| `SOLIDSYSLOG_RESOLVER_POOL_SIZE` | `1U` | You wire more than one resolver. Role pool shared by the numeric and DNS resolvers — counts instances, not implementations, so wiring **both** the numeric and DNS resolver in one build means setting this to `2U`. |
| `SOLIDSYSLOG_DNS_RESOLVE_TIMEOUT_MS` | `5000U` | Default suits a healthy recursive resolver. Raise for a slow / distant DNS server. |
| `SOLIDSYSLOG_LWIP_RAW_DNS_RESOLVE_POLL_MS` | `10U` | Default gives 500 polls inside the 5 s resolve deadline. Lower to notice completion sooner; raise to reduce spin overhead on a constrained MCU. |
| `SOLIDSYSLOG_DATAGRAM_POOL_SIZE` | `1U` | You wire more than one UDP sender. |
| `SOLIDSYSLOG_TCP_STREAM_POOL_SIZE` | `2U` | You wire more than the canonical plain-TCP + TLS-underlying-TCP pair. |
| `SOLIDSYSLOG_ADDRESS_POOL_SIZE` | `3U` | Shared with PlusTcp / Posix / Winsock — bump if you need >3 concurrent destinations. |
| `SOLIDSYSLOG_TCP_CONNECT_TIMEOUT_MS` | `200U` | Default suits loopback / LAN. Raise for WAN deployments behind a high-RTT link; or install a runtime `GetConnectTimeoutMs` getter for per-instance tuning. |
| `SOLIDSYSLOG_LWIP_RAW_TCP_CONNECT_POLL_MS` | `10U` | Default gives 20 polls inside the 200 ms connect deadline. Lower it to notice a fast connect sooner; raise it to reduce spin overhead on a constrained MCU. |
| `SOLIDSYSLOG_LWIP_RAW_TCP_RX_QUEUE_SIZE` | `8U` | Sized for the typical mTLS handshake flight. Bump for streaming server responses; lower if `MEMP_NUM_PBUF` is tight. |

---

## DNS

`SolidSyslogLwipRawDnsResolver` resolves the destination **by name** via
lwIP's `dns_gethostbyname`. It is a superset of the numeric
`SolidSyslogLwipRawResolver` — numeric literals, DNS-cache hits, and
local-hostlist entries all resolve through it too — so you only need one
resolver, picked by whether you compile DNS in.

### The async bridge

`dns_gethostbyname` is asynchronous: a cold lookup returns `ERR_INPROGRESS`
and lwIP invokes a `dns_found_callback` later, on the tcpip thread, when the
answer arrives. The library's `SolidSyslogResolver_Resolve` contract is
synchronous, so the adapter bridges the two exactly as `SolidSyslogLwipRawTcpStream`
bridges `tcp_connect`:

1. The `dns_gethostbyname` call is made **under the marshal hop**
   (`SolidSyslogLwipRaw_SetMarshal`) — it touches lwIP core state, unlike the
   numeric resolver's pure `ipaddr_aton` parse, which is why only this resolver
   marshals.
2. `ERR_OK` (synchronous hit — numeric, cache, or local hostlist) → the address
   is ready immediately; no spin.
3. `ERR_INPROGRESS` → the adapter spins on the **caller's** thread, sleeping via
   your injected `Sleep` between polls of a flag the callback sets, until the
   answer arrives or the deadline elapses. The spin never sleeps the tcpip
   thread (that would starve the DNS retransmit timer and RX).
4. A delivered address → success; a delivered `NULL` (no such host) → failure;
   deadline exceeded → failure plus a `SolidSyslog_Error` report
   (`LWIPRAWDNSRESOLVER_ERROR_RESOLVE_TIMEOUT`).

`Sleep` is **required** (a `NULL` config falls back to `NullResolver`). The
deadline and poll period come from `SOLIDSYSLOG_DNS_RESOLVE_TIMEOUT_MS` (5 s)
and `SOLIDSYSLOG_LWIP_RAW_DNS_RESOLVE_POLL_MS` (10 ms) — build-time only; DNS
timeout rarely needs runtime tuning, so there is no per-instance getter.

### Local hostlist (no DNS server)

For fixed deployments — or test topologies where a real resolver can't return a
*reachable* address — `DNS_LOCAL_HOSTLIST` maps names statically. A hostlist hit
returns `ERR_OK` before any server is consulted, so the resolve completes
synchronously and entirely on-device:

```c
/* lwipopts.h */
#define LWIP_DNS                1
#define DNS_LOCAL_HOSTLIST      1
#define DNS_LOCAL_HOSTLIST_INIT \
    { DNS_LOCAL_HOSTLIST_ELEM("syslog-ng", IPADDR4_INIT_BYTES(10, 0, 2, 2)) }
```

### Wiring

```c
#include "SolidSyslogLwipRawDnsResolver.h"

extern void MyLwipSleep(int milliseconds);   /* same Sleep the TcpStream uses */

struct SolidSyslogLwipRawDnsResolverConfig dnsCfg = { .Sleep = MyLwipSleep };
struct SolidSyslogResolver* resolver = SolidSyslogLwipRawDnsResolver_Create(&dnsCfg);
/* hand `resolver` to the UdpSender / StreamSender config exactly as the
   numeric resolver — the destination host is now a name, e.g. "logs.example.com". */
```

> **Known limitation — over-the-wire DNS is not BDD-covered.** The
> FreeRTOS-on-lwIP BDD target resolves the oracle by name, but only through the
> *synchronous local-hostlist* branch: the QEMU slirp + docker topology cannot
> hand the guest a reachable address for the `syslog-ng` alias over real DNS
> (slirp's forwarder resolves it to a docker-bridge IP the guest has no route
> to; only `10.0.2.2` reaches the oracle). The async / over-the-wire / timeout
> branches are unit-tested instead
> (`Tests/Lwip/SolidSyslogLwipRawDnsResolverTest`), consistent with the
> project's integration-over-BDD stance for paths the harness can't realistically
> drive.

---

## What this guide does not cover

- **IPv6** — the current Address / Resolver are IPv4-only.
- **Multi-`netif` routing** — neither Datagram nor TcpStream selects
  an output interface; lwIP's routing table decides.
- **Jumbo-frame MTU discovery** — `Datagram_MaxPayload` returns
  `SOLIDSYSLOG_UDP_IPV6_SAFE_PAYLOAD` (1232 bytes) unconditionally.
