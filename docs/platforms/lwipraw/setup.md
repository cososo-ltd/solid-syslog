# lwIP (Raw API) setup

Wiring the lwIP adapters so a sender delivers over UDP or TCP.
[lwIP (Raw API)](index.md) covers what the adapters fill and what your
`lwipopts.h` must enable; the config fields are documented on the structs
themselves. This page is the wiring, and the one thing that is easy to get
wrong.

## What to link

```cmake
set(SOLIDSYSLOG_PLATFORMS "LwipRaw;<OsPrimitives>")
target_link_libraries(my_app PRIVATE SolidSyslog SolidSyslog::LwipRaw)
```

The placeholders are whichever platforms the [capability matrix](../index.md)
says fill the rest of what your build needs. See
[naming your platforms](../../build-integration.md#cmake) for how the list is
read.

## The marshal

Every Raw API call the adapters make is routed through a single hop, installed
once at boot:

```c
#include "SolidSyslogLwipRawMarshal.h"

SolidSyslogLwipRaw_SetMarshal(MyMarshal);
```

**The marshal must run its callback synchronously, before it returns.** The
adapter reads results the moment the hop returns. One global slot serves the
process, because there is one lwIP instance to protect.

Which marshal you install depends only on how lwIP is built.

### `NO_SYS=1` - bare metal

Install nothing. There is one execution context and no core to protect, so the
default direct call is correct.

The trap is the `Sleep` callback. The TCP stream's `Open` is synchronous over an
asynchronous `tcp_connect`, so it spins and sleeps while waiting for the
connection callback - and on bare metal nothing else is driving lwIP during that
sleep. Your `Sleep` must keep the stack running:

```c
void MyLwipSleep(uint32_t milliseconds)
{
    /* Elapsed rather than a deadline, so the loop is correct across a
     * timebase wrap; unsigned subtraction wraps with it. */
    uint32_t start = MyTimebase_NowMs();
    while ((MyTimebase_NowMs() - start) < milliseconds)
    {
        sys_check_timeouts();
        MyNetif_DrivePolledRx();   /* your board's receive pump */
    }
}
```

A `Sleep` that merely busy-waits leaves lwIP unable to advance its state
machine, so the connection callback never fires and `Open` times out with
nothing visibly wrong.

Call `SolidSyslog_Service` from the same loop that calls `sys_check_timeouts`.

### `NO_SYS=0` - an lwIP thread

A dedicated thread owns lwIP's state, so every call has to reach it. With core
locking compiled in, take the lock around the hop:

```c
void MyCoreLockMarshal(SolidSyslogLwipRawCallback callback, void* context)
{
    LOCK_TCPIP_CORE();
    callback(context);
    UNLOCK_TCPIP_CORE();
}
```

Core locking is the route to prefer, and it is what the reference target uses.
It runs the callback in your own task under the lock, so it is synchronous by
construction, independent of task priority, and costs no mailbox message.

Without core locking you must post to lwIP's mailbox - and then wait for the
callback to *run*, which the post alone does not do. `tcpip_callback_with_block`
blocks until the mailbox accepts the message, not until the tcpip thread
executes it, so returning at that point would break the contract and leave the
hop's stack frame dangling under the callback. Carry a semaphore:

```c
struct MarshalHop
{
    SolidSyslogLwipRawCallback callback;
    void* context;
    sys_sem_t done;
};

static void RunHop(void* ctx)
{
    struct MarshalHop* hop = ctx;
    hop->callback(hop->context);
    sys_sem_signal(&hop->done);
}

void MyTcpipMarshal(SolidSyslogLwipRawCallback callback, void* context)
{
    if (MyPort_CurrentTaskIsTcpipThread())   /* else this deadlocks */
    {
        callback(context);
        return;
    }
    struct MarshalHop hop = {callback, context, {0}};
    if (sys_sem_new(&hop.done, 0) == ERR_OK)
    {
        if (tcpip_callback_with_block(RunHop, &hop, 1) == ERR_OK)
        {
            sys_arch_sem_wait(&hop.done, 0);
        }
        sys_sem_free(&hop.done);
    }
}
```

The `err_t` matters: a failed post means `RunHop` never runs, so waiting on the
semaphore would hang and returning without waiting would let the adapter read
result state nothing wrote.

Two more things to check in your port: the mailbox must be sized for a blocking
post, and lwIP exposes no portable "am I on the lwIP thread?" predicate, so
most ports compare the current task handle against the one given to
`tcpip_init`. Without that guard, an adapter call made from inside a callback
lwIP itself invoked will deadlock.

Here `Sleep` does not drive lwIP - the lwIP thread is running concurrently - but
it must still wait at least the time asked: the connect and resolve deadlines are
the sum of the sleeps requested, not a clock reading, so a `Sleep` that only
yields spends the whole deadline at once. Wrap your RTOS delay and round a
non-zero request up to one tick.

Marshal at the individual lwIP call, which is what this seam gives you. Wrapping
`SolidSyslog_Service` instead puts file I/O, crypto, buffer locking and record
formatting on the lwIP thread, none of which touch lwIP, and starves its timer
and receive path under load.

## Resolving by name

The DNS resolver bridges lwIP's asynchronous `dns_gethostbyname` to the
synchronous resolve contract the same way the TCP stream bridges connect: a
cache or hostlist hit returns immediately, and anything else spins on your
thread - never lwIP's - sleeping via your `Sleep` until the answer arrives or
the deadline passes. It needs `LWIP_DNS=1` and a `Sleep`; without one it falls
back to the Null resolver.

Where there is no DNS server, `DNS_LOCAL_HOSTLIST` maps names statically and
resolves entirely on-device:

```c
#define LWIP_DNS                1
#define DNS_LOCAL_HOSTLIST      1
#define DNS_LOCAL_HOSTLIST_INIT \
    { DNS_LOCAL_HOSTLIST_ELEM("collector", IPADDR4_INIT_BYTES(10, 0, 2, 2)) }
```

Wire whichever resolver your deployment needs:
[`SolidSyslogLwipRawResolver`](../../api/SolidSyslogLwipRawResolver_8h.md) for an
endpoint given as an address, or
[`SolidSyslogLwipRawDnsResolver`](../../api/SolidSyslogLwipRawDnsResolver_8h.md)
for one that may be a name. Each says which fits. The pool sizes and timeouts are
in [Adding it to your build](../../build-integration.md#tunables).

## Limits

The pack is IPv4. Both resolvers refuse an IPv6 answer rather than store a
destination the datagram and the stream cannot send to, and report through the
error handler when they do. Neither the datagram nor the TCP stream
selects an output interface - lwIP's routing table decides - and the datagram
cannot discover the path maximum transmission unit, so it answers the payload
limit its config states, or `SOLIDSYSLOG_UDP_IPV4_PAYLOAD_LIMIT` where none is
stated.

## When it does not work

Install an error handler before you start; [error severity](../../error-severity.md)
says what each level is telling you.

- **A lookup that does not answer raises nothing from the numeric resolver**,
  and `WARNING` from the DNS resolver only when its deadline passes. An
  unparseable literal, an unknown name or a rejected query fails the resolve
  and reports no event; what you see is the delivery failure the sender raises
  once records stop getting through.
- **A lookup refused for its address family raises `ERROR`.** The destination
  cannot be served as asked, and retrying will not change that.
