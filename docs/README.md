# ![SolidSyslog](assets/images/solidsyslog-mark-a.svg)

SolidSyslog is a structured RFC 5424 syslog library for embedded and industrial
systems, built to give shipping products the audit trail the EU Cyber Resilience
Act and IEC 62443 expect. This documentation is organised around what you came
to do.

See a complete integration, one commit at a time, with every byte measured:
[solid-syslog-example](https://github.com/cososo-ltd/solid-syslog-example) (CMake)
and [solid-syslog-example-make](https://github.com/cososo-ltd/solid-syslog-example-make)
(Make).

Pick a lane:

- [Core](#core): the library itself — the protocol, the pipeline, and the portable pieces that run the same everywhere.
- [Integrate it](#integrate): get a syslog stack compiling and sending in your product.
- [Platforms](#platforms): what reaches your hardware — the shipped adapters, and how to write one for a target we don't cover.
- [Compliance](#compliance): the CRA, IEC 62443, the RFCs, and the security posture.
- [API reference](#api-reference): the public contracts, by audience.
- [Maintaining the library](#maintaining): building, testing, and releasing SolidSyslog itself.

---

## Core

- [Core](core/index.md): what is always compiled and depends on nothing external — the syslog protocol, the assembly, buffering, storage and sending pipeline, and the portable role implementations. Where a platform exists to reach your hardware, Core exists to be the same everywhere.
- Why SolidSyslog, and capability at a glance: the project `README.md`.

## Integrate

Everything you need to consume SolidSyslog in your product.

- [Building up the protection you need](hardening-path.md): start here. The integration path from a device with no syslog to a hardened one, a stage at a time, with the question that drives each step and an indication of what it costs.
- [Adding it to your build](build-integration.md): the build detail. The capability matrix, the three ways to consume the library (CMake, Make, and a source manifest for an IDE project), and the compile-time tunables.
- [Authoring custom structured data](structured-data.md): attaching RFC 5424 SD-ELEMENTs.
- [Error-event severity policy](error-severity.md): installing an error handler and reading the event axes.
- Tunables: the compile-time limits, all `#ifndef`-guarded. See [Adding it to your build → Tunables](build-integration.md#tunables) and [`Core/Interface/SolidSyslogTunablesDefaults.h`](api/SolidSyslogTunablesDefaults_8h.md).

## Platforms

A platform is a set of adapters wrapping one upstream thing — a network stack, a
TLS library, a filesystem, an OS — behind the library's vtables. Each page says
what that platform ships and what wiring it needs; the pages above speak of them
in the general case.

- [Platform × capability matrix](platforms/index.md): start here. Read across a row for what a platform gives you, down a column for who provides a capability.
- [Porting guide](porting.md): nothing shipped fits your target. The role model, the anatomy of an adapter (instance shape, the no-`malloc` static pool, the error convention), the invariants every adapter must honour, and the vtable role contracts, each with its shipped reference implementation.
- The contracts themselves: the `SolidSyslog*Definition.h` vtables under `Core/Interface/`. `Platform/Posix/` is the reference implementation to read alongside them.

## Compliance

- [CRA guide](cra.md): start here. The Annex I map — the requirement that names logging, the requirements an audit trail contributes to, and what the project publishes for your vulnerability handling. Vulnerability and incident reporting obligations apply from 11 September 2026; the Regulation applies in full from 11 December 2027.
- [Compliance in one page](overview.md): the one-screen orientation on CRA and IEC 62443 together.
- [IEC 62443 compliance guide](iec62443.md): the audit-logging-relevant 62443-4-2 CRs and 62443-3-3 SRs, the levels each helps with, and the components that address them.
- [RFC compliance matrix](rfc-compliance.md): sender-side coverage of RFC 5424, 5426, 6587, and 5425.
- Security:
  - [Threat model](security/threat-model.md)
  - [At-rest cryptography](security/at-rest-cryptography.md)
  - [Software Bill of Materials (SBOM)](security/sbom.md)
  - [Vulnerability triage runbook](security/triage-runbook.md)
  - [Release verification guide](security/release-verification.md)
  - Reporting a vulnerability: [Security policy](security/policy.md)

## API reference

The public headers are split by the job your code is doing: logging an event or
draining the queue each include only `SolidSyslog.h`, while the setup that builds
the logger includes `SolidSyslogConfig.h` plus one header per component it wires.
The [API reference](api-reference/index.md) explains that split, introduces the
platforms and roles behind the config struct, and links the full generated
reference — [Headers](api/files.md), [Data structures](api/annotated.md),
[Functions](api/functions.md), and [Macros](api/macros.md).

## Maintaining

For contributors and maintainers of SolidSyslog itself (not for consuming it,
which is [Integrate](#integrate) above).

- [Building and testing](builds.md): the CMake preset catalogue.
- [Pre-PR local checks](local-checks.md): the tiered pre-PR check budget.
- [BDD testing](bdd.md): the Gherkin / target-binary test infrastructure.
- [CI pipeline](ci.md): the status checks behind branch protection.
- [Container images](containers.md): the dev-container and CI image reference.
- [Naming conventions](NAMING.md): the per-tier scheme satisfying MISRA C:2012 rules 5.1–5.9.
- [MISRA C:2012 deviations](misra-deviations.md): the recorded, deliberate deviations.
- [Release process](release-process.md): release-please, Conventional Commits, and versioning.
