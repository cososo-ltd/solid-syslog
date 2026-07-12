# Compliance in one page

> [!WARNING]
> The documentation is under active development and may be incomplete or
> inaccurate. Do not rely on it for integration, security, or compliance
> decisions until the 0.1.0 release.

SolidSyslog is a client-side structured-syslog library that helps you implement
the audit-logging and product-security controls the EU Cyber Resilience Act
(CRA) and IEC 62443 expect of industrial and connected products. You wire
the roles your deployment needs; the library handles the RFC 5424 formatting,
reliable delivery, store-and-forward survival, at-rest record protection, and
evidence metadata that those frameworks expect an audit-logging function to
provide, on any embedded RTOS or bare-metal target (bring your own network
stack, TLS library, and filesystem, or use the shipped reference adapters), and
on POSIX and Windows hosts too.

This page is the evaluator's one-screen orientation. It links out to the
control-by-control detail rather than restating it.

> [!NOTE]
> IEC 62443 certifies systems, not components, and a Security Level is a
> property of your whole system, its deployment, and its assessment, not of a
> parts list. Likewise, the CRA places its obligations on the economic operators
> who bring a product to market (manufacturers, and in defined cases importers
> and distributors), not on any single component. SolidSyslog is a component:
> this is our best advice on how it helps you address the audit-logging and
> security aspects of those frameworks. It is guidance, not a guarantee of
> compliance, and no substitute for assessment of your full product.

## IEC 62443 — audit logging at a glance

The Security Levels are a superset ladder: each adds controls on top of the one
below. SolidSyslog gives you the audit-logging building blocks at every rung.

| Security Level | Attacker in scope | What a SolidSyslog deployment adds on top of the level below |
|---|---|---|
| **SL1** | Casual or accidental | A valid, timestamped RFC 5424 record, delivered to your collector and kept readable |
| **SL2** | Simple, intentional | Trusted-time metadata, gap-visible delivery, a protected and authenticated channel (TLS), and store-and-forward survival across outages |
| **SL3** | Sophisticated, IACS-aware | Authenticated provenance and gap detection (mutual TLS + `sequenceId`), tamper-evident (keyed) at-rest integrity, per-device identity, and storage-threshold warnings |
| **SL4** | State-level | The same evidence, hardened: write-once / immutable storage, a protected time source, and keys held in hardware |

Which components realise each rung, and why each choice is driven by your
deployment rather than the label, is on
[Choosing components by Security Level](security-levels.md). The
control-by-control map (every relevant IEC 62443-4-2 Component Requirement and
62443-3-3 System Requirement, the level it applies at, and the components that
satisfy it) is in the
[IEC 62443 compliance guide](iec62443.md#control-implementation).

## CRA — where SolidSyslog helps

The CRA's essential requirements (Annex I) ask a product with digital elements to
log security-relevant events and support secure updates, and its manufacturer to
maintain a machine-readable bill of materials and handle vulnerabilities.
SolidSyslog contributes to several of those obligations directly:

| CRA obligation | How SolidSyslog supports it |
|---|---|
| **Security logging & monitoring** | The library's whole purpose: RFC 5424 audit records to any SIEM, with SIEM-side gap detection via `sequenceId`. See [IEC 62443 guide → SIEM integration](iec62443.md#siem-integration) |
| **Software bill of materials** | The SolidSyslog project publishes a CycloneDX [SBOM](security/sbom.md) per release, ready to fold into your product's own bill of materials |
| **Vulnerability handling & coordinated disclosure** | [`SECURITY.md`](../SECURITY.md) and the [vulnerability triage runbook](security/triage-runbook.md) |
| **Secure, verifiable releases** | [Release verification guide](security/release-verification.md): signed, reproducible artefacts |
| **Secure-by-design & documented risk** | [Threat model](security/threat-model.md) and [at-rest cryptography](security/at-rest-cryptography.md) |

These support the manufacturer's CRA duties for the finished product; they do
not discharge them on their own.

## Go deeper

- [Choosing components by Security Level](security-levels.md): the choices you make and what drives each.
- [IEC 62443 compliance guide](iec62443.md): the control-by-control map underneath this page.
- [RFC compliance matrix](rfc-compliance.md): sender-side coverage of RFC 5424 / 5426 / 6587 / 5425.
- [Security documentation](README.md#compliance): threat model, at-rest crypto, SBOM, triage, release verification.
- [Getting started](getting-started.md): when you are ready to wire it up.
