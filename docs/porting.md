# Porting SolidSyslog to a new platform

Porting SolidSyslog to a new OS, network stack, filesystem, or crypto library is
filling a role, not editing Core. Core never changes. You write a small
adapter that satisfies one of the vtable contracts, drop it into your
build, and wire it into your config. This page is the contract those adapters
honour, written from the code that already ships.

<!-- markdownlint-disable MD033 — the sticky is styled HTML (.postit-note in brand.css); md_in_html keeps its body as Markdown. -->

<div class="postit-note" markdown>
**Rather we did it?**

We write and support SolidSyslog platform adapters — your RTOS, network stack,
filesystem or crypto library — and the tests that prove them against the
contract. [Talk to us about it](https://www.cososo.co.uk/?service=solidsyslog#contact).
</div>

<!-- markdownlint-enable MD033 -->

## The role model

Core is a fixed set of algorithms (the formatter/message pipeline, the Service
drain loop, the buffer/store machinery) plus a set of roles. A role is a
`struct` of function pointers (a vtable) declared in a
`SolidSyslog<Role>Definition.h` header under `Core/Interface/`. An *adapter* is a concrete
implementation of one role for one platform (`SolidSyslogPosixMutex`,
`SolidSyslogLwipRawDatagram`, …).

Every role has a Core Null implementation
(`SolidSyslogNull<Role>_Get()`) whose methods are safe no-ops. Omit an adapter
and the Null object stands in: nothing dangles, and Core's algorithms keep
running against a well-behaved do-nothing. So porting is additive: you provide
the roles your deployment needs and leave the rest to their Nulls. You never edit
Core, and you never touch a role you don't use.

The [platform × capability matrix](platforms/index.md) lists every role and the
adapters that ship for it; this page is what you write when none of the shipped
adapters fits your platform.

## Anatomy of an adapter

Take `SolidSyslogPosixMutex` as the worked example: the simplest role, but the
shape is identical for every one. An adapter is four files:

| File | Holds |
|---|---|
| `Platform/<X>/Interface/SolidSyslog<Adapter>.h` | Public `<Class>_Create` / `<Class>_Destroy` — the only symbols system-setup code touches |
| `Platform/<X>/Source/SolidSyslog<Adapter>Private.h` | The instance `struct`, embedding the role vtable as its first member |
| `Platform/<X>/Source/SolidSyslog<Adapter>.c` | The vtable function implementations |
| `Platform/<X>/Source/SolidSyslog<Adapter>Static.c` | The static instance pool + `<Class>_Create` / `<Class>_Destroy` |

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

### `<Class>_Create` / `<Class>_Destroy` and the static pool (no `malloc`)

There is no heap. Each adapter owns a file-scope `static` array of instances and
a parallel `InUse[]` flag array, sized by a role tunable. `<Class>_Create` acquires the
first free slot, initialises it, and returns `&pool[i].Base`; on exhaustion it
returns the shared Null sibling and reports an error. `<Class>_Destroy` finds the slot
by handle identity, cleans it up, and releases it. `SolidSyslogPoolAllocator`
owns the slot-walk so no adapter re-implements it:

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
                          SOLIDSYSLOG_POSIX_MUTEX_ERROR_POOL_EXHAUSTED);
    }
    return handle;
}
```

See `SolidSyslogPosixMutexStatic.c` for the matching
`SolidSyslogPosixMutex_Destroy`. The pool size is a role-named tunable,
`SOLIDSYSLOG_MUTEX_POOL_SIZE`, not a per-platform name, because a build links one
implementation per role. Every tunable lives in
[`SolidSyslogTunablesDefaults.h`](api/SolidSyslogTunablesDefaults_8h.md),
`#ifndef`-guarded so integrators override without editing the library.

### Error reporting — the `*Errors.h` convention

Each adapter ships a `SolidSyslog<Adapter>Errors.h` declaring an
`enum SolidSyslog<Adapter>Errors` (`SOLIDSYSLOG_<ADAPTER>_ERROR_*` codes plus a
`SOLIDSYSLOG_<ADAPTER>_ERROR_MAX` bookend) and an
`extern const struct SolidSyslogErrorSource`. How the class name is spelled
inside those constants — one word per PascalCase word, except that your pack's
registry token stays whole — is in
[Naming conventions](NAMING.md#spelling-a-class-name-inside-a-screaming_snake-identifier). When
something fails, the adapter calls `SolidSyslog_Error(severity, source, category,
detail)`: `source` is its own `ErrorSource` (matched by pointer identity in a
handler), `category` is a portable reaction axis from
[`SolidSyslogErrorCategory.h`](api/SolidSyslogErrorCategory_8h.md), and
`detail` is the adapter's own enum value. A handler that doesn't care about your
adapter simply never matches its source. The default handler is a silent no-op:
adapters report and carry on, they never crash the caller.

### Synchronising the slot walk

The pool allocator wraps each slot claim and release in the
`SolidSyslog_LockConfig()` / `SolidSyslog_UnlockConfig()` pair internally:
`AcquireFirstFree` locks per-slot around the claim, `FreeIfInUse` locks around the
release, so an adapter's `<Class>_Create` / `<Class>_Destroy` inherit the synchronisation for
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
- Cleanup runs under the config lock. `FreeIfInUse` holds
  `SolidSyslog_LockConfig()` across your cleanup callback, so whatever `Destroy`
  does to release a resource happens inside whichever primitive the integrator
  installed — a FreeRTOS critical section, at the recommendation above. Blocking
  there is not safe: a mutex may not be taken inside `taskENTER_CRITICAL`, and
  work that needs another task to run cannot complete while interrupts are off.
  The shipped lwIP TCP stream does block this way, tracked as
  [#754](https://github.com/cososo-ltd/solid-syslog/issues/754); until that is
  resolved, keep teardown in your own adapter non-blocking.
- Never free injected handles. An adapter frees only what it created. Handles
  the integrator passed in (a certificate, an RNG, a caller's socket) are
  borrowed; the owner frees them. The same applies to an upstream library's
  process-global state: touch only what you were given, so the library drops
  into a process already using that upstream elsewhere.
- A Null must be safe to call. Whatever your role's Null returns — each is
  documented on its own `SolidSyslogNull<Role>.h` — it must let Core's algorithm
  proceed sanely: drop-on-the-floor where a drop is harmless, `false` where the
  caller has an error path to run.
- Bounded blocking. Anything that can wedge (a `connect`, a handshake) is
  bounded by an explicit timeout or deadline: a timeout tunable (e.g.
  `SOLIDSYSLOG_TCP_CONNECT_TIMEOUT_MS`) or a caller-supplied deadline. A
  `SolidSyslogSleepFunction`, where one is used, only paces the poll loop between
  checks; it does not bound the total wait. Steady-state `Send` / `Read` do not
  wait on the peer, so a wedged collector cannot stall the servicing pass. That
  bounds peer behaviour, not the local stack: a datagram send may still wait on a
  full kernel send buffer, and each platform page states where its own blocking
  surface lies.
- Production-C discipline. Tier 1/2 code is single-return, fully braced, and
  MISRA-leaning, see [MISRA deviations](misra-deviations.md) and
  [Naming conventions](NAMING.md).

## Depending on upstream configuration

An adapter often needs something the upstream project provides only under a
configuration macro — lwIP's `LWIP_DNS`, FreeRTOS's
`configSUPPORT_STATIC_ALLOCATION`. Take these in order.

**Prefer a seam.** Where the adapter is thin, take the dependency as an injected
function pointer with a safe default and never name the upstream symbol.
`SolidSyslogLwipRaw_SetMarshal` covers `NO_SYS=0` against `NO_SYS=1` this way:
the library calls a callback, and the integrator installs a `LOCK_TCPIP_CORE`
pair, or a mailbox shim that waits for the callback to run. Nothing to select at
build time.

**Otherwise gate the translation unit.** Where the adapter carries logic that
belongs in the library — the DNS resolver's async callback handling, poll
interval and timeout — keep it and wrap the file:

```c
#include "lwip/opt.h"

/* This component requires lwIP built with DNS. */
#if LWIP_DNS

/* ... */

#else

/* ISO C forbids an empty translation unit. */
typedef int LwipRawDnsResolver_EmptyTranslationUnit;

#endif /* LWIP_DNS */
```

A gated adapter must also honour these:

- The upstream config header is the first include. It defines the macro, so the
  gate cannot be evaluated before it.
- That hoisted include is the file's only copy. Where the adapter already
  included it further down, delete that one — clang-tidy's
  `readability-duplicate-include` fails the `analyze-tidy-freertos-*` lanes,
  which the `debug` preset does not cover.
- Both translation units gate: the adapter and its `*Static.c` pool sibling.
- The public header neither gates nor includes an upstream header. It states the
  requirement in prose; asking for the class without the option is a link error.
- The adapter stays in its platform's umbrella target, so a consumer globbing the
  pack directory is correct in any configuration.
- Add the macro to the `-D` list in the `cppcheck` steps of
  `.github/workflows/ci.yml` and to `CPPCHECK_CMD` in
  `scripts/misra_renumber.py`. Without it cppcheck analyses the `#else` branch
  and the adapter goes unchecked.

**An `#error` is for a different case.** Use one where the integrator is using
the class and their configuration contradicts it — `SolidSyslogFatFsFile.c`
requires `SOLIDSYSLOG_FILE_DEFAULT_BLOCK_SIZE` to be at least `FF_MAX_SS`.
Absence is not a correct outcome there, so do not gate it.

**Never ship two behaviours from one file.** `#if FEATURE` / `#else` selecting
between implementations is out of scope: it ships two products from one source,
and the combinations multiply across upstream configurations. A gate has one
product and one further state, absent, which has no behaviour to test.

## Wiring a new pack into the build

- The registry. Add a row to `SOLIDSYSLOG_PLATFORM_REGISTRY` in the top-level
  `CMakeLists.txt`. It is the only platform vocabulary in the repo: the option,
  the `SOLIDSYSLOG_PLATFORMS` token, the role report and the manifest all read
  it, so a platform that is not in it does not exist.
- CMake. Group the adapter sources into a namespaced umbrella target
  (`SolidSyslog::<Pack>`) so linking one target compiles the adapter into the
  consumer against its config headers. See the umbrella list in
  [the CMake section of the build guide](build-integration.md#cmake).
- Non-CMake. Add the adapter's `.c` files to your project and put its
  `Interface/` and `Source/` on the include path. The
  [manifest](build-integration.md#ide-and-manifest-builds)
  generator lists the exact files for a chosen set of platforms.

## The role contracts

Each entry is a vtable to implement, linked to its contract: every method, what
it must return, and what Core does with the answer. Each contract page also
diagrams the adapters that already fill that role, so it is where to find an
implementation to read alongside.

### Networking

Most network ports implement Stream (TCP / TLS byte transport) and
Datagram (UDP); `Sender` is a Core composition over them, so you rarely write
one directly.

- [Resolver](api/structSolidSyslogResolver.md)
- [Datagram](api/structSolidSyslogDatagram.md)
- [Stream](api/structSolidSyslogStream.md)
- [Sender](api/structSolidSyslogSender.md)

### Storage

The store-and-forward stack is layered: Store (Core `BlockStore`) sits over
BlockDevice, which sits over File. On a new platform you usually implement
only File (and BlockDevice for raw flash); the rest is Core.

- [Store](api/structSolidSyslogStore.md)
- [BlockDevice](api/structSolidSyslogBlockDevice.md)
- [File](api/structSolidSyslogFile.md)
- [Buffer](api/structSolidSyslogBuffer.md)

The portable in-memory `CircularBuffer` takes an injected `Mutex`, so a `Buffer`
port is often just a `Mutex` port.

### OS primitives

- [Mutex](api/structSolidSyslogMutex.md)
- [AtomicCounter](api/structSolidSyslogAtomicCounter.md)

### Evidence and integrity

- [StructuredData](api/structSolidSyslogStructuredData.md)
- [SecurityPolicy](api/structSolidSyslogSecurityPolicy.md)

A `SecurityPolicy` is the one role where the choice is a security decision, not
a portability one: a keyed MAC is tamper-evident, a checksum such as the
vendor-free [Crc16Policy](api/SolidSyslogCrc16Policy_8h.md) detects accidental
corruption but not an attacker, and an AEAD encrypts as well as authenticates.

## Where to go next

- [Adding it to your build](build-integration.md): the capability matrix, tunables, and build wiring.
- [Integrating with lwIP (Raw API)](platforms/lwipraw/setup.md), [Mbed TLS](platforms/mbedtls/setup.md), [FreeRTOS-Plus-FAT](platforms/plusfat/setup.md): worked ports of the networking, TLS, and file roles.
- [Naming conventions](NAMING.md) and [MISRA deviations](misra-deviations.md): the rules Tier 1/2 adapter code follows.
- [Error-event severity policy](error-severity.md): choosing the severity for your adapter's reports.
