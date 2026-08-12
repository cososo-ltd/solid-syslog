# SolidSyslog

A structured syslog client library for embedded and industrial systems, implementing
RFC 5424 (structured syslog) with RFC 5426 (UDP) and RFC 6587 (TCP) transports.
TLS per RFC 5425 is supplied by a platform, so any TLS library plugs in behind the
same Stream vtable. TLS is not a core dependency: Core carries no reference to any
TLS library.

It exists to give shipping embedded products the security audit trail the EU
Cyber Resilience Act and IEC 62443 expect: a component you add, not a redesign.

## What it costs

**+5 KB flash** for a valid, timestamped RFC 5424 record on the wire, and **0.4 KB
of RAM**. **+13.5 KB flash** for the whole path - store-and-forward, device identity,
mutual TLS and AES-GCM encryption at rest - and **37 KB of RAM**, mainly TLS buffers.
Each figure is what SolidSyslog adds to a device already running FreeRTOS, lwIP,
FatFs and Mbed TLS: a baseline built to carry the third-party code a real-world
device would already have. Measured on a Cortex-M3 under QEMU, a representative
device rather than a specification.

Both are measured by a worked integration, published in full as
[solid-syslog-example](https://github.com/cososo-ltd/solid-syslog-example) (consumed
with CMake) and
[solid-syslog-example-make](https://github.com/cososo-ltd/solid-syslog-example-make)
(the same integration, consumed with Make). Each stage is one commit; `git show`
gives the diff and the measured cost.

Designed for resource-constrained environments:

- C99, no dynamic memory allocation - every instance lives in a library-internal static pool, sized at compile time
- Transport-agnostic - UDP, TCP, TLS, or bring your own
- Buffer-agnostic - PassthroughBuffer (direct send), portable CircularBuffer (mutex-injected ring), POSIX message queue, or bring your own
- MISRA C:2012 informed

## Capabilities

RFC 5424 structured formatting over UDP (RFC 5426), TCP (RFC 6587), and TLS /
mutual TLS (RFC 5425). Asynchronous buffering, rotating block store-and-forward,
and at-rest record protection: CRC-16 for accidental corruption, or keyed
HMAC-SHA256 / AES-256-GCM where a local attacker is in scope. The audit-logging
capabilities an IEC 62443 deployment draws on are mapped control by control in the
[IEC 62443 guide](https://docs.cososo.co.uk/solid-syslog/iec62443/).

SolidSyslog is built for embedded and RTOS targets. Every platform dependency -
TCP/IP stack, TLS library, filesystem, OS primitives, clock - is injected through
a vtable, so the library ports to an embedded OS by filling roles rather
than editing Core. Platforms ship for FreeRTOS on Cortex-M, and for POSIX and
Windows, fully supported as development, test, and edge / gateway hosts. Which
upstream fills which capability is in the
[platform matrix](https://docs.cososo.co.uk/solid-syslog/platforms/). Bring your
own stack and the same Core runs unchanged.

TLS revocation (CRL / OCSP) is not performed by the library. Whether it is
enforced depends on the TLS backend and platform you configure.

## Documentation

Full documentation lives at
[docs.cososo.co.uk/solid-syslog](https://docs.cososo.co.uk/solid-syslog/), organised
around what you came to do: **Core**, **Integrate**, **Platforms**,
**Compliance**, **API reference**, and **Maintaining**. New here?
[Compliance in one page](https://docs.cososo.co.uk/solid-syslog/overview/) is the
fastest orientation for evaluators.

## Integrating it

[Building up the protection you need](https://docs.cososo.co.uk/solid-syslog/hardening-path/)
walks an integration from a device with no syslog to a hardened one, one stage at a
time, stating what each stage adds, the question that decides whether you need it,
and an indication of what it costs.

[Adding it to your build](https://docs.cososo.co.uk/solid-syslog/build-integration/)
is the build detail behind it: how the library composes, how to pick your stack, the
three ways to consume it - CMake, Make, and a source manifest for an IDE project -
and the compile-time tunables.

## Building and testing

Developing the library itself? See
[Building and testing](https://docs.cososo.co.uk/solid-syslog/builds/) - the
contributor/maintainer preset catalogue - alongside the pre-PR check budget, the BDD
infrastructure, the CI pipeline, and the container images, all under **Maintaining**
on the documentation site. (Consuming the library in your product is the integration
path above.)

## Architecture

SolidSyslog is OO-in-C. Every platform dependency and every optional feature is a
vtable role, injected at setup and composed at link time, so a feature you do not
wire is dropped by the linker rather than excluded by the preprocessor. Core's
implementation contains no conditional compilation at all. Public headers are split
by audience: application code that logs events includes `SolidSyslog.h` and nothing
else, while the setup that builds a logger includes `SolidSyslogConfig.h` plus one
header per component it wires.

The [API reference](https://docs.cososo.co.uk/solid-syslog/api-reference/) explains
that split and links the generated reference for every header, type and symbol. The
[porting guide](https://docs.cososo.co.uk/solid-syslog/porting/) covers the roles,
the anatomy of an adapter, and the Null object that stands in for any role you
leave unfilled.

[`Bdd/Targets/`](Bdd/Targets/) holds one BDD-driven binary per platform - Linux,
Windows, and two FreeRTOS-on-QEMU builds, one per network stack - each exercising
the library end to end against a real syslog server; see
[BDD testing](https://docs.cososo.co.uk/solid-syslog/bdd/).

## Compliance

- [CRA guide](https://docs.cososo.co.uk/solid-syslog/cra/) - the Annex I map: the requirement that names logging, the requirements an audit trail contributes to, what the project publishes for your vulnerability handling, and the dates the Regulation applies from
- [Compliance in one page](https://docs.cososo.co.uk/solid-syslog/overview/) - the evaluator's one-screen orientation on CRA and IEC 62443
- [IEC 62443 compliance guide](https://docs.cososo.co.uk/solid-syslog/iec62443/) - the audit-logging-relevant controls, and what the library provides against each
- [RFC compliance matrix](https://docs.cososo.co.uk/solid-syslog/rfc-compliance/) - sender-side coverage of RFC 5424, 5426, 6587, and 5425
- [Threat model](https://docs.cososo.co.uk/solid-syslog/security/threat-model/) - the division of responsibility between the library and your product

Reporting a vulnerability: [`SECURITY.md`](SECURITY.md).

## Licence

Copyright 2026 Cozens Software Solutions Limited.

SolidSyslog is source-available under three alternative licences:

- **[PolyForm Internal Use 1.0.0](LICENSES/PolyForm-Internal-Use-1.0.0.md)** -
  free, no time limit, for evaluation and development inside any organisation,
  commercial or otherwise. Evaluate it, port it, integrate it, test it on
  target hardware. No licence key, no trial period, no sales call.
- **[PolyForm Noncommercial 1.0.0](LICENSES/PolyForm-Noncommercial-1.0.0.md)** -
  free for hobby, charitable, educational, public research and government use,
  including redistribution.
- **COSOSO Commercial Licence** - required when you supply, sell, distribute or
  otherwise make available a commercial product, device, firmware or service
  containing SolidSyslog. [Get in touch](https://www.cososo.co.uk/#contact).

**Free until you ship.** See [LICENSE.md](LICENSE.md) for detail.

For commercial pricing, the early-adopter programme, and the platform adapter
policy, see the
[SolidSyslog product page](https://www.cososo.co.uk/products/solid-syslog/).
