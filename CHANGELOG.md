# Changelog

## 0.1.0 (2026-08-18)

First public release. SolidSyslog is a structured syslog client library for
embedded and industrial systems, built to give a shipping product the security
audit trail the EU Cyber Resilience Act and IEC 62443 expect. Everything in this
release is new; the generated change list begins at 0.2.0.

Released as 0.x deliberately: the beta label describes the breadth of platform
coverage and the absence of field integrations to date, not the maturity of the
code. The public API changes before 1.0.0 only if integration feedback or a
security fix requires it.

### What ships

- RFC 5424 structured formatting over UDP (RFC 5426), TCP (RFC 6587), and TLS
  or mutual TLS (RFC 5425)
- Asynchronous buffering and rotating block store-and-forward
- At-rest record protection: CRC-16 against accidental corruption, HMAC-SHA256
  for tamper evidence, AES-256-GCM for authenticated encryption
- C99, no dynamic allocation. Every instance lives in a static pool sized at
  compile time. Every platform dependency (network stack, TLS library,
  filesystem, OS primitives, clock) is injected behind a vtable; Core carries
  no reference to any of them. MISRA C:2012 informed
- Source only; there are no binary artefacts

Platforms: Posix, Windows, FreeRTOS, FreeRTOS-Plus-TCP, lwIP (Raw API),
OpenSSL, Mbed TLS, FatFs, FreeRTOS-Plus-FAT, C11 atomics.

### RFC compliance at this release

| RFC | Total | Supported | Partial | Not Met | N/A |
|---|---|---|---|---|---|
| RFC 5424 | 40 | 33 | 0 | 0 | 7 |
| RFC 5425 | 20 | 13 | 1 | 1 | 5 |
| RFC 5426 | 17 | 8 | 0 | 0 | 9 |
| RFC 6587 | 8 | 7 | 0 | 0 | 1 |

The maintainer's assessment, not a certification. Each status describes the
library with a conforming platform supplying the roles it needs, and depends on
the components selected, including any you write yourself, which the library
cannot speak for. The full matrix at this release, one row and one note per
clause:
[`docs/rfc-compliance.md` at `v0.1.0`](https://github.com/cososo-ltd/solid-syslog/blob/v0.1.0/docs/rfc-compliance.md).

### Known limitations

Found by a pre-release audit that read every documentation page against the
code it describes. Each is disclosed where the reader meets it: on the page for
the platform it affects, or in the TLS contract and the compliance matrix. All
are tracked for 0.2.0.

TLS divergences from the contract in
[`docs/tls.md`](https://github.com/cososo-ltd/solid-syslog/blob/v0.1.0/docs/tls.md):

- [#731](https://github.com/cososo-ltd/solid-syslog/issues/731) - an expired
  peer certificate stops delivery, where the contract says report and continue
- [#732](https://github.com/cososo-ltd/solid-syslog/issues/732) - four
  `<Class>_Create` functions accept a configuration they cannot work without and
  report nothing
- [#733](https://github.com/cososo-ltd/solid-syslog/issues/733) - the cipher
  policy an integrator sets does not bind the connection that is negotiated
- [#734](https://github.com/cososo-ltd/solid-syslog/issues/734) - a
  half-supplied client credential stops delivery on the OpenSSL stream
- [#718](https://github.com/cososo-ltd/solid-syslog/issues/718) - the Mbed TLS
  stream discards the mutual-TLS credential install result, allowing a silent
  downgrade to server-authenticated TLS
- [#719](https://github.com/cososo-ltd/solid-syslog/issues/719) - the Mbed TLS
  stream does not validate the mutual-TLS key against its certificate
- [#753](https://github.com/cososo-ltd/solid-syslog/issues/753) - a peer cannot
  yet be authorised by certificate fingerprint (RFC 5425 §5.1)

Transport:

- [#736](https://github.com/cososo-ltd/solid-syslog/issues/736) - an oversize
  datagram is lost on a platform that cannot detect oversize. Not reachable at
  the default message size; it requires `SOLIDSYSLOG_MAX_MESSAGE_SIZE` raised
  above the payload the datagram reports
- [#743](https://github.com/cososo-ltd/solid-syslog/issues/743) - TCP keepalive
  timings are file-scope constants rather than tunables, so dead-peer detection
  differs by two orders of magnitude across adapters
- [#755](https://github.com/cososo-ltd/solid-syslog/issues/755) - FreeRTOS
  `sysUpTime` wraps early at tick rates that do not divide 100, including the
  1000 Hz default

The report-and-continue posture behind the TLS items is stated and argued in
[`docs/tls.md`](https://github.com/cososo-ltd/solid-syslog/blob/v0.1.0/docs/tls.md).
It is reasoned rather than field-tested, and 0.x is when integration feedback
can still change it cheaply.

### Verifying this release

Four assets are attached: the CycloneDX SBOM, the content-tree SHA-256, and a
cosign signature bundle for each. Signing is keyless via GitHub OIDC, so each
signature commits to the workflow run that produced it. There is no personal
key. The content-tree hash is reproducible from any clone. Commands:
[`docs/security/release-verification.md` at `v0.1.0`](https://github.com/cososo-ltd/solid-syslog/blob/v0.1.0/docs/security/release-verification.md).
The check that matters is that a bundle verifies, not that the assets are
present.
