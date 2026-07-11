# SolidSyslog documentation

> [!WARNING]
> **The documentation is under active development and may be incomplete or
> inaccurate.** Do not rely on it — for integration, security, or compliance
> decisions — until the 0.1.0 release.

This is the documentation home. It is organised around what you came to do.
Pick a lane:

- **[Overview](#overview)** — what SolidSyslog is and how it helps with CRA and IEC 62443 compliance.
- **[Adopt it](#adopt)** — get a syslog stack compiling and sending in your product.
- **[Port it to a new platform](#port-a-new-platform)** — fill a role for an OS, network stack, filesystem, or crypto library we don't ship yet.
- **[Compliance](#compliance)** — IEC 62443, the RFCs, and the security posture.
- **[API reference](#api-reference)** — the public contracts, by audience.
- **[Maintaining the library](#maintaining)** — building, testing, and releasing SolidSyslog itself.

---

## Overview

- **Why SolidSyslog & capability at a glance** — the [project README](../README.md).
- **[Compliance in one page](overview.md)** — the evaluator's one-screen orientation: what CRA and IEC 62443 ask of an audit-logging function, and how SolidSyslog helps.

## Adopt

Everything you need to consume SolidSyslog in your product.

- **[Getting started](getting-started.md)** — the integrator front door: the capability matrix, both consumption paths (CMake and non-CMake / IAR / Keil source integration), the worked embedded manifest, the tunables, and a "your first log" walkthrough.
- **[Choosing components by Security Level](security-levels.md)** — which roles to fill for a good story at each SL, framed around your deployment's drivers, with worked starter combinations.
- **[Authoring custom structured data](structured-data.md)** — attaching RFC 5424 SD-ELEMENTs.
- **[Error-event severity policy](error-severity.md)** — installing an error handler and reading the event axes.
- **Platform integration guides:**
  - [lwIP (Raw API)](integrating-lwip.md)
  - [Mbed TLS](integrating-mbedtls.md)
  - [FreeRTOS-Plus-FAT](integrating-plusfat.md)
- **Tunables** — the compile-time limits, all `#ifndef`-guarded: see [Getting started → Tunables](getting-started.md#tunables) and [`Core/Interface/SolidSyslogTunablesDefaults.h`](../Core/Interface/SolidSyslogTunablesDefaults.h).

## Port a new platform

Porting SolidSyslog is *filling roles*, not editing Core — omit an adapter and
Core's Null object stands in. Until the dedicated porting guide lands, the
material you need already exists in the tree:

- **The role contracts** — the vtable extension points under
  [`Core/Interface/`](../Core/Interface/): each `SolidSyslog*Definition.h`
  (e.g. `SolidSyslogStreamDefinition.h`, `SolidSyslogFileDefinition.h`,
  `SolidSyslogMutexDefinition.h`) is the contract a new adapter must honour.
- **The reference implementations** — [`Platform/Posix/`](../Platform/Posix/)
  reads alongside each contract as the worked example, including the
  library-internal static-pool pattern (`*Static.c` +
  `SOLIDSYSLOG_*_POOL_SIZE`) and the per-adapter `*Errors.h` convention.
- **The coexistence invariants** — the "never free injected handles" and
  idempotent `Close`/`Destroy` rules, illustrated by the Mbed TLS coexistence
  contract in the [IEC 62443 guide](iec62443.md#embedded-sl4-solidsyslogmbedtlsstream).

*A consolidated porting overview and role-contracts page are planned.*

## Compliance

- **[Compliance in one page](overview.md)** — start here.
- **[IEC 62443 compliance guide](iec62443.md)** — component selection by Security Level (SL1–SL4), mapped control-by-control to IEC 62443-4-2 CRs and 62443-3-3 SRs.
- **[RFC compliance matrix](rfc-compliance.md)** — sender-side coverage of RFC 5424, 5426, 6587, and 5425.
- **Security:**
  - [Threat model](security/threat-model.md)
  - [At-rest cryptography](security/at-rest-cryptography.md)
  - [Software Bill of Materials (SBOM)](security/sbom.md)
  - [Vulnerability triage runbook](security/triage-runbook.md)
  - [Release verification guide](security/release-verification.md)
  - Reporting a vulnerability — [`SECURITY.md`](../SECURITY.md)

## API reference

Application code only ever includes `SolidSyslog.h` (and `SolidSyslogConfig.h`
at setup); everything else is wired once behind the config struct. The public
headers are split by audience (Interface Segregation) under
[`Core/Interface/`](../Core/Interface/) and each `Platform/*/Interface/`. The
[project README](../README.md#architecture) lists the headers by audience.

*A generated API reference (Doxygen → the doc site) is planned.*

## Maintaining

For contributors and maintainers of SolidSyslog itself (not for consuming it —
that is [Adopt](#adopt) above).

- **[Building and testing](builds.md)** — the CMake preset catalogue.
- **[Pre-PR local checks](local-checks.md)** — the tiered pre-PR check budget.
- **[BDD testing](bdd.md)** — the Gherkin / target-binary test infrastructure.
- **[CI pipeline](ci.md)** — the status checks behind branch protection.
- **[Container images](containers.md)** — the dev-container and CI image reference.
- **[Naming conventions](NAMING.md)** — the per-tier scheme satisfying MISRA C:2012 rules 5.1–5.9.
- **[MISRA C:2012 deviations](misra-deviations.md)** — the recorded, deliberate deviations.
- **[Release process](release-process.md)** — release-please, Conventional Commits, and versioning.
