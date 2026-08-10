# Adding it to your build

This page gets SolidSyslog compiling in your project. It covers the three ways to
consume it — CMake, Make, and a source manifest for an IDE project — along with the
adapters available for each role and the compile-time tunables.

For what to wire and why, in the order an integration is usually built up, see
[building up the protection you need](hardening-path.md). This page is the build
detail behind it.

> Building the library itself (presets, tests, CI)? That is the *contributor*
> path; see [builds.md](builds.md). This page is for consuming SolidSyslog
> in your product.

## How SolidSyslog composes

You assemble the stack you need from three layers:

1. Core: always present. The `SolidSyslog.h` API, the
   formatter/message pipeline, error reporting, the buffer / store / structured-data
   machinery, the static pool allocator, and a Null object for every role.
2. Adapters: one per role you want filled (network, TLS, OS mutex, file,
   atomic counter, …). Each is a small group of `.c` files under
   `Platform/<X>/`. Pick the provider that matches your platform.
3. Your callbacks and config: a few function pointers (clock, hostname,
   sleep) and your tunables.

Every role has a Core Null fallback, so omitting an adapter degrades
safely: its `<Class>_Create` is simply never called, and nothing dangles at link
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

## Pick your stack

Choose one provider per role you need; leave the rest out and Core's Null
object stands in. Which platform fills which role is the
[platform × capability matrix](platforms/index.md), and each platform's own
page states what its adapters need from your build.

Two things are build-specific rather than platform-specific, and belong here.

**The file list is generated, not written.** Every platform publishes a
manifest of the exact `.c` files to compile and the include directories they
need — see [Worked manifest](#worked-manifest--the-beta-stack) below. CI
regenerates these and fails on any difference, so they cannot drift from the
tree the way a hand-maintained table would.

**Some roles are callbacks rather than components.** The clock, the host name,
the process id and a bounded sleep are function pointers on
`SolidSyslogConfig`. Hosted platforms ship one of each ready to use; on a bare
target you write them, and they are usually a line apiece. A sleep is required
by the TLS adapters and by the lwIP TCP stream, which is the one that catches
people, because it has no default.

---

## CMake

Name the platforms you want and link what they provide:

```cmake
set(SOLIDSYSLOG_PLATFORMS "FreeRtos;LwipRaw;MbedTls;FatFs")
FetchContent_MakeAvailable(SolidSyslog)
```

Named platforms are on, unnamed platforms are off — set it before
`FetchContent_MakeAvailable`, or pass `-DSOLIDSYSLOG_PLATFORMS=...` on the
command line. Each platform also has its own switch (`-DSOLIDSYSLOG_LWIPRAW=ON`,
`-DSOLIDSYSLOG_PLUSTCP=OFF`, …) when you want to adjust one without restating
the list.

| Platform | Roles filled | Backed by |
|---|---|---|
| `Posix` | network, file, mutex, clock | POSIX sockets, `pthread`, `mqueue` |
| `Windows` | network, file, mutex, atomics, clock | Winsock, `CRITICAL_SECTION`, Win32 |
| `StdAtomic` | atomics | C11 `<stdatomic.h>` |
| `OpenSsl` | tls | OpenSSL 3.0+ |
| `MbedTls` | tls | Mbed TLS |
| `LwipRaw` | network | lwIP Raw API |
| `PlusTcp` | network | FreeRTOS-Plus-TCP |
| `FreeRtos` | mutex, clock | FreeRTOS kernel |
| `FatFs` | file | ChaN FatFs |
| `PlusFat` | file | FreeRTOS-Plus-FAT |

`Posix` and `Windows` each carry more than the roles above — hostname and
process-id callbacks, a sleep wrapper, and on POSIX a message-queue buffer.
Those are extras rather than roles, because nothing forces you to pick a
platform for them: the core ships buffers of its own, and the callbacks have
bring-your-own seams.

Every configure reports what it selected and what those platforms fill, so a
gap shows up as the thing it actually causes — a store with no file, a
CircularBuffer with no mutex:

```text
-- SolidSyslog platforms: MbedTls;LwipRaw;FreeRtos;FatFs
-- SolidSyslog roles:      network=LwipRaw  file=FatFs  mutex=FreeRtos  clock=FreeRtos  atomics=(none)  tls=MbedTls
```

Platforms are listed in registry order, not the order you named them.

The variable takes three kinds of answer:

| Value | Selects |
|---|---|
| `Auto` (the default) | the host platforms this toolchain can provide, decided by a compile probe: `Posix`, `Windows`, `StdAtomic`, `OpenSsl` |
| `""` | nothing — Core alone, for an integrator supplying every adapter themselves |
| a list | exactly those, host or upstream |

`LwipRaw`, `PlusTcp`, `FreeRtos`, `MbedTls`, `FatFs` and `PlusFat` are never
selected for you: you name them or you do not get them. Setting
`LWIP_PATH` or `FREERTOS_KERNEL_PATH` in your environment does not change what
your build contains.

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

The lwIP DNS resolver needs `LWIP_DNS=1`, and gates its own translation units on
it (S33.01). It ships inside `SolidSyslog::LwipRaw` like every other lwIP
adapter: a numeric-only build links the same target and compiles the resolver to
nothing. See [porting.md](porting.md#depending-on-upstream-configuration).

### A full embedded consumer

```cmake
cmake_minimum_required(VERSION 3.16)
project(my_logger C)

include(FetchContent)
FetchContent_Declare(SolidSyslog
    GIT_REPOSITORY https://github.com/cososo-ltd/solid-syslog.git
    GIT_TAG        main)

set(SOLIDSYSLOG_PLATFORMS "FreeRtos;LwipRaw;MbedTls;FatFs;StdAtomic")
FetchContent_MakeAvailable(SolidSyslog)

add_executable(my_logger main.c diskio.c)

target_link_libraries(my_logger PRIVATE
    SolidSyslog                        # core
    SolidSyslog::FreeRtos              # Mutex, SysUpTime
    SolidSyslog::LwipRaw               # Datagram, TcpStream, Resolver (+ DNS)
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

`StdAtomic` is named but never linked — it compiles the C11 counter into
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

`ci/consumer-smoke/` is a working consumer of this
shape, kept honest by CI: it cross-builds with the environment scrubbed, so
`SOLIDSYSLOG_PLATFORMS` is the only thing that can select a platform, and it
links them rather than merely checking the targets exist.

SolidSyslog also has `SOLIDSYSLOG_LWIP_PATH` and siblings. Those build
*SolidSyslog's own* unit tests and BDD targets against the real upstream
headers — a consumer never sets them.

See the worked target wiring in
`Bdd/Targets/FreeRtos/` and
`Bdd/Targets/FreeRtosLwip/` (both consume the
umbrellas), and the platform-specific guides:

- [Integrating with lwIP (Raw API)](platforms/lwipraw/setup.md)
- [Integrating with Mbed TLS](platforms/mbedtls/setup.md)
- [Integrating with FreeRTOS-Plus-FAT](platforms/plusfat/setup.md)

---

## Make

The library ships `solidsyslog.mk` at its root. Name the platforms you want and include
it:

```make
SOLIDSYSLOG_PLATFORMS := LwipRaw StdAtomic FreeRtos MbedTls FatFs
include third_party/solid-syslog/solidsyslog.mk
```

It sets five variables and defines no rules, so your build keeps its own object layout,
flag sets and archive step:

| Variable | Holds |
|---|---|
| `SOLIDSYSLOG_CORE_SRCS` | the Core `.c` set |
| `SOLIDSYSLOG_PLATFORM_SRCS` | the `.c` files of the platforms you named |
| `SOLIDSYSLOG_SRCS` | both of the above |
| `SOLIDSYSLOG_CORE_INCLUDES` | the include set for compiling Core |
| `SOLIDSYSLOG_INCLUDES` | the include set for the platform sources, and for your own code calling the library |

`SOLIDSYSLOG_PLATFORMS` takes the same vocabulary as the CMake variable of that name.
Naming no platform yields Core alone. Naming one that does not exist is an error that
lists the ones that do, rather than a build that quietly omits every adapter it was
supposed to bring.

Core and the platform sources are listed separately because they compile differently.
Core builds against the library's own headers; the platform sources must see your flags
and your config headers (`lwipopts.h`, `FreeRTOSConfig.h`, `mbedtls_config.h`,
`ffconf.h`), for the reason given in [how SolidSyslog composes](#how-solidsyslog-composes)
above. A typical consumer therefore compiles the two groups with different flag sets and
archives Core on its own:

<!-- markdownlint-disable MD010 — a Make recipe line must begin with a literal tab, so the snippet carries one for a reader who copies it. -->

```make
# Yours, not the fragment's: it sets no object lists and defines no rules.
SOLIDSYSLOG_CORE_OBJS     := $(SOLIDSYSLOG_CORE_SRCS:%.c=$(OBJ_DIR)/%.o)
SOLIDSYSLOG_USER_TUNABLES := -DSOLIDSYSLOG_USER_TUNABLES_FILE=\"$(CURDIR)/config/solid_syslog_tunables.h\"

$(SOLIDSYSLOG_CORE_OBJS): CFLAGS := $(COMMON_CFLAGS) $(SOLIDSYSLOG_CORE_INCLUDES) $(SOLIDSYSLOG_USER_TUNABLES)

$(BUILD)/libSolidSyslog.a: $(SOLIDSYSLOG_CORE_OBJS)
	$(AR) rcs $@ $^
```

<!-- markdownlint-enable MD010 -->

There is no separate link step and no distinction between platforms that are named and
platforms that are linked. That distinction exists only for CMake consumers, who receive
some platforms inside `libSolidSyslog.a`; a Make build has no such library, so naming
`StdAtomic` compiles its sources exactly as naming `LwipRaw` compiles its own.

Each adapter gates itself on the upstream option it needs, so naming a platform is safe
whatever your configuration. A build with `LWIP_DNS=0` compiles the lwIP DNS resolver to
nothing rather than failing, and nothing has to be excluded by hand.

Set `SOLIDSYSLOG_DIR` before the `include` if the library does not sit where the fragment
does — by default it locates itself.

[`solid-syslog-example-make`](https://github.com/cososo-ltd/solid-syslog-example-make) is
a worked consumer of this shape, cross-building FreeRTOS, lwIP, Mbed TLS and FatFs.

---

## IDE and manifest builds

For an IAR / Keil / MPLAB / CCS native project, a SolidSyslog integration is three
things:

1. Source files: the Core `.c` set plus the selected adapter `.c` files. Add
   them to your project's source list / compile them in your Makefile.
2. Include directories, so the compiler finds both public and private
   headers:
   - `Core/Interface`: the public API headers.
   - `Core/Source`: Core-internal private headers (e.g.
     `SolidSyslogBlockStorePrivate.h`, `SolidSyslogRecordStorePrivate.h`).
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
packs actually ship (CI regenerates and fails on any difference, and separately
asserts every listed file group matches the directory it comes from):

→ [`docs/generated/beta-stack-manifest.txt`](generated/beta-stack-manifest.txt)

That file is the authoritative source/include/`-D`/language/config-header list;
copy it straight into your IDE or Makefile. To generate the manifest for a
different stack, configure with your platform list and build the `manifest`
target:

```bash
cmake -S . -B build/manifest \
  -DSOLIDSYSLOG_PLATFORMS="LwipRaw;MbedTls;FreeRtos;FatFs;StdAtomic" \
  -DSOLIDSYSLOG_MANIFEST_PLATFORMS="LwipRaw;MbedTls;FreeRtos;FatFs;StdAtomic"
cmake --build build/manifest --target manifest      # prints the manifest
```

`SOLIDSYSLOG_MANIFEST_PLATFORMS` takes the same vocabulary as
`SOLIDSYSLOG_PLATFORMS`: `Auto` describes every platform this configuration
selected, empty describes none. The Core `.c` set is always included.

Platforms selected by a toolchain capability probe — `StdAtomic`, `Posix`,
`Windows`, `OpenSsl` — get their own section. A CMake consumer receives them
inside `libSolidSyslog.a`, but a manifest build has no such library, so compile
them alongside everything else. `StdAtomic` is the one that matters on a
cross-toolchain: without it there is no atomic counter, and every RFC 5424
`sequenceId` is `1`.

> The manifest lists the SolidSyslog-side include dirs only. You still add your
> own upstream include dirs (lwIP, Mbed TLS, FreeRTOS, FatFs) and the directory
> holding your config headers + `my_tunables.h`. FatFs also needs your own
> `diskio.c` media driver.

### 2. Defines

The generated manifest's *Required defines* section names the one define
SolidSyslog itself takes, then notes the upstream options its adapters gate on —
those it describes in prose rather than emitting, because they are yours to set.
For this stack that comes to:

```text
-DSOLIDSYSLOG_USER_TUNABLES_FILE="/abs/path/to/my_tunables.h"   # your tunable overrides
-DLWIP_DNS=1                                        # enables the lwIP DNS resolver
```

> `LWIP_DNS=1` is required because this stack resolves the collector by name; a
> numeric-only build compiles the DNS resolver to nothing and does not need it.
> The header-configured upstreams take their other settings from your config
> headers, not from `-D`s: `lwipopts.h` (incl. `NO_SYS`, `LWIP_RAW`/`UDP`/`TCP`),
> `mbedtls_config.h`, `FreeRTOSConfig.h` (with
> `configSUPPORT_STATIC_ALLOCATION=1` for the mutex), `ffconf.h`.

### 3. Language standard

The manifest's *Language* section states the floor and any platform that raises
it. The library is C99; the `StdAtomic` platform uses `<stdatomic.h>` and needs
C11. Anything at or above that works — C11, C17 and C23 are all fine — so the
manifest names the standard, not a `-std=` flag.

That floor applies to compiling the library, not to your code. The public
headers compile as ISO C89, so an application on an older toolchain can include
them and link a library built at C99. The `build-linux-c89-headers` lane
compiles every public header standalone at `-std=c89 -pedantic-errors` on each
pull request, so this is checked rather than asserted; run it yourself with
`scripts/check_headers_c89.py`. Compiling each header on its own also means
none of them depends on another being included first.

One caveat, and it is about the library rather than the language: the headers
use `<stdint.h>` and `<stdbool.h>`, which are C99 *library* headers a strict C89
implementation need not supply. In practice toolchains of that vintage ship
both, but if yours does not, that — and not the language — is what will stop
you.

### 4. Config headers you own

| Header | Owns |
|---|---|
| `lwipopts.h` | lwIP feature set + sizing (`NO_SYS`, raw/UDP/TCP, PBUF pools) |
| `mbedtls_config.h` | Mbed TLS feature set (ciphersuites, TLS 1.2+) |
| `FreeRTOSConfig.h` | FreeRTOS kernel config (`configSUPPORT_STATIC_ALLOCATION=1`) |
| `ffconf.h` | FatFs feature set |
| `my_tunables.h` | SolidSyslog pool sizes / limits (see below) |

### 5. Bring-your-own callbacks for this stack

- Sleep: required by Mbed TLS (handshake retry) and the lwIP TCP stream
  (bounded synchronous open). Wrap `vTaskDelay`.
- Clock, Hostname, ProcessId: small callbacks you supply.
- AtomicCounter: only if you want RFC 5424 sequence-ids.

For the exact wiring of each adapter's `<Class>_Create` config struct, follow
that platform's own setup guide, reached from its page in
[Platforms](platforms/index.md).

---

## Tunables

All compile-time limits live in
[`Core/Interface/SolidSyslogTunablesDefaults.h`](api/SolidSyslogTunablesDefaults_8h.md),
multiple values, every one `#ifndef`-guarded so you override without editing the
library. Two equivalent mechanisms (works the same for CMake and non-CMake):

- A whole file of overrides:

  ```text
  -DSOLIDSYSLOG_USER_TUNABLES_FILE="/abs/path/to/my_tunables.h"
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

### The override must reach every translation unit

Several tunables are compile-time sizes inside library structs. A build where only some
translation units saw the override would disagree about how large those structs are, and
it would link without complaint before misbehaving at run time.

Apply the define to Core, to the platform sources, and to your own code that includes a
SolidSyslog header — all three, with the same value. A CMake consumer gets this from the
library, which propagates `SOLIDSYSLOG_USER_TUNABLES_FILE` to everything that needs it.
A Make or manifest consumer applies it themselves, and this is the most common way to get
a tunables setup wrong.

If you use `SOLIDSYSLOG_USER_TUNABLES_FILE`, its value is consumed by `#include`, so give
an absolute path and escape the quotes. Core compiles without your application's include
path, so a bare filename will not be found:

```make
SOLIDSYSLOG_USER_TUNABLES := -DSOLIDSYSLOG_USER_TUNABLES_FILE=\"$(CURDIR)/$(APP_DIR)/config/solid_syslog_tunables.h\"
```

## Where to go next

- [Building up the protection you need](hardening-path.md): what to wire and why, stage by stage
- [Platforms](platforms/index.md): which platform fills which role, what each needs from your build, and how to wire it
- [Porting to a new platform](porting.md): writing an adapter for an OS, network stack, filesystem, or crypto library we don't ship
- [Structured data](structured-data.md)
- [Error handling and severity](error-severity.md)
- [IEC 62443 guide](iec62443.md)
- [RFC compliance matrix](rfc-compliance.md)
- [builds.md](builds.md): building/testing the library itself (contributors)
