# Getting started

This is the **integrator front door**. It gets a real syslog stack compiling and
sending in your project — whether you build with CMake or drop the sources
straight into an IAR / Keil / MPLAB / CCS project or a hand-written Makefile.

> Building the library itself (presets, tests, CI)? That is the *contributor*
> path — see [builds.md](builds.md). This page is for **consuming** SolidSyslog
> in your product.

## How SolidSyslog composes

There is no monolith to subtract from. You assemble exactly the stack you need
from three layers:

1. **Core** — always present. The `SolidSyslog.h` API, the
   formatter/message pipeline, error reporting, the buffer / store / structured-data
   machinery, the static pool allocator, and a **Null object for every role**.
2. **Adapters** — one per role you want filled (network, TLS, OS mutex, file,
   atomic counter, …). Each is a small group of `.c` files under
   `Platform/<X>/`. Pick the provider that matches your platform.
3. **Your callbacks and config** — a few function pointers (clock, hostname,
   sleep) and your tunables.

Every role has a Core **Null** fallback, so **omitting an adapter degrades
safely** — its `_Create` is simply never called, and nothing dangles at link
time. You only compile the adapters you actually wire.

Two facts decide everything below:

- **Non-CMake = everything is source.** A cross-compiled IAR/Keil/CCS project
  compiles the Core sources *and* the selected adapter sources together, against
  *your* config headers. There is no prebuilt Core library for a cross build —
  Core must be built with your toolchain too. (The "Core is a prebuilt `.a`" idea
  is a CMake-consumer convenience only.)
- **Some adapters are header-configured.** lwIP (`lwipopts.h`), Mbed TLS
  (`mbedtls_config.h`), FreeRTOS (`FreeRTOSConfig.h`), and FatFs (`ffconf.h`)
  change types and behaviour through *your* config header, so their `.c` files
  must be compiled with that header on the include path — they can never be a
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
| | PlusTcp (static IPv4) | `PlusTcpResolver*` | FreeRTOS-Plus-TCP | ″ |
| | LwipRaw numeric | `LwipRawResolver*` | lwIP `ipaddr_aton` | ″ |
| | LwipRaw **DNS** | `LwipRawDnsResolver*` | lwIP **`LWIP_DNS=1`** + Sleep cb | ″ |
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

Selection is auto-detect plus environment variables pointing at your upstream
trees:

- Host roles (POSIX / Winsock / OpenSSL / C11 atomics) are **auto-detected**
  (`find_package(OpenSSL)`, `check_symbol_exists`, an `_Atomic` compile probe)
  and baked into `libSolidSyslog.a`.
- Embedded upstreams are located by env var: `FREERTOS_KERNEL_PATH`, `LWIP_PATH`,
  `MBEDTLS_DIR`, `FATFS_PATH`, `FREERTOS_PLUS_FAT_PATH`.
- The FreeRTOS networking backend is chosen with
  `-DSOLIDSYSLOG_FREERTOS_NET=PLUSTCP|LWIP|BOTH`.

The header-configured packs ship as **namespaced umbrella targets** — link one
per platform and the adapter sources compile into *your* target against *your*
config header, with the SolidSyslog-side include dirs carried automatically (you
still point at your own upstream trees):

```cmake
target_link_libraries(my_app PRIVATE
    SolidSyslog::LwipRaw      # Address + Datagram + TcpStream + numeric Resolver + Marshal
    SolidSyslog::MbedTls      # TLS Stream + HMAC / AES-GCM at-rest policies
    SolidSyslog::FreeRtos     # Mutex + SysUpTime
    SolidSyslog::FatFs)       # FatFs file adapter
```

Available umbrellas: `SolidSyslog::FreeRtos`, `SolidSyslog::PlusTcp`,
`SolidSyslog::LwipRaw`, `SolidSyslog::MbedTls`, `SolidSyslog::FatFs`,
`SolidSyslog::PlusFat`. The lwIP **DNS** resolver is config-gated (needs
`LWIP_DNS=1`), so it sits outside the umbrella as an opt-in component —
`SolidSyslog::LwipRawDnsResolver` (linking it also pulls the `LwipRaw` umbrella).
A numeric-only lwIP build links `SolidSyslog::LwipRaw` and never enables DNS.

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
SolidSyslog integration is **three things**:

1. **Source files** — the Core `.c` set plus the selected adapter `.c` files. Add
   them to your project's source list / compile them in your Makefile.
2. **Include directories** — so the compiler finds both public and private
   headers:
   - `Core/Interface` — the public API headers.
   - `Core/Source` — Core-internal private headers (e.g.
     `SolidSyslogBlockStorePrivate.h`, `RecordStorePrivate.h`).
   - For every adapter you use: `Platform/<X>/Interface` **and**
     `Platform/<X>/Source` (adapter `.c` files include their own
     `*Private.h` from `Source/`).
   - Each upstream library's include dir (lwIP, Mbed TLS, FreeRTOS, FatFs).
   - The directory holding *your* config headers (`lwipopts.h`,
     `mbedtls_config.h`, `FreeRTOSConfig.h`, `ffconf.h`) and your tunables file.
3. **Defines** — any `-D` an adapter requires (e.g. `LWIP_DNS=1` only if you use
   the lwIP DNS resolver), plus your tunable overrides (see [Tunables](#tunables)).

> **Tip — adapter file groups.** For an adapter named `Foo`, compile every
> `Platform/<X>/Source/SolidSyslogFoo*.c` (the `Foo` + `FooStatic` pair) and put
> `Platform/<X>/Source` on the include path for the `FooPrivate.h` header.
> The matrix above lists which groups you need per role.

> *(This manifest is hand-authored against the current tree. S30.03 will
> generate it from CMake so it can never drift — see #569.)*

---

## Worked manifest — the beta stack

**Target:** FreeRTOS + lwIP + Mbed TLS + FatFs, IAR, **no CMake**. TLS transport,
store-and-forward, numeric (no-DNS) resolver, `NO_SYS=0`.

### 1. Source files to compile

**Core (all of it):**

```
Core/Source/*.c
```

> All 63 Core `.c` files. The sender group
> (`SolidSyslogUdpSender*`, `SolidSyslogStreamSender*`,
> `SolidSyslogSwitchingSender*`), the resolver/datagram dispatchers, and the
> atomic-counter dispatcher (`SolidSyslogAtomicCounter*`) are part of Core; CMake
> gates them by host probe, but a non-CMake build simply compiles the whole
> directory — unused objects are dead-stripped by the linker and every role has a
> Null fallback.

**Network — lwIP (numeric resolver; DNS omitted so no `LWIP_DNS` needed):**

```
Platform/LwipRaw/Source/SolidSyslogLwipRawAddress.c
Platform/LwipRaw/Source/SolidSyslogLwipRawAddressStatic.c
Platform/LwipRaw/Source/SolidSyslogLwipRawDatagram.c          # only if you send UDP
Platform/LwipRaw/Source/SolidSyslogLwipRawDatagramStatic.c    # only if you send UDP
Platform/LwipRaw/Source/SolidSyslogLwipRawTcpStream.c
Platform/LwipRaw/Source/SolidSyslogLwipRawTcpStreamStatic.c
Platform/LwipRaw/Source/SolidSyslogLwipRawResolver.c
Platform/LwipRaw/Source/SolidSyslogLwipRawResolverStatic.c
Platform/LwipRaw/Source/SolidSyslogLwipRawMarshal.c           # NO_SYS=0 only
```

> Omit `SolidSyslogLwipRawDnsResolver*` (that is what keeps `LWIP_DNS` off the
> requirement list).

**TLS — Mbed TLS (over the lwIP TCP stream):**

```
Platform/MbedTls/Source/SolidSyslogMbedTlsStream.c
Platform/MbedTls/Source/SolidSyslogMbedTlsStreamStatic.c
```

> Optional at-rest integrity: add
> `SolidSyslogMbedTlsHmacSha256Policy*` and/or `SolidSyslogMbedTlsAesGcmPolicy*`.

**OS — FreeRTOS:**

```
Platform/FreeRtos/Source/SolidSyslogFreeRtosMutex.c
Platform/FreeRtos/Source/SolidSyslogFreeRtosMutexStatic.c
Platform/FreeRtos/Source/SolidSyslogFreeRtosSysUpTime.c
```

**Store — FatFs (crash-safe persistent store-and-forward):**

```
Platform/FatFs/Source/SolidSyslogFatFsFile.c
Platform/FatFs/Source/SolidSyslogFatFsFileStatic.c
```

> Plus your own `diskio.c` media driver (FatFs is RTOS-agnostic; you supply the
> block device). `BlockStore` / `RecordStore` / `BlockSequence` /
> `FileBlockDevice` are already in the Core set above.

### 2. Include directories

```
Core/Interface
Core/Source
Platform/LwipRaw/Interface
Platform/LwipRaw/Source
Platform/MbedTls/Interface
Platform/MbedTls/Source
Platform/FreeRtos/Interface
Platform/FreeRtos/Source
Platform/FatFs/Interface
Platform/FatFs/Source
<your lwIP include dirs>
<your Mbed TLS include dir>
<your FreeRTOS-Kernel include dir + port>
<your FatFs include dir>
<dir holding your config headers + my_tunables.h>
```

### 3. Defines

```
-DSOLIDSYSLOG_USER_TUNABLES_FILE="my_tunables.h"   # your tunable overrides
```

> No `LWIP_DNS` is required for this stack (numeric resolver). The header-configured
> upstreams take their settings from your config headers, not from `-D`s:
> `lwipopts.h` (incl. `NO_SYS`, `LWIP_RAW`/`UDP`/`TCP`), `mbedtls_config.h`,
> `FreeRTOSConfig.h` (with `configSUPPORT_STATIC_ALLOCATION=1` for the mutex),
> `ffconf.h`.

### 4. Config headers you own

| Header | Owns |
|---|---|
| `lwipopts.h` | lwIP feature set + sizing (`NO_SYS`, raw/UDP/TCP, PBUF pools) |
| `mbedtls_config.h` | Mbed TLS feature set (ciphersuites, TLS 1.2+) |
| `FreeRTOSConfig.h` | FreeRTOS kernel config (`configSUPPORT_STATIC_ALLOCATION=1`) |
| `ffconf.h` | FatFs feature set |
| `my_tunables.h` | SolidSyslog pool sizes / limits (see below) |

### 5. Bring-your-own callbacks for this stack

- **Sleep** — required by Mbed TLS (handshake retry) and the lwIP TCP stream
  (bounded synchronous open). Wrap `vTaskDelay`.
- **Clock**, **Hostname**, **ProcessId** — small callbacks (see the matrix).
- **AtomicCounter** — only if you want RFC 5424 sequence-ids; otherwise it
  degrades to the Null counter (always 1).

For the exact wiring of each adapter's `_Create` config struct, follow the
platform guides: [lwIP](integrating-lwip.md), [Mbed TLS](integrating-mbedtls.md).

---

## Tunables

All compile-time limits live in
[`Core/Interface/SolidSyslogTunablesDefaults.h`](../Core/Interface/SolidSyslogTunablesDefaults.h)
— 34 values, every one `#ifndef`-guarded so you override without editing the
library. Two equivalent mechanisms (works the same for CMake and non-CMake):

- **A whole file of overrides:**

  ```
  -DSOLIDSYSLOG_USER_TUNABLES_FILE="my_tunables.h"
  ```

  Your `my_tunables.h` just `#define`s the values you want to change; the
  defaults header fills in the rest.

- **Per-value on the command line:**

  ```
  -DSOLIDSYSLOG_MAX_MESSAGE_SIZE=1024
  ```

Pool-size tunables are named **by role, not by platform** (e.g.
`SOLIDSYSLOG_TCP_STREAM_POOL_SIZE`, not a per-vendor name) — a build links one
implementation per role, so you size "how many TCP streams", never "how many
*POSIX* streams". The pool counts concurrent *instances*. See the header's
top-of-file comment for the full rationale.

---

## Your first log

The application-facing API is tiny — `Create` once at setup, then `Log` from
anywhere and `Service` from your drain loop:

```c
#include "SolidSyslog.h"
#include "SolidSyslogConfig.h"

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
drains — run `Service` from a dedicated task. To attach per-message structured
data, use `SolidSyslog_LogWithSd` instead of `SolidSyslog_Log`.

Application code only ever includes `SolidSyslog.h` (and `SolidSyslogConfig.h` at
setup). Everything else — senders, buffers, TLS, stores — is wired once behind
the config struct.

---

## Where to go next

- [Integrating with lwIP (Raw API)](integrating-lwip.md)
- [Integrating with Mbed TLS](integrating-mbedtls.md)
- [Integrating with FreeRTOS-Plus-FAT](integrating-plusfat.md)
- [Structured data](structured-data.md)
- [Error handling and severity](error-severity.md)
- [IEC 62443 component selection by Security Level](iec62443.md)
- [RFC compliance matrix](rfc-compliance.md)
- [builds.md](builds.md) — building/testing the library itself (contributors)
