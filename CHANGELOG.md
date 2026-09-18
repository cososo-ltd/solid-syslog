# Changelog

## [0.2.0](https://github.com/cososo-ltd/solid-syslog/compare/v0.1.0...v0.2.0) (2026-09-18)


### ⚠ BREAKING CHANGES

* S39 give the crypto roles their detail codes from Core ([#818](https://github.com/cososo-ltd/solid-syslog/issues/818))
* S39 give the TLS-stream role one portable error enum ([#814](https://github.com/cososo-ltd/solid-syslog/issues/814))
* S39 supply the TLS profile per connection, and bind cipher policy ([#812](https://github.com/cososo-ltd/solid-syslog/issues/812))
* the Mbed TLS stream asks a credentials source for its material ([#802](https://github.com/cososo-ltd/solid-syslog/issues/802))
* every *ErrorSource object declared in a public *Errors.h header gains the SolidSyslog prefix, so UdpSenderErrorSource becomes SolidSyslogUdpSenderErrorSource and the rest follow. An error handler that matches on source identity, event->Source == &UdpSenderErrorSource, must be updated to the new name. The matching rule itself is unchanged.
* the OpenSSL stream asks a credentials source for its material ([#799](https://github.com/cososo-ltd/solid-syslog/issues/799))

### Features

* a credentials role for each TLS pack ([#797](https://github.com/cososo-ltd/solid-syslog/issues/797)) ([7e446d0](https://github.com/cososo-ltd/solid-syslog/commit/7e446d05ef070a822928c1494c84f6b63f1c5f68))
* an Mbed TLS credentials backend that parses PEM per connection ([#803](https://github.com/cososo-ltd/solid-syslog/issues/803)) ([9ca9184](https://github.com/cososo-ltd/solid-syslog/commit/9ca918454e1acabd7b1dcbca311d4c27e2d46e34))
* S39 authorise a TLS peer by certificate fingerprint (Core + OpenSSL) ([#806](https://github.com/cososo-ltd/solid-syslog/issues/806)) ([f3e7445](https://github.com/cososo-ltd/solid-syslog/commit/f3e74456c006802cedde28f9acf1bc583b2f5446))
* S39 authorise a TLS peer by certificate fingerprint (Mbed TLS) ([#808](https://github.com/cososo-ltd/solid-syslog/issues/808)) ([7bb982a](https://github.com/cososo-ltd/solid-syslog/commit/7bb982a2d18be97abb95c5b1dd7c4b1960865729))
* S39 detect a TLS configuration change from the stream ([#809](https://github.com/cososo-ltd/solid-syslog/issues/809)) ([ebc17c9](https://github.com/cososo-ltd/solid-syslog/commit/ebc17c9685f6f444d1f99fca31be3c8553b9a99d))
* S39 supply the TLS profile per connection, and bind cipher policy ([#812](https://github.com/cososo-ltd/solid-syslog/issues/812)) ([804764e](https://github.com/cososo-ltd/solid-syslog/commit/804764e8d4e4c385750cd7e432c6290edc064457))
* the Mbed TLS stream asks a credentials source for its material ([#802](https://github.com/cososo-ltd/solid-syslog/issues/802)) ([f9db5f5](https://github.com/cososo-ltd/solid-syslog/commit/f9db5f5c8245a7f7cb38251bf1223b2e5e3beb1f))
* the OpenSSL stream asks a credentials source for its material ([#799](https://github.com/cososo-ltd/solid-syslog/issues/799)) ([c6720e3](https://github.com/cososo-ltd/solid-syslog/commit/c6720e39b09a445ceb997ea9e00ddfb5fcf495db))


### Bug Fixes

* check the Mbed TLS client key against its certificate ([#790](https://github.com/cososo-ltd/solid-syslog/issues/790)) ([05e8e68](https://github.com/cososo-ltd/solid-syslog/commit/05e8e68a4def8c8323fe0bb0516d5155bb7f89f3))
* check the wiring four Create functions cannot work without ([#791](https://github.com/cososo-ltd/solid-syslog/issues/791)) ([35446ab](https://github.com/cososo-ltd/solid-syslog/commit/35446ab13e8c4178dff8bfa0ab9aa1e824e88a8d))
* name the check that refused a TLS handshake ([#792](https://github.com/cososo-ltd/solid-syslog/issues/792)) ([1201eb4](https://github.com/cososo-ltd/solid-syslog/commit/1201eb40d2a8ae126538de37cb06f6bd740d3ee9))
* report a client credential a TLS stream cannot present, and keep delivering ([#788](https://github.com/cososo-ltd/solid-syslog/issues/788)) ([c5c7e3f](https://github.com/cososo-ltd/solid-syslog/commit/c5c7e3f5620d423bf87e3c4153435abd0798cc20))
* report a mutual-TLS credential the Mbed TLS stream cannot present ([#785](https://github.com/cososo-ltd/solid-syslog/issues/785)) ([0522448](https://github.com/cososo-ltd/solid-syslog/commit/0522448a8e081d6b5886ea56e71ebf4de92db605))
* S39 stop a closed lwIP pcb reaching the stream that replaced it ([#822](https://github.com/cososo-ltd/solid-syslog/issues/822)) ([20a1cce](https://github.com/cososo-ltd/solid-syslog/commit/20a1cce5d295fa7550f0a83a36bf9fb44f64f676))
* S39.01 close the findings from the E39 closing security audit ([#828](https://github.com/cososo-ltd/solid-syslog/issues/828)) ([4d07865](https://github.com/cososo-ltd/solid-syslog/commit/4d07865d68cb165dd0dcc86dbb2407d41962caec))
* S39.01 name an allocation failure in the linked TLS library, and act on the integration field report ([#831](https://github.com/cososo-ltd/solid-syslog/issues/831)) ([2878e46](https://github.com/cososo-ltd/solid-syslog/commit/2878e46a6b37055d4dc3ac61ea062424db95dfcf))
* S39.01 pin what the TLS adapters inherited, and rewrite the TLS docs ([#829](https://github.com/cososo-ltd/solid-syslog/issues/829)) ([8cb0281](https://github.com/cososo-ltd/solid-syslog/commit/8cb0281456cf054124b678776388ad01c629c983))
* S39.04 report an address that does not match as a name mismatch, found by the TLS matrix ([#821](https://github.com/cososo-ltd/solid-syslog/issues/821)) ([ff9f7c4](https://github.com/cososo-ltd/solid-syslog/commit/ff9f7c440ec267cefdfc3a9e283cd0d9e711ab32))


### Refactoring

* prefix the public error sources ([#801](https://github.com/cososo-ltd/solid-syslog/issues/801)) ([047d94a](https://github.com/cososo-ltd/solid-syslog/commit/047d94ababb4bc801a1808e28bd00f08a905be28))
* S39 give the crypto roles their detail codes from Core ([#818](https://github.com/cososo-ltd/solid-syslog/issues/818)) ([5344bf7](https://github.com/cososo-ltd/solid-syslog/commit/5344bf75d57c3aff5455a73ef6dfe1bfa8ea0999))
* S39 give the TLS-stream role one portable error enum ([#814](https://github.com/cososo-ltd/solid-syslog/issues/814)) ([b6ef7a3](https://github.com/cososo-ltd/solid-syslog/commit/b6ef7a36be75e7fe81423fc8efc02fa9c7255227))

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
