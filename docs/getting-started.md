# Getting started

This is the integrator front door. It gets a real syslog stack compiling and
sending in your project, whether you build with CMake or drop the sources
straight into an IAR / Keil / MPLAB / CCS project or a hand-written Makefile.

> Building the library itself (presets, tests, CI)? That is the *contributor*
> path; see [builds.md](builds.md). This page is for consuming SolidSyslog
> in your product.

## How SolidSyslog composes

There is no monolith to subtract from. You assemble exactly the stack you need
from three layers:

1. Core: always present. The `SolidSyslog.h` API, the
   formatter/message pipeline, error reporting, the buffer / store / structured-data
   machinery, the static pool allocator, and a Null object for every role.
2. Adapters: one per role you want filled (network, TLS, OS mutex, file,
   atomic counter, …). Each is a small group of `.c` files under
   `Platform/<X>/`. Pick the provider that matches your platform.
3. Your callbacks and config: a few function pointers (clock, hostname,
   sleep) and your tunables.

Every role has a Core Null fallback, so omitting an adapter degrades
safely: its `_Create` is simply never called, and nothing dangles at link
time. You only compile the adapters you actually wire.

Two facts decide everything below:

- Non-CMake = everything is source. A cross-compiled IAR/Keil/CCS project
  compiles the Core sources and the selected adapter sources together, against
  your config headers. There is no prebuilt Core library for a cross build;
  Core must be built with your toolchain too. (The "Core is a prebuilt `.a`" idea
  is a CMake-consumer convenience only.)
- Some adapters are header-configured. lwIP (`lwipopts.h`), Mbed TLS
  (`mbedtls_config.h`), FreeRTOS (`FreeRTOSConfig.h`), and FatFs (`ffconf.h`)
  change types and behaviour through your config header, so their `.c` files
  must be compiled with that header on the include path; they can never be a
  one-size pre-build. That is why they ship as sources.

---

## Pick your stack — capability matrix

Choose one provider per role you need. Roles you don't need: leave the adapter
out and the Core Null object stands in.

*Files* lists the file group (`Solid…` prefix elided). Most adapters are
`Adapter.c` + `AdapterStatic.c` (the `Static` file is the instance pool) plus an
`AdapterPrivate.h`; compile the `.c` files, keep the directory on the include
path. *Pool tunable* is the `SOLIDSYSLOG_<NAME>` slot count (see
[Tunables](#tunables)).

### Networking

| Role | Provider | Files (`Solid…`) | Upstream / config | Pool tunable |
|---|---|---|---|---|
| Resolver | GetAddrInfo | `GetAddrInfoResolver*` | POSIX | `RESOLVER_POOL_SIZE` |
| | Winsock | `WinsockResolver*` | Win32 | ″ |
| | PlusTcp (DNS) | `PlusTcpResolver*` | FreeRTOS-Plus-TCP + `ipconfigUSE_DNS=1` | ″ |
| | LwipRaw numeric | `LwipRawResolver*` | lwIP `ipaddr_aton` | ″ |
| | LwipRaw DNS | `LwipRawDnsResolver*` | lwIP `LWIP_DNS=1` + Sleep cb | ″ |
| Datagram (UDP) | Posix / Winsock / PlusTcp / LwipRaw | `{Posix,Winsock,PlusTcp,LwipRaw}Datagram*` | resp. stacks | `DATAGRAM_POOL_SIZE` |
| Stream (TCP) | Posix / Winsock / PlusTcp / LwipRaw | `{…}TcpStream*` | resp. stacks (lwIP also needs a Sleep cb) | `TCP_STREAM_POOL_SIZE` |
| Address | Posix / Winsock / PlusTcp / LwipRaw | `{…}Address*` | resp. `sockaddr` | `ADDRESS_POOL_SIZE` |
| Marshal (lwIP) | LwipRawMarshal | `LwipRawMarshal` | lwIP, for `NO_SYS=0` | — |

### Transport security (TLS) and at-rest integrity

| Role | Provider | Files (`Solid…`) | Upstream / config | Pool tunable |
|---|---|---|---|---|
| TLS Stream | OpenSSL | `TlsStream*` | OpenSSL ≥ 3.0 | `TLS_STREAM_POOL_SIZE` |
| | Mbed TLS | `MbedTlsStream*` | `mbedtls_config.h` | ″ |
| SecurityPolicy | CRC-16 | `Crc16Policy*` + `Crc16` | — | — |
| | HMAC-SHA256 | `{OpenSsl,MbedTls}HmacSha256Policy*` | OpenSSL or Mbed TLS | `HMAC_SHA256_POLICY_POOL_SIZE` |
| | AES-GCM | `{OpenSsl,MbedTls}AesGcmPolicy*` | OpenSSL or Mbed TLS | `AES_GCM_POLICY_POOL_SIZE` |

### OS primitives, storage, structured data

| Role | Provider | Files (`Solid…`) | Upstream / config | Pool tunable |
|---|---|---|---|---|
| Mutex | Posix / Windows / FreeRtos | `{…}Mutex*` | FreeRtos needs `configSUPPORT_STATIC_ALLOCATION=1` | `MUTEX_POOL_SIZE` |
| AtomicCounter | C11 std / Windows | `StdAtomicCounter*` / `WindowsAtomicCounter*` | C11 `<stdatomic.h>` / Win32 | `ATOMIC_COUNTER_POOL_SIZE` |
| Buffer | Passthrough / Circular / Posix mq | `PassthroughBuffer*` / `CircularBuffer*` / `PosixMessageQueueBuffer*` | — / — / POSIX | resp. pool sizes |
| Store | BlockStore | `BlockStore*`, `RecordStore*`, `BlockSequence*` | — | `BLOCK_STORE_POOL_SIZE` |
| BlockDevice | FileBlockDevice | `FileBlockDevice*` | — | `FILE_BLOCK_DEVICE_POOL_SIZE` |
| File | Posix / Windows / FatFs / PlusFat | `{Posix,Windows}File*` / `FatFsFile*` / `PlusFatFile*` | FatFs: `ffconf.h` + `diskio.c`; PlusFat: `FreeRTOSFATConfig.h` + `FF_Disk_t` | `FILE_POOL_SIZE` |
| Structured Data | Meta / TimeQuality / Origin | `{Meta,TimeQuality,Origin}Sd*` | — | resp. pool sizes |

### Bring-your-own callbacks

A small tier is just function pointers. Host platforms ship a provider; embedded
targets supply a one-line callback. (Provider authoring for these is tracked in a
later epic, but the seams exist today.)

| Role | Host provider | Embedded |
|---|---|---|
| Clock | `{Posix,Windows}Clock` | BYO `SolidSyslogClockFunction` |
| SysUpTime | `{Posix,Windows,FreeRtos}SysUpTime` | FreeRtos provided; else BYO |
| Sleep | `{Posix,Windows}Sleep` | BYO (needed by TLS + lwIP) — e.g. a `vTaskDelay` wrapper |
| Hostname / ProcessId | `{Posix,Windows}…` | BYO header-field callbacks |
| AtomicCounter | C11 / Win32 (above) | BYO, else sequence-id degrades to Null (always 1) |

---

## Path A — CMake consumer

Name the platforms you want and link what they provide:

```cmake
set(SOLIDSYSLOG_PLATFORMS "FreeRtos;LwipRaw;MbedTls;FatFs")
FetchContent_MakeAvailable(SolidSyslog)
```

Named platforms are on, unnamed platforms are off — set it before
`FetchContent_MakeAvailable`, or pass `-DSOLIDSYSLOG_PLATFORMS=...` on the
command line. Each platform also has its own switch (`-DSOLIDSYSLOG_LWIP=ON`,
`-DSOLIDSYSLOG_PLUSTCP=OFF`, …) when you want to adjust one without restating
the list.

| Platform | Provides |
|---|---|
| `Posix` | network, file, mutex, buffer, clock, identity |
| `Windows` | network, file, mutex, atomics, clock, identity |
| `Atomics` | C11 atomic sequence-id counter |
| `OpenSsl` | TLS stream + HMAC / AES-GCM at-rest policies |
| `MbedTls` | TLS stream + HMAC / AES-GCM at-rest policies (embedded) |
| `LwipRaw` | network (lwIP Raw API) |
| `PlusTcp` | network (FreeRTOS-Plus-TCP) |
| `FreeRtos` | mutex, sysUpTime |
| `FatFs` | file (ChaN FatFs) |
| `PlusFat` | file (FreeRTOS-Plus-FAT) |

Every configure reports what it selected and what those platforms fill, so a
gap shows up as the thing it actually causes — a store with no file, a
CircularBuffer with no mutex:

```text
-- SolidSyslog platforms: LwipRaw;FreeRtos;MbedTls;FatFs
-- SolidSyslog roles:      network=LwipRaw  tls=MbedTls  file=FatFs  mutex=FreeRtos  atomics=(none)
```

Leave `SOLIDSYSLOG_PLATFORMS` unset and each platform falls back to its own
availability — a compile probe for the host ones, "the upstream tree is on the
environment" (`FREERTOS_KERNEL_PATH`, `LWIP_PATH`, `MBEDTLS_DIR`, `FATFS_PATH`,
`FREERTOS_PLUS_TCP_PATH`, `FREERTOS_PLUS_FAT_PATH`) for the rest. That is how
this repo's own containers work. It is a convenience, not the contract — your
build should not have to depend on how it was invoked.

### What you link

Platforms attach in one of two ways, and the rule is short: **if the upstream
needs your config header, you link it; otherwise it is already inside.**

Header-configured upstreams — lwIP, FreeRTOS, Plus-TCP, Plus-FAT, Mbed TLS,
FatFs — cannot be precompiled, because `lwipopts.h`, `FreeRTOSConfig.h`,
`mbedtls_config.h` and `ffconf.h` change layout and behaviour and we cannot see
your copy. Each is a `SolidSyslog::<Platform>` target carrying its adapter
sources, which compile into *your* target against *your* config.

Stable system APIs — POSIX, Win32/Winsock, OpenSSL, C11 atomics — have no such
hazard and compile straight into `libSolidSyslog.a`. Linking `SolidSyslog` is
all they need, which is why a Windows or Linux consumer writes one link line.

You never tell SolidSyslog where your upstream trees live — your own target
already puts `lwip/*.h`, `FreeRTOS.h`, `mbedtls/*.h` and `ff.h` on the include
path, and the SolidSyslog-side include dirs arrive with each platform target.

The lwIP DNS resolver is config-gated (it needs `LWIP_DNS=1`), so it sits
outside the `LwipRaw` umbrella as an opt-in component,
`SolidSyslog::LwipRawDnsResolver`; linking it also pulls `SolidSyslog::LwipRaw`.
A numeric-only lwIP build links `SolidSyslog::LwipRaw` and never enables DNS.

### A full embedded consumer

```cmake
cmake_minimum_required(VERSION 3.16)
project(my_logger C)

include(FetchContent)
FetchContent_Declare(SolidSyslog
    GIT_REPOSITORY https://github.com/cososo-ltd/solid-syslog.git
    GIT_TAG        main)

set(SOLIDSYSLOG_PLATFORMS "FreeRtos;LwipRaw;MbedTls;FatFs;Atomics")
FetchContent_MakeAvailable(SolidSyslog)

add_executable(my_logger main.c diskio.c)

target_link_libraries(my_logger PRIVATE
    SolidSyslog                        # core
    SolidSyslog::FreeRtos              # Mutex, SysUpTime
    SolidSyslog::LwipRawDnsResolver    # pulls ::LwipRaw too
    SolidSyslog::MbedTls               # TLS + at-rest policies
    SolidSyslog::FatFs                 # File
    freertos_kernel lwip mbedtls fatfs)  # yours — we ship adapters, not upstreams

target_include_directories(my_logger PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/config   # lwipopts.h, arch/cc.h, FreeRTOSConfig.h,
                                         # mbedtls_config.h, ffconf.h — all yours
    ${LWIP_DIR}/src/include
    ${FREERTOS_KERNEL_DIR}/include
    ${FREERTOS_KERNEL_DIR}/portable/GCC/ARM_CM4F
    ${MBEDTLS_DIR}/include
    ${FATFS_DIR}/source)
```

`Atomics` is named but never linked — it compiles the C11 counter into
`libSolidSyslog.a`. Naming it says "my toolchain has working `_Atomic`"; get it
wrong and the configure stops rather than silently degrading to the Null
counter.

### A host consumer

```cmake
set(SOLIDSYSLOG_PLATFORMS "Windows;OpenSsl")   # or "Posix;OpenSsl"
FetchContent_MakeAvailable(SolidSyslog)

add_executable(my_logger main.c)
target_link_libraries(my_logger PRIVATE SolidSyslog)
```

That is the whole thing — no config headers, no include paths, no upstream to
build. You can drop the `set()` entirely and let auto-detection find the same
stack; declare it when you would rather the build state its platforms than
infer them.

[`ci/consumer-smoke/`](../ci/consumer-smoke/) is a working consumer of this
shape, kept honest by CI: it cross-builds with the environment scrubbed, so
`SOLIDSYSLOG_PLATFORMS` is the only thing that can select a platform, and it
links them rather than merely checking the targets exist.

SolidSyslog also has `SOLIDSYSLOG_LWIP_PATH` and siblings. Those build
*SolidSyslog's own* unit tests and BDD targets against the real upstream
headers — a consumer never sets them.

See the worked target wiring in
[`Bdd/Targets/FreeRtos/`](../Bdd/Targets/FreeRtos/) and
[`Bdd/Targets/FreeRtosLwip/`](../Bdd/Targets/FreeRtosLwip/) (both consume the
umbrellas), and the platform-specific guides:

- [Integrating with lwIP (Raw API)](integrating-lwip.md)
- [Integrating with Mbed TLS](integrating-mbedtls.md)
- [Integrating with FreeRTOS-Plus-FAT](integrating-plusfat.md)

---

## Path B — non-CMake integrator (the manifest)

For an IAR / Keil / MPLAB / CCS native project or a hand-written Makefile, a
SolidSyslog integration is three things:

1. Source files: the Core `.c` set plus the selected adapter `.c` files. Add
   them to your project's source list / compile them in your Makefile.
2. Include directories, so the compiler finds both public and private
   headers:
   - `Core/Interface`: the public API headers.
   - `Core/Source`: Core-internal private headers (e.g.
     `SolidSyslogBlockStorePrivate.h`, `RecordStorePrivate.h`).
   - For every adapter you use: `Platform/<X>/Interface` and
     `Platform/<X>/Source` (adapter `.c` files include their own
     `*Private.h` from `Source/`).
   - Each upstream library's include dir (lwIP, Mbed TLS, FreeRTOS, FatFs).
   - The directory holding your config headers (`lwipopts.h`,
     `mbedtls_config.h`, `FreeRTOSConfig.h`, `ffconf.h`) and your tunables file.
3. Defines: any `-D` an adapter requires (e.g. `LWIP_DNS=1` only if you use
   the lwIP DNS resolver), plus your tunable overrides (see [Tunables](#tunables)).

> Tip: adapter file groups. For an adapter named `Foo`, compile every
> `Platform/<X>/Source/SolidSyslogFoo*.c` (the `Foo` + `FooStatic` pair) and put
> `Platform/<X>/Source` on the include path for the `FooPrivate.h` header.
> The matrix above lists which groups you need per role.

---

## Worked manifest — the beta stack

Target: FreeRTOS + lwIP + Mbed TLS + FatFs, IAR, no CMake. TLS transport,
store-and-forward, numeric resolver + DNS, `NO_SYS=0`.

### 1. Source files + include directories — generated, not hand-listed

The exact `.c` file list and SolidSyslog-side include directories for this stack
are generated from CMake and committed, so they can never drift from what the
packs actually ship (CI regenerates and fails on any difference):

→ [`docs/generated/beta-stack-manifest.txt`](generated/beta-stack-manifest.txt)

That file is the authoritative source/include/`-D`/config-header list; copy it
straight into your IDE or Makefile. To generate the manifest for a different
selection of packs, configure with your upstream trees on the environment and
your pack list, then build the `manifest` target:

```bash
cmake -S . -B build/manifest \
  -DSOLIDSYSLOG_MANIFEST_PACKS="LwipRaw;LwipRawDnsResolver;MbedTls;FreeRtos;FatFs"
cmake --build build/manifest --target manifest      # prints the manifest
```

Leave `SOLIDSYSLOG_MANIFEST_PACKS` empty to include every pack the configure
defined. The Core `.c` set is always included; the host Pattern-A adapters
(POSIX / Windows / OpenSSL / C11 atomics) are CMake-auto-detected host
conveniences and are intentionally omitted from the (embedded) manifest.

> The manifest lists the SolidSyslog-side include dirs only. You still add your
> own upstream include dirs (lwIP, Mbed TLS, FreeRTOS, FatFs) and the directory
> holding your config headers + `my_tunables.h`. FatFs also needs your own
> `diskio.c` media driver.

### 2. Defines

The generated manifest's *Required defines* section is authoritative. For this
stack:

```text
-DSOLIDSYSLOG_USER_TUNABLES_FILE="my_tunables.h"   # your tunable overrides
-DLWIP_DNS=1                                        # required by SolidSyslog::LwipRawDnsResolver
```

> `LWIP_DNS=1` is required because this stack includes the lwIP DNS resolver; a
> numeric-only build (omit `SolidSyslog::LwipRawDnsResolver`) does not need it.
> The header-configured upstreams take their other settings from your config
> headers, not from `-D`s: `lwipopts.h` (incl. `NO_SYS`, `LWIP_RAW`/`UDP`/`TCP`),
> `mbedtls_config.h`, `FreeRTOSConfig.h` (with
> `configSUPPORT_STATIC_ALLOCATION=1` for the mutex), `ffconf.h`.

### 3. Config headers you own

| Header | Owns |
|---|---|
| `lwipopts.h` | lwIP feature set + sizing (`NO_SYS`, raw/UDP/TCP, PBUF pools) |
| `mbedtls_config.h` | Mbed TLS feature set (ciphersuites, TLS 1.2+) |
| `FreeRTOSConfig.h` | FreeRTOS kernel config (`configSUPPORT_STATIC_ALLOCATION=1`) |
| `ffconf.h` | FatFs feature set |
| `my_tunables.h` | SolidSyslog pool sizes / limits (see below) |

### 4. Bring-your-own callbacks for this stack

- Sleep: required by Mbed TLS (handshake retry) and the lwIP TCP stream
  (bounded synchronous open). Wrap `vTaskDelay`.
- Clock, Hostname, ProcessId: small callbacks (see the matrix).
- AtomicCounter: only if you want RFC 5424 sequence-ids; otherwise it
  degrades to the Null counter (always 1).

For the exact wiring of each adapter's `_Create` config struct, follow the
platform guides: [lwIP](integrating-lwip.md), [Mbed TLS](integrating-mbedtls.md).

---

## Tunables

All compile-time limits live in
[`Core/Interface/SolidSyslogTunablesDefaults.h`](../Core/Interface/SolidSyslogTunablesDefaults.h),
multiple values, every one `#ifndef`-guarded so you override without editing the
library. Two equivalent mechanisms (works the same for CMake and non-CMake):

- A whole file of overrides:

  ```text
  -DSOLIDSYSLOG_USER_TUNABLES_FILE="my_tunables.h"
  ```

  Your `my_tunables.h` just `#define`s the values you want to change; the
  defaults header fills in the rest.

- Per-value on the command line:

  ```text
  -DSOLIDSYSLOG_MAX_MESSAGE_SIZE=1024
  ```

Pool-size tunables are named by role, not by platform (e.g.
`SOLIDSYSLOG_TCP_STREAM_POOL_SIZE`, not a per-vendor name): a build links one
implementation per role, so you size "how many TCP streams", never "how many
POSIX streams". The pool counts concurrent instances. See the header's
top-of-file comment for the full rationale.

---

## Your first log

The application-facing API is tiny: `Create` once at setup, then `Log` from
anywhere and `Service` from your drain loop:

```c
#include "SolidSyslog.h"
#include "SolidSyslogConfig.h"
/* Setup also includes one header per component it wires — e.g.
 * "SolidSyslogCircularBuffer.h", "SolidSyslogStreamSender.h",
 * "SolidSyslogBlockStore.h", "SolidSyslogPosixClock.h". Code that only
 * logs or drains (elsewhere in your app) includes just "SolidSyslog.h". */

/* 1. Build a config and create the logger (setup code).
 *    Wire your chosen Buffer, Sender, Store, clock and header-field
 *    callbacks into the config struct — the worked manifest and the
 *    platform guides show how to build each one for your stack. */
struct SolidSyslogConfig config = {
    .Buffer      = myBuffer,      /* e.g. SolidSyslogCircularBuffer_Create(...) */
    .Sender      = mySender,      /* e.g. SolidSyslogStreamSender_Create(...)   */
    .Store       = myStore,       /* e.g. SolidSyslogBlockStore_Create(...) or NULL */
    .Clock       = myClock,       /* SolidSyslogClockFunction */
    .GetHostname = myHostname,    /* SolidSyslogHeaderFieldFunction */
    .GetAppName  = myAppName,
    .GetProcessId = myProcessId,
    /* .Sd / .SdCount for per-instance structured data, optional */
};

struct SolidSyslog* logger = SolidSyslog_Create(&config);

/* 2. Log an event (anywhere in your application). */
struct SolidSyslogMessage message = {
    .Facility  = SOLIDSYSLOG_FACILITY_LOCAL0,
    .Severity  = SOLIDSYSLOG_SEVERITY_INFORMATIONAL,
    .MessageId = "BOOT",
    .Msg       = "system started",
};
SolidSyslog_Log(logger, &message);

/* 3. Drain the buffer to the network (your Service task / main loop). */
for (;;) {
    SolidSyslog_Service(logger);
    /* ... your scheduling / sleep ... */
}

/* 4. On shutdown (rare on embedded). */
SolidSyslog_Destroy(logger);
```

With a `PassthroughBuffer`, `Log` sends inline and the `Service` loop is a no-op.
With a `CircularBuffer` (the embedded default), `Log` enqueues and `Service`
drains; run `Service` from a dedicated task. To attach per-message structured
data, use `SolidSyslog_LogWithSd` instead of `SolidSyslog_Log`.

Code that logs an event or drains the queue (the `Service` loop) includes only
`SolidSyslog.h`. Building the logger is the other job: it includes
`SolidSyslogConfig.h` plus one header per component it wires (a Sender, a Buffer,
an optional Store, structured data, and the clock / hostname / app-name
callbacks), fills the config once, and hands the returned handle to the other
two. See the [API reference](api-reference/index.md) for the header-by-job map.

---

## Where to go next

- [Integrating with lwIP (Raw API)](integrating-lwip.md)
- [Integrating with Mbed TLS](integrating-mbedtls.md)
- [Integrating with FreeRTOS-Plus-FAT](integrating-plusfat.md)
- [Porting to a new platform](porting.md): writing an adapter for an OS, network stack, filesystem, or crypto library we don't ship
- [Structured data](structured-data.md)
- [Error handling and severity](error-severity.md)
- [IEC 62443 component selection by Security Level](iec62443.md)
- [RFC compliance matrix](rfc-compliance.md)
- [builds.md](builds.md): building/testing the library itself (contributors)
