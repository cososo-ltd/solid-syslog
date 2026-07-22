# EU Cyber Resilience Act (CRA) guide

The EU Cyber Resilience Act — [Regulation (EU) 2024/2847](https://eur-lex.europa.eu/eli/reg/2024/2847/oj) —
sets essential cybersecurity requirements for products with digital elements, with
its core obligations biting from December 2027. Several of its Annex I essential
requirements concern the *audit trail*: recording security-relevant activity,
protecting those records in transit and at rest, keeping the logging function
available, and authenticating the device that produced them.

This page maps the audit-trail-relevant Annex I requirements to the SolidSyslog
capability that helps meet each. It mirrors the role the
[IEC 62443 guide](iec62443.md) plays for the IEC controls. Capabilities are named
generically — by the role or primary class that realises them — with vendor
variants (OpenSSL / Mbed TLS / per-platform) treated as asides.

> [!NOTE]
> The CRA places its obligations on the economic operators who bring a product to
> market (manufacturers, and in defined cases importers and distributors), not on
> any single component. SolidSyslog is a component: this is guidance on how
> SolidSyslog's components help you address the audit-logging aspects of these
> frameworks; it is not a guarantee of compliance, and no substitute for
> assessment of your full product.

## Annex I Part I — essential requirements the audit trail helps with

The requirements below are the audit-trail-relevant subset of Annex I, Part I,
point (2) ("products with digital elements shall …"). The recording-and-monitoring
requirement, **(2)(l)**, is the anchor — it is the reason SolidSyslog exists; the
others describe how those records must be protected and kept available.

| Annex I Part I (2) requirement | What it asks | SolidSyslog capability (generic) |
|---|---|---|
| **(l)** — provide security-related information by recording and monitoring relevant internal activity | log the access to, or modification of, data, services or functions | `SolidSyslog_Log` emits RFC 5424 audit records to any SIEM; `SolidSyslogMetaSd` sequenceId gives the SIEM end-to-end gap detection. The library's whole purpose |
| **(f)** — protect the integrity of stored, transmitted or processed data, commands and configuration | records must not be silently altered | in transit: TLS transport (`SolidSyslogTlsStream`); at rest: keyed **HMAC** policy (tamper-evident) — CRC-16 catches accidental corruption only. See [at-rest cryptography](security/at-rest-cryptography.md) |
| **(e)** — protect the confidentiality of stored, transmitted or processed data (e.g. by encryption) | protect records on the wire and at rest | TLS in transit (`SolidSyslogTlsStream`); **AES-GCM** authenticated-encryption at-rest policy. See [at-rest cryptography](security/at-rest-cryptography.md) |
| **(d)** — ensure protection from unauthorised access by appropriate control mechanisms | authenticate the sender; surface anomalies | mutual TLS device identity (`SolidSyslogTlsStream`); the error-reporting subsystem (`SolidSyslog_SetErrorHandler`) surfaces delivery and storage anomalies |
| **(h)** — protect the availability of essential and basic functions, incl. resilience against denial-of-service | the audit function survives outages and pressure | store-and-forward (`SolidSyslogBlockStore`); non-blocking buffering (`SolidSyslogCircularBuffer`); caller-chosen discard / threshold policies bound resource use |
| **(b)** — be made available with a secure by default configuration | safe out of the box | Null-object secure defaults; TLS pins verify-peer, floors at TLS 1.2, and treats mutual TLS as all-or-nothing (no silent downgrade) |

Where embedded targets matter, the same TLS capability is realised by
`SolidSyslogMbedTlsStream` over Mbed TLS
([integrating Mbed TLS](integrating-mbedtls.md)) and the same at-rest policies by
their Mbed TLS variants — the requirement maps to the *role*, not to a specific
vendor adapter.

## Annex I Part II — vulnerability handling (met at product level)

Part II places process duties on the manufacturer of the finished product, not on
a logging component. SolidSyslog supports two of them directly, and they are
documented in full elsewhere rather than restated here:

- **(1)** — identify and document vulnerabilities and components, including a
  software bill of materials in a machine-readable format: the project publishes a
  CycloneDX [SBOM](security/sbom.md) per release, ready to fold into your product's
  own bill of materials.
- **(5)** — put in place and enforce a coordinated vulnerability disclosure policy:
  see [`SECURITY.md`](../SECURITY.md) and the
  [vulnerability triage runbook](security/triage-runbook.md).

These support the manufacturer's Part II duties for the finished product; they do
not discharge them on their own.

## Reference designs

The three [reference designs](reference-designs.md) map cleanly onto three CRA
risk postures:

- **A minimal device** — the least that satisfies **(2)(l)**: a valid,
  timestamped RFC 5424 record delivered to a collector.
- **A sensible secure device** — adds confidentiality and integrity in transit
  (**(2)(e)**, **(2)(f)**), availability across outages (**(2)(h)**), and
  trustworthy, gap-visible records.
- **Hardened against a state-level attacker** — adds integrity at rest
  (**(2)(f)**), authenticated provenance and protection from unauthorised access
  (**(2)(d)**), and availability under resource exhaustion (**(2)(h)**).

## Where to go next

- [Reference designs](reference-designs.md): the three risk postures, wired.
- [IEC 62443 compliance guide](iec62443.md): the parallel control-by-control map.
- [Choosing components by Security Level](security-levels.md): the choices you make.
- [Compliance in one page](overview.md): the evaluator's one-screen orientation.
