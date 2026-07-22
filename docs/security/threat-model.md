# SolidSyslog Threat Model

## Purpose and how to read this

SolidSyslog is a library component, not a running system. It has no process,
privileges, or network identity of its own; it is compiled into *your* product
and runs with *your* trust and *your* privileges. This document therefore cannot
be a threat model of a deployed system; only you can produce that, because only
you know your device, your network, and your adversaries.

What this document does instead is give you the two things you need to fold
SolidSyslog into *your* threat model:

1. Where SolidSyslog sits across trust boundaries, so you know which edges of
   your system it touches.
2. The division of responsibility: what the library defends by construction,
   and what it delegates to you by contract. A component's security is largely a
   contract; this document states SolidSyslog's side of it.

It is written for integrators. It is not a compliance certificate, but it is
citeable as supplier-diligence evidence (see *Compliance* below).

## Scope

What SolidSyslog does: formats RFC 5424 syslog messages and transports them
(RFC 5426 UDP, RFC 6587 TCP, RFC 5425 TLS), with optional asynchronous buffering
and store-and-forward persistence.

What SolidSyslog does not do: it is not a log server, aggregator, parser,
analyser, or redactor. It does not inspect, sanitise, or make security judgements
about the *content* you ask it to log. It does not manage keys, certificates, or
identities; it consumes ones you provide.

## Data flow

A single log record travels:

```text
your code ──Log()──▶ buffer ──Service()──▶ store ──▶ sender ──▶ TLS/TCP/UDP ──▶ receiver
              (in-process)      (optional persistence)     (network)
```

Two branches matter for security: the record leaves the process at the network
edge, and, if store-and-forward is configured, it comes to rest on a storage
medium (file/flash) that may outlive the process and may be physically
accessible.

## Trust boundaries

| # | Boundary | Nature | SolidSyslog's role |
|---|---|---|---|
| B1 | Your code ↔ library | In-process API surface | The library trusts its caller but defends its own invariants and the wire framing (see *Defended by construction*). |
| B2 | Library ↔ network | Real boundary — the network is untrusted | TLS/mTLS (RFC 5425) provides confidentiality, authenticity, and hop integrity. Plain UDP/TCP provide none. |
| B3 | Library ↔ storage medium | Real boundary if the medium is physically accessible (removable/embedded/stolen) | Optional at-rest integrity (CRC-16) and cryptographic policies (HMAC-SHA256, AES-GCM). |
| B4 | Log producer ↔ consumer (the buffer) | Conditional — see below | None. Buffer contents are treated as trusted bytes. |

### B4 — the conditional buffer boundary

The `SolidSyslogBuffer` extension point decouples `Log()` (producer) from
`Service()` (consumer). Whether this is a trust boundary depends on the
implementation you wire:

- PassthroughBuffer / CircularBuffer: producer and consumer are threads in
  one address space. Not a trust boundary: any thread that can write the ring
  can already do anything to the process. The injected mutex is for concurrency
  correctness, not security.
- PosixMessageQueueBuffer: the record transits a kernel message queue. The
  library creates that queue `0600` (owner read/write only), so in the
  supported single-process, same-UID configuration it stays within one trust
  domain by construction.

Assumption (current): the `Log()`→`Service()` dataflow stays within a single
trust domain. Under that assumption no boundary exists and none is enforced; the
library applies no integrity or authenticity check across the buffer path.
(Contrast the at-rest *store*, which does.)

Your obligation: if you plug in a custom Buffer, or expose a message queue,
such that the dataflow crosses a process or privilege boundary (a multi-process
broker, a shared-memory ring across privilege domains, a queue read by a
lower-privileged consumer), then you own authenticity, integrity, and
access-control across it. The library will treat whatever it reads back as trusted.

## Assets

- Log content in transit: confidentiality/integrity between you and the receiver (B2).
- Log content at rest: confidentiality/integrity of buffered/stored records (B3).
- The receiver's trust in log authenticity: that records came from this device, unmodified (B2, and end-to-end: see gaps).
- Device liveness: that a hung/hostile receiver cannot stall your logging thread (B2).

## Threat actors

| Actor | In scope | SolidSyslog's answer |
|---|---|---|
| Passive network eavesdropper | Yes | TLS (RFC 5425). |
| Active network MITM | Yes | TLS server-cert + hostname validation; mutual TLS for peer authentication. |
| Device-physical-access / media theft | Yes | At-rest HMAC-SHA256 / AES-GCM policies (opt-in). Residual if none chosen. |
| Malicious or buggy caller | Partial | Library defends its own invariants and wire framing; trusts the caller for content and process integrity. |
| Nation-state exploitation of the underlying TLS/OS | No | Delegated to the platform crypto/OS. |
| Root / same-UID host principal | No | Same trust domain by definition. |

## Defended by construction

These are properties of the shipped code, not aspirations:

- No dynamic allocation. Every stateful object lives in a static pool. There is
  no heap use-after-free, double-free, or allocator exhaustion *originating in the
  library*; memory footprint is bounded and known at link time.
- The library owns the wire framing. Header field sinks bound field widths and
  substitute out-of-charset bytes; the SD-value writer's escaper is the single
  source of truth for `"`, `\`, and `]` plus per-byte ill-formed substitution.
  Consequently a caller- or callback-supplied hostname, app-name, or structured-data
  value cannot break header framing or forge/inject a structured-data element:
  syslog injection via logged values is prevented by construction.
- Null-object pattern throughout. An unwired dependency dispatches to a safe
  no-op, not a NULL dereference.
- Bounded, non-blocking transport. TCP connects are non-blocking with a bounded
  `select()` deadline and fail-fast on refusal; Send/Read never block on a wedged
  peer or a full kernel buffer. A hostile or dead receiver cannot stall your
  service thread.
- Graceful degradation under pressure. Store-and-forward offers explicit
  discard policies (oldest / newest / halt) and threshold/halt callbacks, so a
  backlog or a network outage has a defined, caller-chosen outcome rather than
  unbounded growth.
- Transport security (opt-in). TLS 1.2+ (RFC 5425): server-cert validation,
  hostname verification, cipher pinning, optional mutual TLS.
- At-rest protection (opt-in). CRC-16 for accidental-corruption integrity;
  HMAC-SHA256 for tamper-evidence; AES-GCM for confidentiality + integrity, each
  available for both the OpenSSL and Mbed TLS reference integrations.

## Your obligations (the contract)

| You must | Because |
|---|---|
| Not log secrets you don't want transported/stored | The library is a transport, not a redactor — it never inspects content. |
| Provision and validate TLS/mTLS certificates; supply the CA bundle and cipher policy | The library consumes trust material; it does not mint or manage it. |
| Resolve and trust the destination address | The library connects to whatever address the injected resolver returns; it does not authenticate DNS responses. On targets without DNS you supply the address directly. |
| Supply a properly-seeded RNG (Mbed TLS `ctr_drbg`) | A weak RNG silently weakens TLS. The library uses the RNG you inject. |
| Inject a real mutex (CircularBuffer) / config-lock (multi-task pools) where concurrency exists | The library's synchronisation primitives are injected; the defaults are no-ops. |
| Keep `Log()`→`Service()` within one trust domain, or secure a boundary-crossing Buffer yourself | The library does not check the buffer dataflow (B4). |
| Protect the storage medium and/or select an at-rest policy for sensitive logs | Physical extraction of an unprotected medium discloses stored content. |
| Own the underlying TLS/crypto library: selection, patching, CVE response | SolidSyslog rolls no crypto; it links yours. |
| Size buffers/records via the `*_MAX_*` tunables and manage object lifecycle | The library enforces its bounds against these values, not against your intent. |

## Not defended against (and whose problem it is)

- A compromised calling process: out of scope. The library trusts its caller by
  definition; if your process is owned, so is anything it logs.
- Log content confidentiality/correctness: yours. See obligations.
- End-to-end integrity through log relays: not provided. TLS protects each
  hop only; once a relay terminates the connection, SolidSyslog offers no
  cryptographic guarantee that downstream records are unaltered. RFC 5848 message
  signing is the standard answer and is not implemented.
- Replay of captured records: TLS prevents replay within a live session, but
  SolidSyslog adds no cryptographic anti-replay of its own. Anyone able to
  re-inject records downstream of a terminated TLS hop (a malicious or compromised
  relay) can replay valid records undetected, the same root cause as the
  end-to-end integrity gap above. Receiver-side de-duplication on the RFC 5424
  `sequenceId` / timestamp is the mitigation; note `sequenceId` is informational,
  not cryptographically bound.
- TLS certificate revocation (CRL/OCSP): the library performs no revocation
  checking itself. Whether revocation is enforced depends on the TLS backend and
  platform you configure; treat it as your responsibility unless your backend
  guarantees it.
- Cryptographic attacks on the underlying TLS/crypto library: delegated to
  OpenSSL / Mbed TLS.
- Network denial of service: a sender cannot prevent it. The library degrades
  gracefully (store-and-forward, discard policies) but cannot keep packets flowing.
- Physical extraction of stored logs when no at-rest policy is selected: the
  default store is plaintext + CRC-16 (integrity, not confidentiality).

## Residual risks

- Non-constant-time formatting. Formatting is not constant-time; a co-resident
  timing attacker could in principle infer something about content. Syslog content
  is not typically secret-bearing, but if yours is, treat this as residual.
- Caller-driven message size. The library bounds against its `*_MAX_*`
  tunables, but does not police whether your *intended* record fits your configured
  limits; oversize content is truncated at the boundary, not rejected upstream.

## Compliance

This model supports SolidSyslog's IEC 62443 / EU CRA positioning as a
security-conscious component supplier. See [`iec62443.md`](../iec62443.md) for
component selection by Security Level, [`cra.md`](../cra.md) for the CRA Annex I
audit-trail map, and [`rfc-compliance.md`](../rfc-compliance.md)
for the standards coverage matrix. The public disclosure process for issues found
against this model is in [`SECURITY.md`](../../SECURITY.md).

## Review policy

This is a living document. It is reviewed on any architectural change that alters a
trust boundary or the division of responsibility: a new transport, a new field
type, a new platform backend, or a new extension point. The review lands in the
pull request that makes the change.
