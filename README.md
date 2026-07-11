# SolidSyslog

A structured syslog client library for embedded and industrial systems, implementing
RFC 5424 (structured syslog) with RFC 5426 (UDP) and RFC 6587 (TCP) transports.
TLS per RFC 5425 is available via a pluggable Stream abstraction — the repo ships
a reference OpenSSL integration, and callers can plug in any TLS library (wolfSSL,
mbedTLS, hardware-offload, …) by implementing the same Stream vtable. TLS itself
is not a core dependency; Core has zero OpenSSL references.

Designed for resource-constrained environments:

- C99, no dynamic memory allocation required — allocator is caller-injected
- Transport-agnostic — UDP, TCP, TLS, or bring your own
- Buffer-agnostic — PassthroughBuffer (direct send), portable CircularBuffer (mutex-injected ring), POSIX message queue, or bring your own
- No `#ifdef` feature flags — optional features composed at link time
- MISRA C:2012 informed
- Dependency injection throughout — fully testable without a network

## Capabilities

RFC 5424 structured formatting over UDP (RFC 5426), TCP (RFC 6587), and TLS /
mutual TLS (RFC 5425). Asynchronous buffering, rotating block store-and-forward,
and at-rest record protection — CRC-16 for accidental corruption, or keyed
HMAC-SHA256 / AES-256-GCM where a local attacker is in scope. The full
[IEC 62443 SL1–SL4 component set](docs/iec62443.md) is available.

POSIX and Windows are first-class hosts. FreeRTOS on Cortex-M carries the same
UDP / TCP / TLS / mTLS transports — networking via FreeRTOS-Plus-TCP or lwIP,
transport security via `SolidSyslogMbedTlsStream` (Mbed TLS) — with persistent
store-and-forward over ChaN FatFs or FreeRTOS-Plus-FAT.

The library is pre-1.0: the public API may still change and carries no stability
guarantee yet. TLS revocation (CRL / OCSP) is delegated to the platform trust
store rather than performed by the library.

## Getting started

New to SolidSyslog? Start at [Getting started](docs/getting-started.md) — the
integrator front door. It covers picking your stack from the capability matrix,
both consumption paths (CMake and non-CMake / IAR / Keil source integration), a
copy-pasteable manifest for an embedded stack, the tunables, and a minimal "your
first log" walkthrough.

## Building and testing

Developing the library itself? See [Building and testing](docs/builds.md) — the
contributor/maintainer preset catalogue. (Consuming the library in your product
is the [Getting started](docs/getting-started.md) path above.)

## Architecture

SolidSyslog uses an OO-in-C style with vtable structs and dependency injection.
All fields — required and optional — use a uniform field object pattern.
Optional features are composed at link time via dead code elimination; there are
no conditional compilation directives in the library source.

Public headers are split by audience (Interface Segregation Principle):

- **`SolidSyslog.h`** — application code that logs events (`Log`, `Service`)
- **`SolidSyslogConfig.h`** — system setup code that creates and destroys loggers
- **`SolidSyslogError.h`** — install a handler to react to library-internal errors (NULL guards, send failures); default is silent. See `Bdd/Targets/Common/BddTargetStderrErrorHandler.c` for a reference implementation
- **`SolidSyslogConfigLock.h`** — optional setup-time lock injection (`SolidSyslog_SetConfigLock(lockFn, unlockFn)`); wraps library-internal pool slot walks so multi-task setup is safe. Defaults are no-ops, so single-task systems can ignore it. Integrators on RTOS / multi-core wire `taskENTER_CRITICAL`, a static `pthread_mutex_t`, `EnterCriticalSection`, or a spinlock pair
- **`SolidSyslogSenderDefinition.h`** / **`SolidSyslogBufferDefinition.h`** — extension points for custom senders and buffers
- **`SolidSyslogPassthroughBuffer.h`** — direct-send buffer for single-task systems
- **`SolidSyslogCircularBuffer.h`** — portable ring buffer with caller-allocated ring memory and an injected `SolidSyslogMutex` (`SolidSyslogPosixMutex` / `SolidSyslogWindowsMutex` / `SolidSyslogFreeRtosMutex` / `SolidSyslogNullMutex` / your own); the cross-platform threaded buffer. Instance bookkeeping lives in a library-internal static pool
- **`SolidSyslogPosixMessageQueueBuffer.h`** — thread-safe POSIX message queue buffer
- **`SolidSyslogUdpSender.h`** — UDP transport (RFC 5426)
- **`SolidSyslogStreamSender.h`** — octet-framed syslog (RFC 6587) over any Stream. Note: RFC 6587
  is a Historic RFC — the IESG recommends TLS (RFC 5425) over plain TCP for new deployments.
  TCP is provided for interoperability with existing infrastructure
- **`SolidSyslogTlsStream.h`** — OpenSSL-backed TLS 1.2+ Stream (RFC 5425): server cert validation,
  hostname verification, cipher pinning, optional mutual TLS. Plugs into `SolidSyslogStreamSender`
  as a drop-in for `SolidSyslogPosixTcpStream`
- **`SolidSyslogSwitchingSender.h`** — composition sender delegating to one of several
  inner senders via an application-supplied selector callback; `Disconnect`s the
  outgoing inner on every change
- **`SolidSyslogEndpoint.h`** — destination spec for senders. Application supplies `endpoint`
  (fills host/port on (re)connect) and `endpointVersion` (cheap polled fingerprint); senders
  Disconnect and lazily reopen when the version changes — supports runtime address rotation
- **`SolidSyslogStoreDefinition.h`** / **`SolidSyslogBlockStore.h`** — BlockDevice-backed store-and-forward with rotating blocks
- **`SolidSyslogSecurityPolicyDefinition.h`** — extension point for record integrity policies
- **`SolidSyslogCrc16Policy.h`** — CRC-16/CCITT-FALSE integrity policy
- **`SolidSyslogStructuredDataDefinition.h`** — extension point for custom structured data
- **`SolidSyslogMetaSd.h`** — meta structured data (RFC 5424 §7.3): sequenceId, sysUpTime, language
- **`SolidSyslogTimeQualitySd.h`** — timeQuality structured data (RFC 5424 §7.1): tzKnown, isSynced, syncAccuracy
- **`SolidSyslogOriginSd.h`** — origin structured data (RFC 5424 §7.2): software, swVersion, enterpriseId, ip
- **`SolidSyslogPosixClock.h`** / **`SolidSyslogPosixHostname.h`** / **`SolidSyslogPosixProcessId.h`** / **`SolidSyslogPosixSysUpTime.h`** — POSIX helpers
- **`SolidSyslogPlusTcpDatagram.h`** / **`SolidSyslogPlusTcpTcpStream.h`** / **`SolidSyslogPlusTcpResolver.h`** / **`SolidSyslogPlusTcpAddress.h`** — FreeRTOS-Plus-TCP networking adapters: UDP datagram and TCP stream (with ARP-prime on cold connect and bounded `SO_RCVTIMEO` connect), hardcoded-IPv4 resolver, `struct freertos_sockaddr` address adapter. Selected at CMake time via `SOLIDSYSLOG_FREERTOS_NET=PLUSTCP` (an lwIP Raw sibling backend is selected with `LWIP`)
- **`SolidSyslogFreeRtosMutex.h`** / **`SolidSyslogFreeRtosSysUpTime.h`** — FreeRTOS kernel primitives independent of the chosen networking backend: `xSemaphoreCreateMutexStatic`-backed mutex for CircularBuffer and a kernel-tick sysUpTime source
- **`SolidSyslogFatFsFile.h`** — ChaN FatFs file adapter for the `SolidSyslogFile` extension point; `f_sync` per write for crash-safe store-and-forward. RTOS-agnostic; runs on bare-metal, FreeRTOS, Zephyr, NuttX

Three BDD-driven target binaries exercise the library on each supported
platform. They live under [`Bdd/Targets/`](Bdd/Targets/) — one binary
per platform, all named `SolidSyslogBddTarget`:

- **`Bdd/Targets/Linux/`** — POSIX, PosixMessageQueueBuffer, two pthreads (logger + service), SwitchingSender over UDP + TCP + TLS + mTLS (OpenSSL); `--transport` sets the initial transport, `switch <name>` flips it at runtime
- **`Bdd/Targets/Windows/`** — Windows, CircularBuffer + WindowsMutex, Win32 service thread (`_beginthreadex`) draining the buffer, SwitchingSender over Winsock UDP / TCP + OpenSSL TLS / mTLS over Winsock TCP, file-backed `SolidSyslogBlockStore` over `SolidSyslogWindowsFile`, with the Windows clock / hostname / process-id / sysUpTime helpers
- **`Bdd/Targets/FreeRtos/`** — FreeRTOS-on-QEMU (Cortex-M3, mps2-an385), CircularBuffer + FreeRtosMutex drained by a dedicated Service task, SwitchingSender over UDP + TCP via FreeRTOS-Plus-TCP plus TLS + mTLS via `SolidSyslogMbedTlsStream` (Mbed TLS) layered over the same FreeRTOS-Plus-TCP byte stream, persistent store-and-forward via ChaN FatFs over a semihosting-backed disk image, interactive `set NAME VALUE` / `send N` / `set store file` / `set shutdown 1` / `quit` command channel over the CMSDK UART; BDD-driven against syslog-ng including `store_and_forward`, `power_cycle_replay`, `store_capacity`, `tls_transport`, and `mtls_transport` scenarios. See [`Bdd/Targets/FreeRtos/README.md`](Bdd/Targets/FreeRtos/README.md)

## Compliance

- [IEC 62443 Compliance Guide](docs/iec62443.md) — component selection by Security Level (SL1–SL4) for industrial control systems
- [RFC Compliance Matrix](docs/rfc-compliance.md) — sender-side coverage of RFC 5424, 5426, 6587, and 5425

## CI pipeline

See [CI pipeline](docs/ci.md).

## BDD testing

See [BDD testing](docs/bdd.md).

## Container images

See [Container images](docs/containers.md).

## License

Copyright 2026 Cozens Software Solutions Limited.

Licensed under the [PolyForm Noncommercial License 1.0.0](LICENSE.md). Free for
noncommercial, personal, educational, and government use.

For commercial licensing enquiries, please use the contact form at
[cososo.co.uk](https://www.cososo.co.uk/#contact).
