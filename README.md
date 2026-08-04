# SolidSyslog

A structured syslog client library for embedded and industrial systems, implementing
RFC 5424 (structured syslog) with RFC 5426 (UDP) and RFC 6587 (TCP) transports.
TLS per RFC 5425 ships two ways — `SolidSyslogTlsStream` over OpenSSL and
`SolidSyslogMbedTlsStream` over Mbed TLS — and any other TLS library (wolfSSL,
hardware offload, …) plugs in behind the same Stream vtable. TLS is not a core
dependency: Core carries no reference to any TLS library.

It exists to give shipping embedded products the security audit trail the EU
Cyber Resilience Act and IEC 62443 expect — as a component you add, not a
redesign.

## What it costs

**+5 KB flash** for a valid, timestamped RFC 5424 record on the wire, and **0.4 KB
of RAM**. **+13.5 KB flash** for the whole path — store-and-forward, device identity,
mutual TLS and AES-GCM encryption at rest — and **37 KB of RAM**, mainly TLS buffers.
Figures measured on FreeRTOS with lwIP on a Cortex-M3, run under QEMU — a
representative device, not a specification.

Both are measured by a worked integration, published in full as
[solid-syslog-example](https://github.com/cososo-ltd/solid-syslog-example) (consumed
with CMake) and
[solid-syslog-example-make](https://github.com/cososo-ltd/solid-syslog-example-make)
(the same integration, consumed with Make). Each stage is one commit; `git show`
gives the diff and the measured cost.

Designed for resource-constrained environments:

- C99, no dynamic memory allocation — every instance lives in a library-internal static pool, sized at compile time
- Transport-agnostic — UDP, TCP, TLS, or bring your own
- Buffer-agnostic — PassthroughBuffer (direct send), portable CircularBuffer (mutex-injected ring), POSIX message queue, or bring your own
- MISRA C:2012 informed

## Capabilities

RFC 5424 structured formatting over UDP (RFC 5426), TCP (RFC 6587), and TLS /
mutual TLS (RFC 5425). Asynchronous buffering, rotating block store-and-forward,
and at-rest record protection — CRC-16 for accidental corruption, or keyed
HMAC-SHA256 / AES-256-GCM where a local attacker is in scope. The full
[IEC 62443 SL1–SL4 component set](https://docs.cososo.co.uk/solid-syslog/iec62443/)
is available.

SolidSyslog is built for embedded and RTOS targets. Every platform dependency —
TCP/IP stack, TLS library, filesystem, OS primitives, clock — is injected through
a vtable, so the library ports to **any** embedded OS by filling roles rather
than editing Core. Reference adapters ship for FreeRTOS on Cortex-M (networking
via FreeRTOS-Plus-TCP or lwIP, transport security via `SolidSyslogMbedTlsStream`
over Mbed TLS, persistent store-and-forward over ChaN FatFs or FreeRTOS-Plus-FAT)
— and for POSIX and Windows, fully supported as development, test, and edge /
gateway hosts. Bring your own stack and the same Core runs unchanged.

TLS revocation (CRL / OCSP) is not performed by the library. Whether it is
enforced depends on the TLS backend and platform you configure.

## Documentation

Full documentation lives at
[docs.cososo.co.uk/solid-syslog](https://docs.cososo.co.uk/solid-syslog/), organised
around what you came to do: **Overview**, **Adopt**, **Port a new platform**,
**Compliance**, **API reference**, and **Maintaining**. New here?
[Compliance in one page](https://docs.cososo.co.uk/solid-syslog/overview/) is the
fastest orientation for evaluators.

## Integrating it

[Building up the protection you need](https://docs.cososo.co.uk/solid-syslog/hardening-path/)
walks an integration from a device with no syslog to a hardened one, one stage at a
time, stating what each stage adds, the question that decides whether you need it,
and an indication of what it costs.

[Adding it to your build](https://docs.cososo.co.uk/solid-syslog/build-integration/)
is the build detail behind it: the capability matrix, the three ways to consume the
library — CMake, Make, and a source manifest for an IDE project — and the
compile-time tunables.

## Building and testing

Developing the library itself? See
[Building and testing](https://docs.cososo.co.uk/solid-syslog/builds/) — the
contributor/maintainer preset catalogue — alongside the pre-PR check budget, the BDD
infrastructure, the CI pipeline, and the container images, all under **Maintaining**
on the documentation site. (Consuming the library in your product is the integration
path above.)

## Architecture

SolidSyslog is OO-in-C. Every platform dependency and every optional feature is a
vtable role, injected at setup and composed at link time, so a feature you do not
wire is dropped by the linker rather than excluded by the preprocessor — Core's
implementation contains no conditional compilation at all. Public headers are split
by audience: application code that logs events includes `SolidSyslog.h` and nothing
else, while the setup that builds a logger includes `SolidSyslogConfig.h` plus one
header per component it wires.

The [API reference](https://docs.cososo.co.uk/solid-syslog/api-reference/) explains
that split and links the generated reference for every header, type and symbol. The
[porting guide](https://docs.cososo.co.uk/solid-syslog/porting/) covers the twelve
roles, the anatomy of an adapter, and the Null object that stands in for any role you
leave unfilled.

[`Bdd/Targets/`](Bdd/Targets/) holds one BDD-driven binary per platform — Linux,
Windows, and FreeRTOS on QEMU — each exercising the library end to end against a real
syslog server; see [BDD testing](https://docs.cososo.co.uk/solid-syslog/bdd/).

## Compliance

- [CRA guide](https://docs.cososo.co.uk/solid-syslog/cra/) — the Annex I map: the requirement that names logging, the requirements an audit trail contributes to, what the project publishes for your vulnerability handling, and the dates the Regulation applies from
- [Compliance in one page](https://docs.cososo.co.uk/solid-syslog/overview/) — the evaluator's one-screen orientation on CRA and IEC 62443
- [IEC 62443 compliance guide](https://docs.cososo.co.uk/solid-syslog/iec62443/) — component selection by Security Level (SL1–SL4) for industrial control systems
- [RFC compliance matrix](https://docs.cososo.co.uk/solid-syslog/rfc-compliance/) — sender-side coverage of RFC 5424, 5426, 6587, and 5425
- [Threat model](https://docs.cososo.co.uk/solid-syslog/security/threat-model/) — the division of responsibility between the library and your product

Reporting a vulnerability: [`SECURITY.md`](SECURITY.md).

## License

Copyright 2026 Cozens Software Solutions Limited.

Licensed under the [PolyForm Noncommercial License 1.0.0](LICENSE.md). Free for
noncommercial, personal, educational, and government use.

For commercial licensing — including pricing, the early-adopter programme, and
the platform adapter policy — see the
[SolidSyslog product page](https://www.cososo.co.uk/products/solid-syslog/), or
use the contact form at [cososo.co.uk](https://www.cososo.co.uk/#contact).
