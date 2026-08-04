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

- [Overview](#overview): what SolidSyslog is and how it helps with CRA and IEC 62443 compliance.
- [Adopt it](#adopt): get a syslog stack compiling and sending in your product.
- [Port it to a new platform](#port-a-new-platform): fill a role for an OS, network stack, filesystem, or crypto library we don't ship yet.
- [Compliance](#compliance): the CRA, IEC 62443, the RFCs, and the security posture.
- [API reference](#api-reference): the public contracts, by audience.
- [Maintaining the library](#maintaining): building, testing, and releasing SolidSyslog itself.

---

## Overview

- Why SolidSyslog, and capability at a glance: the [project README](../README.md).
- [Compliance in one page](overview.md): the evaluator's one-screen orientation. What CRA and IEC 62443 ask of an audit-logging function, and how SolidSyslog helps.

## Adopt

Everything you need to consume SolidSyslog in your product.

- [Building up the protection you need](hardening-path.md): start here. The integration path from a device with no syslog to a hardened one, a stage at a time, with the question that drives each step and an indication of what it costs.
- [Adding it to your build](build-integration.md): the build detail. The capability matrix, the three ways to consume the library (CMake, Make, and a source manifest for an IDE project), and the compile-time tunables.
- [Authoring custom structured data](structured-data.md): attaching RFC 5424 SD-ELEMENTs.
- [Error-event severity policy](error-severity.md): installing an error handler and reading the event axes.
- Platform integration guides:
  - [lwIP (Raw API)](integrating-lwip.md)
  - [Mbed TLS](integrating-mbedtls.md)
  - [FreeRTOS-Plus-FAT](integrating-plusfat.md)
- Tunables: the compile-time limits, all `#ifndef`-guarded. See [Adding it to your build → Tunables](build-integration.md#tunables) and [`Core/Interface/SolidSyslogTunablesDefaults.h`](../Core/Interface/SolidSyslogTunablesDefaults.h).

## Port a new platform

Porting SolidSyslog is filling roles, not editing Core: omit an adapter and
Core's Null object stands in.

- [Porting guide](porting.md): the role model, the anatomy of an adapter (instance shape, the no-`malloc` static pool, the error convention), the invariants every adapter must honour, and the twelve vtable role contracts, each with its Null fallback and shipped reference implementation (POSIX where available, otherwise a Core composition).
- The contracts themselves: the `SolidSyslog*Definition.h` vtables under [`Core/Interface/`](../Core/Interface/). [`Platform/Posix/`](../Platform/Posix/) is the reference implementation to read alongside them.

## Compliance

- [CRA guide](cra.md): start here. The Annex I map — the requirement that names logging, the requirements an audit trail contributes to, and what the project publishes for your vulnerability handling. Incident reporting obligations apply from 11 September 2026; the Regulation applies in full from 11 December 2027.
- [Compliance in one page](overview.md): the one-screen orientation on CRA and IEC 62443 together.
- [IEC 62443 compliance guide](iec62443.md): the audit-logging-relevant 62443-4-2 CRs and 62443-3-3 SRs, the levels each helps with, and the components that address them.
- [RFC compliance matrix](rfc-compliance.md): sender-side coverage of RFC 5424, 5426, 6587, and 5425.
- Security:
  - [Threat model](security/threat-model.md)
  - [At-rest cryptography](security/at-rest-cryptography.md)
  - [Software Bill of Materials (SBOM)](security/sbom.md)
  - [Vulnerability triage runbook](security/triage-runbook.md)
  - [Release verification guide](security/release-verification.md)
  - Reporting a vulnerability: [`SECURITY.md`](../SECURITY.md)

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
which is [Adopt](#adopt) above).

- [Building and testing](builds.md): the CMake preset catalogue.
- [Pre-PR local checks](local-checks.md): the tiered pre-PR check budget.
- [BDD testing](bdd.md): the Gherkin / target-binary test infrastructure.
- [CI pipeline](ci.md): the status checks behind branch protection.
- [Container images](containers.md): the dev-container and CI image reference.
- [Naming conventions](NAMING.md): the per-tier scheme satisfying MISRA C:2012 rules 5.1–5.9.
- [MISRA C:2012 deviations](misra-deviations.md): the recorded, deliberate deviations.
- [Release process](release-process.md): release-please, Conventional Commits, and versioning.
