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
- Buffer-agnostic — NullBuffer (direct send), POSIX message queue, or bring your own
- No `#ifdef` feature flags — optional features composed at link time
- MISRA C:2012 informed
- Dependency injection throughout — fully testable without a network

## Status

Approaching feature-complete for POSIX and Windows: RFC 5424 structured
formatting, UDP / TCP / TLS / mTLS transport, asynchronous buffering, rotating
file store-and-forward with CRC-16 integrity, and the full
[IEC 62443 SL1–SL4 component set](docs/iec62443.md).

**Not yet production-ready**, and no API stability guarantee yet. Known gaps:

- Public API may evolve — sender implementations currently use static
  singleton state (`SolidSyslogUdpSender`, `SolidSyslogStreamSender`,
  `SolidSyslogSwitchingSender`), so multiple concurrent instances per
  process aren't yet supported. Additional platform backends (RTOS,
  custom) are still to land.
- At-rest integrity is CRC-16 only; HMAC + AES-at-rest are planned for SL4
  ([E17 #105](https://github.com/DavidCozens/solid-syslog/issues/105)).
- TLS revocation (CRL / OCSP) is deferred to the OS trust store; the library
  itself does not perform revocation checks.
- Comprehensive error guards still rolling out
  ([E12 #31](https://github.com/DavidCozens/solid-syslog/issues/31)).

## Building and testing

See [Building and testing](docs/builds.md).

## Architecture

SolidSyslog uses an OO-in-C style with vtable structs and dependency injection.
All fields — required and optional — use a uniform field object pattern.
Optional features are composed at link time via dead code elimination; there are
no conditional compilation directives in the library source.

Public headers are split by audience (Interface Segregation Principle):
- **`SolidSyslog.h`** — application code that logs events (`Log`, `Service`)
- **`SolidSyslogConfig.h`** — system setup code that creates and destroys loggers
- **`SolidSyslogSenderDefinition.h`** / **`SolidSyslogBufferDefinition.h`** — extension points for custom senders and buffers
- **`SolidSyslogNullBuffer.h`** — direct-send buffer for single-task systems
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
- **`SolidSyslogStoreDefinition.h`** / **`SolidSyslogFileStore.h`** — file-based store-and-forward with rotating files
- **`SolidSyslogSecurityPolicyDefinition.h`** — extension point for record integrity policies
- **`SolidSyslogCrc16Policy.h`** — CRC-16/CCITT-FALSE integrity policy
- **`SolidSyslogStructuredDataDefinition.h`** — extension point for custom structured data
- **`SolidSyslogMetaSd.h`** — sequenceId structured data (RFC 5424 §7.3)
- **`SolidSyslogTimeQualitySd.h`** — timeQuality structured data (RFC 5424 §7.1)
- **`SolidSyslogOriginSd.h`** — origin structured data (RFC 5424 §7.2)
- **`SolidSyslogPosixClock.h`** / **`SolidSyslogPosixHostname.h`** / **`SolidSyslogPosixProcessId.h`** — POSIX helpers

Two example programs demonstrate usage:
- **`Example/SingleTask/`** — NullBuffer, single-task bare-metal model
- **`Example/Threaded/`** — PosixMessageQueueBuffer, two pthreads (logger + service), SwitchingSender over UDP + TCP + TLS + mTLS (TLS build required for the last two); `--transport` sets the initial transport, `switch <name>` flips it at runtime

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