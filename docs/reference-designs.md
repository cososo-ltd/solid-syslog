# Reference designs

No standard mandates syslog. IEC 62443 and the EU Cyber Resilience Act (CRA)
state *capabilities* an audit-logging function should have — auditable events,
integrity, confidentiality, availability, non-repudiation — not the protocol you
realise them with. These three reference designs show how SolidSyslog realises
the audit-logging slice of those capabilities at three risk postures: a
**minimal** device, a **sensible secure** device, and a device **hardened
against a state-level attacker**. They are starting points you adapt to your own
drivers, not a parts list you must ship.

> [!NOTE]
> IEC 62443 certifies systems, not components, and a Security Level is a
> property of your whole system, its deployment, and its assessment, not of a
> parts list. Likewise, the CRA places its obligations on the economic operators
> who bring a product to market (manufacturers, and in defined cases importers
> and distributors), not on any single component. This is guidance on how
> SolidSyslog's components help you address the audit-logging aspects of these
> frameworks; it is not a guarantee of compliance, and no substitute for
> assessment of your full product.

Each design points back to the full [choices table](security-levels.md) for the
driver behind every decision, and forward to the framework maps
([IEC 62443](iec62443.md), [CRA](cra.md)) for the control-by-control detail.

<!-- markdownlint-disable MD033 — these diagrams are embedded as <object>, not a Markdown image, so the SVG's interface stickies stay clickable through to their API pages. -->

## Design 1 — A minimal device

<div class="postit-diagram">
  <object type="image/svg+xml" data="../assets/postit/sl1.svg" title="A minimal device">
    <img src="assets/postit/sl1.svg" alt="A minimal device">
  </object>
</div>

Core (`SolidSyslog` + `Config`) + an injected clock + `UdpSender` +
`PassthroughBuffer`; store, policy, and structured data all `Null`. The least
that emits a valid, timestamped RFC 5424 record to your collector.

Why: a trusted internal network, a single task with time to send inline, and no
audit-loss budget yet. Most drivers are off, so most roles stay Null and cost
nothing.

*Maps to:* IEC 62443 SL1 · CRA Annex I Part I (2)(l) — recording and monitoring
relevant internal activity — at its most basic.

> **Working example:** *(link to a tagged release of the examples repo — forthcoming)*

## Design 2 — A sensible secure device

<div class="postit-diagram">
  <object type="image/svg+xml" data="../assets/postit/sl2.svg" title="A sensible secure device">
    <img src="assets/postit/sl2.svg" alt="A sensible secure device">
  </object>
</div>

Adds a server-authenticated TLS transport (`SolidSyslogTlsStream`) for
confidentiality and integrity over the untrusted hop, `SolidSyslogBlockStore`
store-and-forward so records survive an outage or reboot, and
`SolidSyslogTimeQualitySd` + `SolidSyslogMetaSd` sequenceId so the collector can
trust the timestamps and see the gaps. Reach for mutual TLS if the receiver must
authenticate the device.

Why: the log path leaves your trust boundary, records must not be lost across an
outage, and the collector needs both timestamp trust and gap visibility.

*Maps to:* IEC 62443 SL2 · CRA confidentiality and integrity in transit
(Annex I Part I (2)(e), (2)(f)), availability across outages (2)(h), and
trustworthy, gap-visible records (2)(l).

> **Working example:** *(link to a tagged release of the examples repo — forthcoming)*

## Design 3 — Hardened against a state-level attacker

<div class="postit-diagram">
  <object type="image/svg+xml" data="../assets/postit/sl3.svg" title="Hardened against a state-level attacker">
    <img src="assets/postit/sl3.svg" alt="Hardened against a state-level attacker">
  </object>
</div>

<!-- markdownlint-enable MD033 -->

Adds mutual TLS with a per-device certificate (`SolidSyslogTlsStream`), a keyed
at-rest integrity policy (HMAC is tamper-evident; CRC-16 catches only accidental
corruption, not an attacker — see
[at-rest cryptography](security/at-rest-cryptography.md)), `SolidSyslogOriginSd`
with sysUpTime for authenticated provenance, and a discard policy with the
matching store-full response (`OnStoreFull` under the halt policy, or the
threshold callback for a pre-full warning).

Why: the receiver must be able to prove the origin of each record, stored records
may be physically reachable, and storage exhaustion needs a defined,
caller-chosen response.

*Maps to:* IEC 62443 SL3 rising to SL4 · CRA integrity at rest
(Annex I Part I (2)(f)), authenticated provenance and protection from
unauthorised access ((2)(d)), and availability under resource exhaustion (2)(h).

> **Working example:** *(link to a tagged release of the examples repo — forthcoming)*

On embedded targets the same TLS capability is realised by
`SolidSyslogMbedTlsStream` over Mbed TLS — see
[integrating Mbed TLS](integrating-mbedtls.md). The reference design is
transport-agnostic: the role is *TLS transport*, and the vendor adapter is an
implementation detail.

## Where to go next

- [Choosing components by Security Level](security-levels.md): the full choices
  table and the driver behind every decision.
- [IEC 62443 compliance guide](iec62443.md): the control-by-control map.
- [CRA guide](cra.md): the Annex I essential requirements these designs help
  address.
- [Getting started](getting-started.md): wiring the components you chose.
