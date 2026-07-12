# Porting SolidSyslog to a new platform

> [!WARNING]
> The documentation is under active development and may be incomplete or
> inaccurate. Do not rely on it for integration, security, or compliance
> decisions until the 0.1.0 release.

Porting SolidSyslog to a new OS, network stack, filesystem, or crypto library is
filling a role, not editing Core. Core never changes. You write a small
adapter that satisfies one of the twelve vtable contracts, drop it into your
build, and wire it into your config. This page is the contract those adapters
honour, written from the code that already ships.

## The role model

Core is a fixed set of algorithms (the formatter/message pipeline, the Service
drain loop, the buffer/store machinery) plus twelve roles. A role is a
`struct` of function pointers (a vtable) declared in a
`SolidSyslog<Role>Definition.h` header under
[`Core/Interface/`](../Core/Interface/). An *adapter* is a concrete
implementation of one role for one platform (`SolidSyslogPosixMutex`,
`SolidSyslogLwipRawDatagram`, …).

Every role has a Core Null implementation
(`SolidSyslogNull<Role>_Get()`) whose methods are safe no-ops. Omit an adapter
and the Null object stands in: nothing dangles, and Core's algorithms keep
running against a well-behaved do-nothing. So porting is additive: you provide
the roles your deployment needs and leave the rest to their Nulls. You never edit
Core, and you never touch a role you don't use.

The [capability matrix in Getting started](getting-started.md#pick-your-stack--capability-matrix)
lists every role and the adapters that ship for it; this page is what you write
when none of the shipped adapters fits your platform.

## Anatomy of an adapter

Take [`SolidSyslogPosixMutex`](../Platform/Posix/Source/SolidSyslogPosixMutex.c)
as the worked example: the simplest role, but the shape is identical for all
twelve. An adapter is four files:

| File | Holds |
|---|---|
| `Platform/<X>/Interface/SolidSyslog<Adapter>.h` | Public `_Create` / `_Destroy` — the only symbols system-setup code touches |
| `Platform/<X>/Source/SolidSyslog<Adapter>Private.h` | The instance `struct`, embedding the role vtable as its first member |
| `Platform/<X>/Source/SolidSyslog<Adapter>.c` | The vtable function implementations |
| `Platform/<X>/Source/SolidSyslog<Adapter>Static.c` | The static instance pool + `_Create` / `_Destroy` |

### The instance shape

The instance `struct` embeds the role type as its first member, named `Base`:

```c
struct SolidSyslogPosixMutex
{
    struct SolidSyslogMutex Base;   /* the vtable — first member */
    pthread_mutex_t Mutex;          /* your per-instance state */
};
```

Because `Base` is first, a `struct SolidSyslogMutex*` and a
`struct SolidSyslogPosixMutex*` share an address; Core holds the former, your
adapter downcasts to the latter by pointer identity. The vtable function pointers
are wired to your `static` implementations once, when the instance is
initialised.

### `_Create` / `_Destroy` and the static pool (no `malloc`)

There is no heap. Each adapter owns a file-scope `static` array of instances and
a parallel `InUse[]` flag array, sized by a role tunable. `_Create` acquires the
first free slot, initialises it, and returns `&pool[i].Base`; on exhaustion it
returns the shared Null sibling and reports an error. `_Destroy` finds the slot
by handle identity, cleans it up, and releases it. The
[`SolidSyslogPoolAllocator`](../Core/Source/SolidSyslogPoolAllocator.h) owns the
slot-walk so no adapter re-implements it:

```c
static bool PosixMutex_InUse[SOLIDSYSLOG_MUTEX_POOL_SIZE];
static struct SolidSyslogPosixMutex PosixMutex_Pool[SOLIDSYSLOG_MUTEX_POOL_SIZE];
static struct SolidSyslogPoolAllocator PosixMutex_Allocator =
    {PosixMutex_InUse, SOLIDSYSLOG_MUTEX_POOL_SIZE};

struct SolidSyslogMutex* SolidSyslogPosixMutex_Create(void)
{
    size_t index = SolidSyslogPoolAllocator_AcquireFirstFree(&PosixMutex_Allocator);
    struct SolidSyslogMutex* handle = SolidSyslogNullMutex_Get();   /* fallback */
    if (SolidSyslogPoolAllocator_IndexIsValid(&PosixMutex_Allocator, index) == true)
    {
        PosixMutex_Initialise(&PosixMutex_Pool[index].Base);
        handle = &PosixMutex_Pool[index].Base;
    }
    else
    {
        PosixMutex_Report(SOLIDSYSLOG_POOL_EXHAUSTED_SEVERITY,
                          SOLIDSYSLOG_CAT_POOL_EXHAUSTED,
                          POSIXMUTEX_ERROR_POOL_EXHAUSTED);
    }
    return handle;
}
```

See [`SolidSyslogPosixMutexStatic.c`](../Platform/Posix/Source/SolidSyslogPosixMutexStatic.c)
for the matching `_Destroy`. The pool size is a role-named tunable,
`SOLIDSYSLOG_MUTEX_POOL_SIZE`, not a per-platform name, because a build links one
implementation per role. Every tunable lives in
[`SolidSyslogTunablesDefaults.h`](../Core/Interface/SolidSyslogTunablesDefaults.h),
`#ifndef`-guarded so integrators override without editing the library.

### Error reporting — the `*Errors.h` convention

Each adapter ships a `SolidSyslog<Adapter>Errors.h` declaring an
`enum SolidSyslog<Adapter>Errors` (`<ADAPTER>_ERROR_*` codes plus an
`_ERROR_MAX` bookend) and an `extern const struct SolidSyslogErrorSource`. When
something fails, the adapter calls `SolidSyslog_Error(severity, source, category,
detail)`: `source` is its own `ErrorSource` (matched by pointer identity in a
handler), `category` is a portable reaction axis from
[`SolidSyslogErrorCategory.h`](../Core/Interface/SolidSyslogErrorCategory.h), and
`detail` is the adapter's own enum value. A handler that doesn't care about your
adapter simply never matches its source. The default handler is a silent no-op:
adapters report and carry on, they never crash the caller.

### Synchronising the slot walk

The pool allocator wraps each slot claim and release in the
`SolidSyslog_LockConfig()` / `SolidSyslog_UnlockConfig()` pair internally:
`AcquireFirstFree` locks per-slot around the claim, `FreeIfInUse` locks around the
release, so an adapter's `_Create` / `_Destroy` inherit the synchronisation for
free and never lock themselves (which is why the example above has no lock call).
Single-task setup gets the no-op default and pays nothing. On a multi-task or multi-core target
where setup races, install the pair once with `SolidSyslog_SetConfigLock(...)`:
`taskENTER_CRITICAL` / `taskEXIT_CRITICAL` (FreeRTOS), a static `pthread_mutex_t`
(POSIX), `EnterCriticalSection` / `LeaveCriticalSection` (Windows), or a spinlock.
This is the only synchronisation primitive the pools use for their own walks.

## Invariants every adapter must honour

- Idempotent `Close` / `Destroy`. No leak on a partial `Open` failure, no
  double-free if `Close` and `Destroy` are both called. Release each resource
  exactly once and null the handle.
- Never free injected handles. An adapter frees only what it created. Handles
  the integrator passed in (an `mbedtls_x509_crt*`, an RNG, a caller's socket)
  are borrowed; the owner frees them. The [Mbed TLS coexistence
  contract](iec62443.md#embedded-sl4-solidsyslogmbedtlsstream) is the template:
  `Platform/MbedTls/Source/` never touches process-global Mbed TLS state.
- A Null must be safe to call. Whatever your role's Null returns (see each
  contract below), it must let Core's algorithm proceed sanely: drop-on-the-floor
  where a drop is harmless, `false` where the caller has an error path to run.
- Bounded blocking. Anything that can wedge (a `connect`, a handshake) is
  bounded by an explicit timeout or deadline: a timeout tunable (e.g.
  `SOLIDSYSLOG_TCP_CONNECT_TIMEOUT_MS`) or a caller-supplied deadline. A
  `SolidSyslogSleepFunction`, where one is used, only paces the poll loop between
  checks; it does not bound the total wait. Steady-state `Send` / `Read` are
  non-blocking.
- Production-C discipline. Tier 1/2 code is single-return, fully braced, and
  MISRA-leaning, see [MISRA deviations](misra-deviations.md) and
  [Naming conventions](NAMING.md).

## Wiring a new pack into the build

- CMake. Group the adapter sources into a namespaced umbrella target
  (`SolidSyslog::<Pack>`) so linking one target compiles the adapter into the
  consumer against its config headers. See the umbrella list in
  [Getting started → Path A](getting-started.md#path-a--cmake-consumer).
- Non-CMake. Add the adapter's `.c` files to your project and put its
  `Interface/` and `Source/` on the include path. The
  [manifest](getting-started.md#path-b--non-cmake-integrator-the-manifest)
  generator lists the exact files for a chosen pack set.

## The twelve role contracts

Each row is a vtable to implement. The reference column is the shipped
implementation to read alongside the contract: a `Platform/Posix/` adapter where
one exists, otherwise the Core composition over a lower role. The Null column
is the fallback Core substitutes when the role is unfilled.

### Networking

Most network ports implement Stream (TCP / TLS byte transport) and
Datagram (UDP); `Sender` is a Core composition over them, so you rarely write
one directly.

| Role | Contract | Vtable | Null fallback | Reference |
|---|---|---|---|---|
| **Resolver** | [`ResolverDefinition.h`](../Core/Interface/SolidSyslogResolverDefinition.h) | `Resolve(transport, host, port, *out)` | `Resolve` → `false`, so the caller's unresolved-host error path runs | [`GetAddrInfoResolver.c`](../Platform/Posix/Source/SolidSyslogGetAddrInfoResolver.c) |
| **Datagram** | [`DatagramDefinition.h`](../Core/Interface/SolidSyslogDatagramDefinition.h) | `Open` · `SendTo(...)→SendResult` · `MaxPayload` · `Close` | Open/Close no-op, `SendTo` → `SENT` (drop), `MaxPayload` → IPv6-safe default | [`PosixDatagram.c`](../Platform/Posix/Source/SolidSyslogPosixDatagram.c) |
| **Stream** | [`StreamDefinition.h`](../Core/Interface/SolidSyslogStreamDefinition.h) | `Open(addr)` · `Send` · `Read` · `Close` | Open/Close no-op, `Send` → `true` (drop), `Read` → `0` (would-block, no teardown) | [`PosixTcpStream.c`](../Platform/Posix/Source/SolidSyslogPosixTcpStream.c) |
| **Sender** | [`SenderDefinition.h`](../Core/Interface/SolidSyslogSenderDefinition.h) | `Send` · `Disconnect` | `Send` → `true` (drop), `Disconnect` no-op | [`StreamSender.c`](../Core/Source/SolidSyslogStreamSender.c) · [`UdpSender.c`](../Core/Source/SolidSyslogUdpSender.c) |

`SendTo` returns a three-way `enum SolidSyslogDatagramSendResult` (not a bool) so
the sender can distinguish a would-block from a hard failure. A `Stream` owns its
connect/keepalive lifecycle; `Read` returns `0` for would-block and a negative
`SolidSyslogSsize` only for a real teardown.

### Storage

The store-and-forward stack is layered: Store (Core `BlockStore`) sits over
BlockDevice, which sits over File. On a new platform you usually implement
only File (and BlockDevice for raw flash); the rest is Core.

| Role | Contract | Vtable | Null fallback | Reference |
|---|---|---|---|---|
| **Store** | [`StoreDefinition.h`](../Core/Interface/SolidSyslogStoreDefinition.h) | `Write` · `ReadNextUnsent` · `MarkSent` · `HasUnsent` · `IsHalted` · `GetTotalBytes` · `GetUsedBytes` · `IsTransient` | No store-and-forward; `IsTransient` → `true` so a rejected `Write` falls through to the sender | [`BlockStore.c`](../Core/Source/SolidSyslogBlockStore.c) |
| **BlockDevice** | [`BlockDeviceDefinition.h`](../Core/Interface/SolidSyslogBlockDeviceDefinition.h) | `Acquire` · `Dispose` · `Exists` · `Read` · `Append` · `WriteAt` · `Size(block)` · `GetBlockSize` | Every method `false` / `0` — no disk | [`FileBlockDevice.c`](../Core/Source/SolidSyslogFileBlockDevice.c) |
| **File** | [`FileDefinition.h`](../Core/Interface/SolidSyslogFileDefinition.h) | `Open` · `Close` · `IsOpen` · `Read` · `Write` · `SeekTo` · `Size` · `Truncate` · `Exists` · `Delete` | Reads / `Exists` → `false`, `Write` / `Delete` → `true`, `Size` → `0`, seek/truncate/close no-op | [`PosixFile.c`](../Platform/Posix/Source/SolidSyslogPosixFile.c) |
| **Buffer** | [`BufferDefinition.h`](../Core/Interface/SolidSyslogBufferDefinition.h) | `Write` · `Read` | `Read` → `false` (empty), `Write` swallows | [`CircularBuffer.c`](../Core/Source/SolidSyslogCircularBuffer.c) · [`PassthroughBuffer.c`](../Core/Source/SolidSyslogPassthroughBuffer.c) |

`Store.IsTransient` is the crucial hint: a *transient* store (like Null) never
retained the record, so Service may try the sender directly; a real store's `Write`
rejection is the discard policy speaking, and Service must not let a newer
record jump the queue past older ones. The portable in-memory `CircularBuffer`
takes an injected `Mutex`, so a `Buffer` port is often just a `Mutex` port.

### OS primitives

| Role | Contract | Vtable | Null fallback | Reference |
|---|---|---|---|---|
| **Mutex** | [`MutexDefinition.h`](../Core/Interface/SolidSyslogMutexDefinition.h) | `Lock` · `Unlock` | No-op (single-task) | [`PosixMutex.c`](../Platform/Posix/Source/SolidSyslogPosixMutex.c) |
| **AtomicCounter** | [`AtomicCounterDefinition.h`](../Core/Interface/SolidSyslogAtomicCounterDefinition.h) | `Increment` — wrap-aware in `[1, 2³¹−1]`, never returns `0` (RFC 5424 §7.3.1) | `Increment` → `1` unconditionally | [`StdAtomicCounter.c`](../Platform/Atomics/Source/SolidSyslogStdAtomicCounter.c) |

### Evidence and integrity

| Role | Contract | Vtable | Null fallback | Reference |
|---|---|---|---|---|
| **StructuredData** | [`StructuredDataDefinition.h`](../Core/Interface/SolidSyslogStructuredDataDefinition.h) | `Format(element)` — write one `[SD-ID …]` via the `SolidSyslogSdElement` sink | No-op (element omitted) | [`MetaSd.c`](../Core/Source/SolidSyslogMetaSd.c) |
| **SecurityPolicy** | [`SecurityPolicyDefinition.h`](../Core/Interface/SolidSyslogSecurityPolicyDefinition.h) | `TrailerSize` + `SealRecord` · `OpenRecord` over a `SolidSyslogSecurityRecord` | No integrity check; `TrailerSize` `0`, seal/open pass through | [`Crc16Policy.c`](../Core/Source/SolidSyslogCrc16Policy.c) |

A `StructuredData.Format` writes through the opaque `SolidSyslogSdElement` sink;
it owns the brackets, the `@`-enterprise SD-ID suffix, and the escaping, so a
producer cannot break the RFC 5424 framing. A `SecurityPolicy` is handed a
`SolidSyslogSecurityRecord` split into a cleartext header (associated data) and a
body. A keyed MAC policy authenticates the whole span (tamper-evident); a
checksum policy such as the vendor-free `Crc16Policy` covers the same span but
only detects accidental corruption, not an attacker; an AEAD policy encrypts the
body in place and writes its `TrailerSize`-byte trailer. `Crc16Policy` is the
reference to read first.

## Where to go next

- [Getting started](getting-started.md): the capability matrix, tunables, and build wiring.
- [Integrating with lwIP (Raw API)](integrating-lwip.md), [Mbed TLS](integrating-mbedtls.md), [FreeRTOS-Plus-FAT](integrating-plusfat.md): worked ports of the networking, TLS, and file roles.
- [Naming conventions](NAMING.md) and [MISRA deviations](misra-deviations.md): the rules Tier 1/2 adapter code follows.
- [Error-event severity policy](error-severity.md): choosing the severity for your adapter's reports.
