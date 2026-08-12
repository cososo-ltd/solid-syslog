# CRA — where SolidSyslog helps

The EU Cyber Resilience Act, Regulation (EU) 2024/2847, sets essential cybersecurity
requirements for products with digital elements placed on the EU market. Its Annex I has
two parts: Part I covers the properties of the product, Part II the manufacturer's
vulnerability-handling process. This page maps the audit-trail-relevant parts of both to
what SolidSyslog provides and what stays yours.

The Regulation applies from **11 December 2027**, with two earlier dates: Chapter IV
(Articles 35 to 51) from 11 June 2026, and Article 14, the manufacturer's reporting
obligations for actively exploited vulnerabilities and severe incidents, from 11
September 2026.

> [!NOTE]
> SolidSyslog is a library you build into a product, not a product in its own right.
> Under IEC 62443 it is assessed as part of the component you ship, and CRA obligations
> fall on the manufacturer placing that product on the market. What follows is our
> account of how the library helps you meet the audit-logging parts of those frameworks.
> It is guidance, not a guarantee of compliance, and no substitute for assessment of
> your finished product.

## The requirements are gated on your own risk assessment

Annex I Part I introduces its requirements with a qualifier worth reading closely:

> On the basis of the cybersecurity risk assessment referred to in Article 13(2) and
> where applicable, products with digital elements shall: …

Article 13(2) requires the manufacturer to assess the cybersecurity risks of the product
and take the outcome into account through design, development, production, delivery and
maintenance. The Part I requirements then apply *on the basis of* that assessment, and
*where applicable*.

So the regulation does not hand you a fixed specification, and two products can meet it
with very different implementations. What your device needs from its audit trail is an
output of your risk assessment and your documented intended purpose. That is the same
reasoning behind [building up the protection you need](hardening-path.md), which walks
the capabilities in the order an integration usually adds them so you can stop where
your own assessment says to.

## Part I (2)(l) — the requirement that names logging

> provide security related information by recording and monitoring relevant internal
> activity, including the access to or modification of data, services or functions, with
> an opt-out mechanism for the user

This is the requirement SolidSyslog exists to serve. The library formats RFC 5424
records and delivers them to a collector, so what your device records becomes evidence a
SIEM can consume, correlate across devices, and retain.

| What the point asks for | What SolidSyslog provides |
|---|---|
| Recording relevant internal activity | `SolidSyslog_Log` and RFC 5424 formatting. Your application decides which events are security-relevant; the library carries them |
| Security-related *information*, not raw lines | Structured data elements — `MetaSd`, `TimeQualitySd`, `OriginSd` — attaching sequence, clock quality, uptime and device identity, plus your own private elements |
| Monitoring | Delivery to any RFC 5424 collector over UDP, TCP or TLS, with `sequenceId` letting the collector detect records that never arrived |
| Activity that must survive the device | Store-and-forward across outages and reboots, with at-rest protection |

**What stays yours.** Deciding which internal activity is relevant. Providing the user's
opt-out mechanism, which is a product-level control the library has no view of.
Retention, access control and disposal at the collector.

## Part I — requirements an audit trail contributes to

The log path is not the primary means of meeting these, but it is in scope for each,
because a log record is itself stored and transmitted data, and because several of them
call for reporting. Where the answer below is TLS, the [TLS obligations](tls.md) page states what
any TLS stream must do, and each backend's page records where it falls short of that
today.

| Point | What it asks for | How the audit trail contributes |
|---|---|---|
| **(2)(d)** | protection from unauthorised access, and *report on possible unauthorised access* | The reporting half only, and only the carriage of it: your application detects the access and decides it is reportable, the library delivers the record. Mutual TLS authenticates the TLS peer to the receiver, which is the device itself only where it connects directly |
| **(2)(e)** | confidentiality of stored, transmitted or otherwise processed data, including by encryption at rest or in transit | TLS in transit; authenticated encryption at rest for the spooled store. Records routinely carry data you would not publish |
| **(2)(f)** | integrity of stored and transmitted data against unauthorised modification, and *report on corruptions* | A keyed at-rest policy makes stored records tamper-evident rather than merely checksummed; TLS protects them in transit; the error handler surfaces corruption the store detects |
| **(2)(h)** | availability of essential and basic functions, also after an incident | Buffering keeps logging off the critical path, and store-and-forward keeps records through an outage so the trail survives the incident it recorded |

Where each of these sits on the integration path, and what it costs, is on [building up
the protection you need](hardening-path.md).

## Part II — supporting your vulnerability handling

Part II binds you as the manufacturer of your product, not us. What the SolidSyslog
project publishes is intended to drop into your own process for the component you are
consuming.

| Point | What it asks of you | What the project publishes |
|---|---|---|
| **(1)** | identify and document components, including a software bill of materials in a machine-readable format | A CycloneDX [SBOM](security/sbom.md) per release, to fold into your product's own |
| **(2)**, **(4)** | remediate without delay; disclose fixed vulnerabilities | The [vulnerability triage runbook](security/triage-runbook.md) sets out how reports are assessed and published |
| **(5)**, **(6)** | a coordinated vulnerability disclosure policy, and a contact address | [Security policy](security/policy.md) |
| **(7)** | securely distribute updates | [Release verification](security/release-verification.md): a signed source-tree hash and signed CycloneDX SBOM, both verifiable against the workflow and tag that produced them. They are published by the release workflow, so verify their presence for the release you are adopting rather than assuming it |

Secure-by-design evidence for your technical file is in the [threat
model](security/threat-model.md) and [at-rest
cryptography](security/at-rest-cryptography.md), which state which edges the library
crosses in your product and what it delegates to you by contract.

## Where to go next

- [Building up the protection you need](hardening-path.md): the integration path, stage by stage.
- [TLS obligations](tls.md): what a TLS stream must do, for the transit half of (2)(e) and (2)(f).
- [IEC 62443 guide](iec62443.md): the control-by-control map for industrial deployments.
- [Compliance in one page](overview.md): the one-screen orientation across both frameworks.
- [Threat model](security/threat-model.md): the division of responsibility this page assumes.

Citations are to Regulation (EU) 2024/2847 as published in the Official Journal.
