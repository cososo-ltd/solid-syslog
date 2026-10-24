# Changelog

## [0.3.0](https://github.com/cososo-ltd/solid-syslog/compare/v0.2.0...v0.3.0) (2026-10-24)


### Features

* every axis of an error event names itself ([#1039](https://github.com/cososo-ltd/solid-syslog/issues/1039)) ([5e96a25](https://github.com/cososo-ltd/solid-syslog/commit/5e96a2595b1c936c1e5356f15c1bca706b12576a))
* every File and network pack reports the failing call and its native code ([#1052](https://github.com/cososo-ltd/solid-syslog/issues/1052)) ([cce9a8f](https://github.com/cososo-ltd/solid-syslog/commit/cce9a8f995d2a8d08eabb5df2fbe0dd0865b25e6))
* S41.01 Core builds for VxWorks 6.4 and boots in QEMU Malta ([#937](https://github.com/cososo-ltd/solid-syslog/issues/937)) ([02115ab](https://github.com/cososo-ltd/solid-syslog/commit/02115abbddde9084a14fce35d0b662553a7cb8ce))
* S41.02 VxWorks 6.4 pack skeleton and mutex ([#932](https://github.com/cososo-ltd/solid-syslog/issues/932)) ([2bcd6c8](https://github.com/cososo-ltd/solid-syslog/commit/2bcd6c818bb066e1eb72747f8d2c389b6a5230d3))
* S41.03 VxWorks 6.4 UDP datagram and IPv4 resolver ([#939](https://github.com/cososo-ltd/solid-syslog/issues/939)) ([ca677f0](https://github.com/cososo-ltd/solid-syslog/commit/ca677f0489dcebda627eb2fde8a209b25e168df8))
* S41.07 remote runner for the VxWorks 6.4 QEMU target ([#940](https://github.com/cososo-ltd/solid-syslog/issues/940)) ([f5399cb](https://github.com/cososo-ltd/solid-syslog/commit/f5399cb235550aad035d60f2c51eee63c4a7f29a))
* S41.09 VxWorks 6.4 atomic counter ([#960](https://github.com/cososo-ltd/solid-syslog/issues/960)) ([23ee76e](https://github.com/cososo-ltd/solid-syslog/commit/23ee76e51aa6ff7e5327b98e912ca152284d2794))
* S41.10 VxWorks 6.4 clock, uptime, hostname and sleep ([#956](https://github.com/cososo-ltd/solid-syslog/issues/956)) ([f27408d](https://github.com/cososo-ltd/solid-syslog/commit/f27408d10cbf998d17736358045cc47f0c51d5f8))
* S41.11 VxWorks 6.4 TCP stream ([#961](https://github.com/cososo-ltd/solid-syslog/issues/961)) ([67a86c3](https://github.com/cososo-ltd/solid-syslog/commit/67a86c3e4913a94289f68b7309fbff667f3713ca))
* S41.12 VxWorks 6.4 file, for the store ([#968](https://github.com/cososo-ltd/solid-syslog/issues/968)) ([b45e937](https://github.com/cososo-ltd/solid-syslog/commit/b45e937b2aebbdd6f99121ad9f8825cc8680b72f))
* S41.14 VxWorks 6.4 store on HRFS as well as dosFs ([#970](https://github.com/cososo-ltd/solid-syslog/issues/970)) ([195d3e7](https://github.com/cososo-ltd/solid-syslog/commit/195d3e79319794a669fefe2d6f1285ef931bbfe9))
* S41.15 VxWorks 6.4 file and store report the file system call that failed ([#976](https://github.com/cososo-ltd/solid-syslog/issues/976)) ([87da5fb](https://github.com/cososo-ltd/solid-syslog/commit/87da5fb396e7ece92b639febe1a9b331c0aae1bc))
* S41.16 VxWorks 6.4 datagram returns SENT only once the next hop is resolved ([#988](https://github.com/cososo-ltd/solid-syslog/issues/988)) ([4d15707](https://github.com/cososo-ltd/solid-syslog/commit/4d15707a27fe2ecc55997446bab06b370455decd))
* S42.01 the Plus-TCP datagram returns SENT only once the next hop is resolved ([#1036](https://github.com/cososo-ltd/solid-syslog/issues/1036)) ([69d4fe6](https://github.com/cososo-ltd/solid-syslog/commit/69d4fe66c80c4637f966c477c6b23fcf73b0151e))
* S43.01 the store freezes on its first media fault ([#1011](https://github.com/cososo-ltd/solid-syslog/issues/1011)) ([697a99d](https://github.com/cososo-ltd/solid-syslog/commit/697a99d76d7e6a9af3e047f69f5cd434f00ecb11))
* S43.02 the store discards a record that fails verification and carries on ([#1013](https://github.com/cososo-ltd/solid-syslog/issues/1013)) ([14f3da9](https://github.com/cososo-ltd/solid-syslog/commit/14f3da92db35b2b168dd3084ad47e54e9199d582))
* S43.03 a security policy's OpenRecord says whether it opened, rejected, or could not run ([#1014](https://github.com/cososo-ltd/solid-syslog/issues/1014)) ([57ea219](https://github.com/cososo-ltd/solid-syslog/commit/57ea21966bee69a86bbf6172d20e22e39b2704c3))
* S43.04 the store reports once when records start being lost ([#1015](https://github.com/cososo-ltd/solid-syslog/issues/1015)) ([8561254](https://github.com/cososo-ltd/solid-syslog/commit/85612548b28781f010711baf252bd58913f607c3))
* S43.05 buffers report once when records start being lost ([#1016](https://github.com/cososo-ltd/solid-syslog/issues/1016)) ([9dfdf91](https://github.com/cososo-ltd/solid-syslog/commit/9dfdf919c988a0043d569bad2b3d991ddb30d4d1))
* S43.07 with a store that keeps nothing, Service stops at the first failed send ([#1022](https://github.com/cososo-ltd/solid-syslog/issues/1022)) ([cfef5f1](https://github.com/cososo-ltd/solid-syslog/commit/cfef5f1fe7fa14b88c93fc2e4332a679f545504e))
* S43.08 the discard policy is set once, on the logger ([#1025](https://github.com/cososo-ltd/solid-syslog/issues/1025)) ([b1d8d59](https://github.com/cososo-ltd/solid-syslog/commit/b1d8d59bf844152631081bd4d003b44290d27637))
* S43.10 File Size reports failure rather than an empty file ([#1027](https://github.com/cososo-ltd/solid-syslog/issues/1027)) ([212fc60](https://github.com/cososo-ltd/solid-syslog/commit/212fc602f22e0879e4f7540b935ae10f178f6d6a))
* S43.11 buffers apply the discard policy ([#1028](https://github.com/cososo-ltd/solid-syslog/issues/1028)) ([20ea7f8](https://github.com/cososo-ltd/solid-syslog/commit/20ea7f8e9db6160682227b9b1befad0aae8713c8))
* the error event carries the platform's native code as a field ([#1042](https://github.com/cososo-ltd/solid-syslog/issues/1042)) ([436454d](https://github.com/cososo-ltd/solid-syslog/commit/436454d320f71dc5a64dc2f1d74432f2708e6f97))
* the integrator states the IPv4 payload limit where the stack cannot discover the path MTU ([#1035](https://github.com/cososo-ltd/solid-syslog/issues/1035)) ([4c73ade](https://github.com/cososo-ltd/solid-syslog/commit/4c73adee8a53b42ede28dd5957c7ee715a254d17))
* VxWorks 6.4 platform support ([6819c2b](https://github.com/cososo-ltd/solid-syslog/commit/6819c2b4f7f5752822424624bdb18eac1c3292bf))


### Bug Fixes

* every TCP stream bounds the connect deadline at five minutes ([#1031](https://github.com/cososo-ltd/solid-syslog/issues/1031)) ([fa83da7](https://github.com/cososo-ltd/solid-syslog/commit/fa83da757b10d24eb80bae4752f825b68d7c5b77))
* every transport refuses Open while it is already open, and reports it ([#1049](https://github.com/cososo-ltd/solid-syslog/issues/1049)) ([a01cc8e](https://github.com/cososo-ltd/solid-syslog/commit/a01cc8ed3f1bfdecef9ae0a409510739b9998cba))
* no stream takes a record after the collector has closed the connection ([#1053](https://github.com/cososo-ltd/solid-syslog/issues/1053)) ([dac9961](https://github.com/cososo-ltd/solid-syslog/commit/dac9961493f6d1acc300e2059c450666f87213ee))
* the block store's used bytes follow occupancy again after a refusal ([#1032](https://github.com/cososo-ltd/solid-syslog/issues/1032)) ([7b6c76b](https://github.com/cososo-ltd/solid-syslog/commit/7b6c76b5274c5c67e11fd5e95ed052c0cd906b48))
* the lwIP Raw resolvers refuse an IPv6 answer the transports cannot send to ([#1045](https://github.com/cososo-ltd/solid-syslog/issues/1045)) ([a70d9ad](https://github.com/cososo-ltd/solid-syslog/commit/a70d9ada5600f52d944aaef19ef5371199754c0a))
* the Mbed TLS stream tells the library it is deliberately not naming the peer ([#1044](https://github.com/cososo-ltd/solid-syslog/issues/1044)) ([89a6d99](https://github.com/cososo-ltd/solid-syslog/commit/89a6d99ea649e123a6e04d3369613429618683b6))
* the OpenSSL stream names a fingerprint mismatch ahead of an expired issuer ([#1034](https://github.com/cososo-ltd/solid-syslog/issues/1034)) ([29631ed](https://github.com/cososo-ltd/solid-syslog/commit/29631ed3ee497e65681f6441c9f26a0fc5853eeb)), closes [#919](https://github.com/cososo-ltd/solid-syslog/issues/919)
* VxWorks 6.4 Delete answers removed only for a removed or absent path ([#1040](https://github.com/cososo-ltd/solid-syslog/issues/1040)) ([44f243f](https://github.com/cososo-ltd/solid-syslog/commit/44f243f72cd907ec3c2dbecaef4a0db12524291d))
* VxWorks 6.4 Exists asks stat, not open, whether a path is there ([#1041](https://github.com/cososo-ltd/solid-syslog/issues/1041)) ([7019a30](https://github.com/cososo-ltd/solid-syslog/commit/7019a30dbbb07d31e7371879ba99b034b2c83d1d))
* VxWorks 6.4 review sweep - resolver, runner hardening, and comments and docs said once ([#981](https://github.com/cososo-ltd/solid-syslog/issues/981)) ([bd687bc](https://github.com/cososo-ltd/solid-syslog/commit/bd687bc3918b525c1f5efb26eed82e585cce9614))

## [0.2.0](https://github.com/cososo-ltd/solid-syslog/compare/v0.1.0...v0.2.0) (2026-09-28)

The TLS release. 0.2.0 lets a device authorise its collector by certificate
fingerprint, gives each TLS pack a credentials role of its own, and closes every
limitation 0.1.0 shipped with. It also adds three platform packs aimed at
microcontroller targets.

Still 0.x, for the reason 0.1.0 gave: the label describes platform breadth and
the absence of field integrations, not the maturity of the code. This release
does break the public API. Every break is listed below, under *Before you
upgrade*.

### What's in this release

- **Fingerprint pinning (RFC 5425 section 5.1).** A collector can be authorised by the
  fingerprint of its certificate, with no CA involved, so a closed site with no
  PKI can run TLS. More than one pin can be held at once, so a collector's
  certificate renewal can be crossed without stopping delivery. A CA chain can be required
  as well as the pin.
- **A credentials role per TLS pack.** The OpenSSL and Mbed TLS streams fetch
  their trust anchors, client credential and pins from a credentials source when
  they connect, and let go of them when the connection ends. A file, a
  caller-built handle, a secure element or a keyring can each back it.
- **TLS policy per connection.** The expected peer name, the cipher policy and,
  on Mbed TLS, the certificate profile are asked for at each connection. The
  cipher policy an integrator sets now binds the connection that is negotiated.
  A configuration change is picked up without a restart when the stream's
  version moves.
- **Certificate validity is enforced by contract.** A peer certificate outside
  its validity period stops delivery. 0.1.0 already behaved this way, but its
  contract said report and continue; the contract now says what the code does,
  and `docs/tls.md` gives the reasoning.
- **One portable set of detail codes per role.** A handler that matches on
  detail codes reacts the same way whichever backend raised the code.
- **TCP.** Keepalive timings are tunables shared by every TCP backend, and a
  failed connect reports which step gave up.
- **An oversize datagram is trimmed** on a platform that can only report a
  failure, not name the size.

Platforms added: lwIP Sockets API, CMSIS-RTOS2 (mutex and uptime), and
LittleFS. The full list is POSIX, Windows, C11 atomics, OpenSSL, Mbed TLS, lwIP
Raw API, lwIP Sockets API, FreeRTOS-Plus-TCP, FreeRTOS, CMSIS-RTOS2, ChaN FatFs,
FreeRTOS-Plus-FAT and LittleFS.

### Before you upgrade

These break source compatibility with 0.1.0:

- **Error enums by role.** The per-pack `SolidSyslog<Pack><Class>Errors` enums
  for the Address, Datagram, Resolver, File, Mutex and AtomicCounter roles are
  replaced by `SolidSyslog<Role>Errors`, and their constants by
  `SOLIDSYSLOG_<ROLE>_ERROR_*`. The values are unchanged, so a handler matching
  on numbers is unaffected. The TCP streams, the TLS streams and the crypto
  roles each have one set of codes too.
- **`ErrorSource` names.** Every `*ErrorSource` object gains the `SolidSyslog`
  prefix: `UdpSenderErrorSource` becomes `SolidSyslogUdpSenderErrorSource`. A
  handler that matches on source identity must use the new names.
- **TLS stream configuration.** The OpenSSL and Mbed TLS streams take a
  credentials source instead of certificate and key fields, and take their peer
  name and cipher policy from a per-connection profile. The TLS setup pages show
  the new wiring.

Documentation corrected in this release, where acting on the old text would have
cost you:

- A consumer of the OpenSSL pack links `OpenSSL::SSL OpenSSL::Crypto` alongside
  `SolidSyslog`. The library does not link OpenSSL for you, and the docs said it
  did.
- A structured data element registered in `SolidSyslogConfig.Sd[]`, and the
  array itself, must outlive the logger, not only the call that creates it.
- FreeRTOS-Plus-TCP needs `ipconfigUSE_DNS=1` even when the collector is given as
  a numeric address.
- The POSIX pack needs Linux, not just a POSIX system.
- LittleFS has no usable default for `block_cycles`. Set it.
- A per-platform CMake switch such as `-DSOLIDSYSLOG_LWIPRAW=ON` takes effect
  only when `SOLIDSYSLOG_PLATFORMS` is `Auto`.
- A stored record that fails verification on read is discarded without a report,
  and shows at the collector as a gap in the sequence number. 0.1.0's IEC 62443
  and CRA pages said it was reported.

### RFC compliance at this release

| RFC | Total | Supported | Partial | Not Met | N/A |
|---|---|---|---|---|---|
| RFC 5424 | 40 | 33 | 0 | 0 | 7 |
| RFC 5425 | 21 | 16 | 0 | 0 | 5 |
| RFC 5426 | 16 | 7 | 0 | 0 | 9 |
| RFC 6587 | 8 | 7 | 0 | 0 | 1 |

RFC 5425 is now met throughout.
- **section 5.1**, authorising a peer by certificate fingerprint, moves from Not Met to
  Supported.
- **section 4.2.3** moves from Partial to Supported now that the cipher policy binds the
  connection, and gains a row for session resumption.

RFC 5426 has one row fewer because its two section 3.2 rows are now one. No clause
changed status. RFC 5424 and RFC 6587 are unchanged.

The maintainer's assessment, not a certification. Each status describes the
library with a conforming platform supplying the roles it needs, and depends on
the components selected, including any you write yourself, which the library
cannot speak for. The full matrix at this release, one row and one note per
clause:
[`docs/rfc-compliance.md` at `v0.2.0`](https://github.com/cososo-ltd/solid-syslog/blob/v0.2.0/docs/rfc-compliance.md).

### Known limitations

Every limitation 0.1.0 shipped with is resolved in this release.

The pre-release audit of 0.2.0 read every documentation page against the code
again. Each finding is disclosed on the page for the affected platform, or in the
compliance guides:

- [#919](https://github.com/cososo-ltd/solid-syslog/issues/919) - the OpenSSL
  stream reports an expired issuer ahead of a pin that matches nothing, where the
  contract puts the pin first. The peer is refused either way; only the reported
  code differs. Tracked for 0.3.0.
- [#921](https://github.com/cososo-ltd/solid-syslog/issues/921) - a stored record
  that fails verification on read, and a failed store write on most file
  backends, reach no error handler. Tracked for 0.3.0.
- [#842](https://github.com/cososo-ltd/solid-syslog/issues/842) - on an lwIP
  build with IPv6 enabled, the lwIP Raw API resolver accepts an IPv6 literal the
  datagram can never send to, so every send fails without saying why. Give the
  collector as an IPv4 address.

The Mbed TLS page also states three properties of Mbed TLS rather than of this
library:
- validity dates are checked only where the build carries a clock;
- an address literal can also match a DNS name spelling the same digits;
- the X.509 profile is the linked library's default unless `CertProfile` is set.

### Verifying this release

Six assets are attached: the CycloneDX SBOM, the content-tree SHA-256, and the
offline documentation bundle, each with a cosign signature bundle. Signing is
keyless via GitHub OIDC, so each signature commits to the workflow run that
produced it. There is no personal key. The content-tree hash is reproducible from
any clone. Commands:
[`docs/security/release-verification.md` at `v0.2.0`](https://github.com/cososo-ltd/solid-syslog/blob/v0.2.0/docs/security/release-verification.md).
The check that matters is that a bundle verifies, not that the assets are
present.

The offline documentation bundle is new in this release: the documentation as
of this tag, readable with no server and no network.

### ⚠ BREAKING CHANGES

* the per-pack enums SolidSyslog<Pack><Class>Errors for the Address, Datagram, Resolver, File, Mutex and AtomicCounter roles are replaced by enum SolidSyslog<Role>Errors, and their SOLIDSYSLOG_<PACK>_<CLASS>_ERROR_* constants by SOLIDSYSLOG_<ROLE>_ERROR_*. The values are unchanged, so a handler matching on numbers is unaffected.
* give the TCP streams one set of detail codes ([#863](https://github.com/cososo-ltd/solid-syslog/issues/863))
* S39 give the crypto roles their detail codes from Core ([#818](https://github.com/cososo-ltd/solid-syslog/issues/818))
* S39 give the TLS-stream role one portable error enum ([#814](https://github.com/cososo-ltd/solid-syslog/issues/814))
* S39 supply the TLS profile per connection, and bind cipher policy ([#812](https://github.com/cososo-ltd/solid-syslog/issues/812))
* the Mbed TLS stream asks a credentials source for its material ([#802](https://github.com/cososo-ltd/solid-syslog/issues/802))
* every *ErrorSource object declared in a public *Errors.h header gains the SolidSyslog prefix, so UdpSenderErrorSource becomes SolidSyslogUdpSenderErrorSource and the rest follow. An error handler that matches on source identity, event->Source == &UdpSenderErrorSource, must be updated to the new name. The matching rule itself is unchanged.
* the OpenSSL stream asks a credentials source for its material ([#799](https://github.com/cososo-ltd/solid-syslog/issues/799))

### Features

* a credentials role for each TLS pack ([#797](https://github.com/cososo-ltd/solid-syslog/issues/797)) ([7e446d0](https://github.com/cososo-ltd/solid-syslog/commit/7e446d05ef070a822928c1494c84f6b63f1c5f68))
* an Mbed TLS credentials backend that parses PEM per connection ([#803](https://github.com/cososo-ltd/solid-syslog/issues/803)) ([9ca9184](https://github.com/cososo-ltd/solid-syslog/commit/9ca918454e1acabd7b1dcbca311d4c27e2d46e34))
* expose an optional Mbed TLS certificate profile ([#908](https://github.com/cososo-ltd/solid-syslog/issues/908)) ([8f24d9b](https://github.com/cososo-ltd/solid-syslog/commit/8f24d9bfedd3987f472d71eb8b4c04f6cb4541f4))
* give the TCP keepalive timings one set of tunables ([#847](https://github.com/cososo-ltd/solid-syslog/issues/847)) ([28dcdb2](https://github.com/cososo-ltd/solid-syslog/commit/28dcdb2918f22cfba827a133d963b5dd942c4ef6))
* report which step of a failed TCP connect gave up ([#865](https://github.com/cososo-ltd/solid-syslog/issues/865)) ([85af38d](https://github.com/cososo-ltd/solid-syslog/commit/85af38d830a666f4b1766c0090d94bd337b939d4))
* S35.01 lwIP Sockets Address and Resolver ([#886](https://github.com/cososo-ltd/solid-syslog/issues/886)) ([3f526b3](https://github.com/cososo-ltd/solid-syslog/commit/3f526b3b407cc9b1c0502a4986281356bf73f92d))
* S35.02 lwIP Sockets Datagram and TcpStream ([#887](https://github.com/cososo-ltd/solid-syslog/issues/887)) ([863aa3f](https://github.com/cososo-ltd/solid-syslog/commit/863aa3fe081587a240dd5203eb44acfdacf6533f))
* S35.03 run the BDD target's network through the lwIP Sockets API ([#893](https://github.com/cososo-ltd/solid-syslog/issues/893)) ([33a733f](https://github.com/cososo-ltd/solid-syslog/commit/33a733f6d4fd419275637e23cd0a7ec2712b699b))
* S36.01 a LittleFS File adapter ([#877](https://github.com/cososo-ltd/solid-syslog/issues/877)) ([032d322](https://github.com/cososo-ltd/solid-syslog/commit/032d322eb3c76b10e69ed366159f7825d53cd24d))
* S36.03 run the BDD target's store on LittleFS ([#881](https://github.com/cososo-ltd/solid-syslog/issues/881)) ([6174804](https://github.com/cososo-ltd/solid-syslog/commit/6174804924f0e5f37c9572a24ba582a11e94ae94))
* S39 authorise a TLS peer by certificate fingerprint (Core + OpenSSL) ([#806](https://github.com/cososo-ltd/solid-syslog/issues/806)) ([f3e7445](https://github.com/cososo-ltd/solid-syslog/commit/f3e74456c006802cedde28f9acf1bc583b2f5446))
* S39 authorise a TLS peer by certificate fingerprint (Mbed TLS) ([#808](https://github.com/cososo-ltd/solid-syslog/issues/808)) ([7bb982a](https://github.com/cososo-ltd/solid-syslog/commit/7bb982a2d18be97abb95c5b1dd7c4b1960865729))
* S39 detect a TLS configuration change from the stream ([#809](https://github.com/cososo-ltd/solid-syslog/issues/809)) ([ebc17c9](https://github.com/cososo-ltd/solid-syslog/commit/ebc17c9685f6f444d1f99fca31be3c8553b9a99d))
* S39 supply the TLS profile per connection, and bind cipher policy ([#812](https://github.com/cososo-ltd/solid-syslog/issues/812)) ([804764e](https://github.com/cososo-ltd/solid-syslog/commit/804764e8d4e4c385750cd7e432c6290edc064457))
* S40.01 add the fourth BDD target and its lanes ([#868](https://github.com/cososo-ltd/solid-syslog/issues/868)) ([bf0f792](https://github.com/cososo-ltd/solid-syslog/commit/bf0f792eeabb5e15593228aa5d8cf79192793e28))
* S40.02 a CMSIS-RTOS2 Mutex ([#871](https://github.com/cososo-ltd/solid-syslog/issues/871)) ([eed100c](https://github.com/cososo-ltd/solid-syslog/commit/eed100ccde9b50deaced750eb0b469c7c60fd6a1))
* S40.03 a CMSIS-RTOS2 uptime that survives its own tick rollover ([#872](https://github.com/cososo-ltd/solid-syslog/issues/872)) ([7be7d51](https://github.com/cososo-ltd/solid-syslog/commit/7be7d512d27c4ab4a56a1e0e8382675ab623f111))
* S40.04 run the BDD target's OS primitives through CMSIS-RTOS2 ([#876](https://github.com/cososo-ltd/solid-syslog/issues/876)) ([4da6840](https://github.com/cososo-ltd/solid-syslog/commit/4da6840d7f3bfac9c8c4389956909a35b4092518))
* the Mbed TLS stream asks a credentials source for its material ([#802](https://github.com/cososo-ltd/solid-syslog/issues/802)) ([f9db5f5](https://github.com/cososo-ltd/solid-syslog/commit/f9db5f5c8245a7f7cb38251bf1223b2e5e3beb1f))
* the OpenSSL stream asks a credentials source for its material ([#799](https://github.com/cososo-ltd/solid-syslog/issues/799)) ([c6720e3](https://github.com/cososo-ltd/solid-syslog/commit/c6720e39b09a445ceb997ea9e00ddfb5fcf495db))


### Bug Fixes

* carry FreeRTOS uptime past the tick counter's own rollover ([#844](https://github.com/cososo-ltd/solid-syslog/issues/844)) ([bbff2b0](https://github.com/cososo-ltd/solid-syslog/commit/bbff2b0fd904d223d0249b22508a28fa89e4fbc4))
* check the Mbed TLS client key against its certificate ([#790](https://github.com/cososo-ltd/solid-syslog/issues/790)) ([05e8e68](https://github.com/cososo-ltd/solid-syslog/commit/05e8e68a4def8c8323fe0bb0516d5155bb7f89f3))
* check the wiring four Create functions cannot work without ([#791](https://github.com/cososo-ltd/solid-syslog/issues/791)) ([35446ab](https://github.com/cososo-ltd/solid-syslog/commit/35446ab13e8c4178dff8bfa0ab9aa1e824e88a8d))
* class-qualify the file-scope macros in the File adapters ([#882](https://github.com/cososo-ltd/solid-syslog/issues/882)) ([cff8310](https://github.com/cososo-ltd/solid-syslog/commit/cff83108551cf122768b3590ce92b4f37b46e475))
* name the check that refused a TLS handshake ([#792](https://github.com/cososo-ltd/solid-syslog/issues/792)) ([1201eb4](https://github.com/cososo-ltd/solid-syslog/commit/1201eb40d2a8ae126538de37cb06f6bd740d3ee9))
* report a client credential a TLS stream cannot present, and keep delivering ([#788](https://github.com/cososo-ltd/solid-syslog/issues/788)) ([c5c7e3f](https://github.com/cososo-ltd/solid-syslog/commit/c5c7e3f5620d423bf87e3c4153435abd0798cc20))
* report a mutual-TLS credential the Mbed TLS stream cannot present ([#785](https://github.com/cososo-ltd/solid-syslog/issues/785)) ([0522448](https://github.com/cososo-ltd/solid-syslog/commit/0522448a8e081d6b5886ea56e71ebf4de92db605))
* report an unroutable destination as a connect that never started ([#869](https://github.com/cososo-ltd/solid-syslog/issues/869)) ([c9f0e35](https://github.com/cososo-ltd/solid-syslog/commit/c9f0e35b3eba9c1db2b4bfa6fdde57c0f62c2805))
* S39 stop a closed lwIP pcb reaching the stream that replaced it ([#822](https://github.com/cososo-ltd/solid-syslog/issues/822)) ([20a1cce](https://github.com/cososo-ltd/solid-syslog/commit/20a1cce5d295fa7550f0a83a36bf9fb44f64f676))
* S39.01 close the findings from the E39 closing security audit ([#828](https://github.com/cososo-ltd/solid-syslog/issues/828)) ([4d07865](https://github.com/cososo-ltd/solid-syslog/commit/4d07865d68cb165dd0dcc86dbb2407d41962caec))
* S39.01 name an allocation failure in the linked TLS library, and act on the integration field report ([#831](https://github.com/cososo-ltd/solid-syslog/issues/831)) ([2878e46](https://github.com/cososo-ltd/solid-syslog/commit/2878e46a6b37055d4dc3ac61ea062424db95dfcf))
* S39.01 pin what the TLS adapters inherited, and rewrite the TLS docs ([#829](https://github.com/cososo-ltd/solid-syslog/issues/829)) ([8cb0281](https://github.com/cososo-ltd/solid-syslog/commit/8cb0281456cf054124b678776388ad01c629c983))
* S39.04 report an address that does not match as a name mismatch, found by the TLS matrix ([#821](https://github.com/cososo-ltd/solid-syslog/issues/821)) ([ff9f7c4](https://github.com/cososo-ltd/solid-syslog/commit/ff9f7c440ec267cefdfc3a9e283cd0d9e711ab32))
* trim an over-large datagram on a platform that cannot name one ([#841](https://github.com/cososo-ltd/solid-syslog/issues/841)) ([ee2365c](https://github.com/cososo-ltd/solid-syslog/commit/ee2365c9f8630182bf7dc83523fad2b3d5ef1c0a))


### Refactoring

* give every platform role one set of detail codes ([#866](https://github.com/cososo-ltd/solid-syslog/issues/866)) ([8e8d3e4](https://github.com/cososo-ltd/solid-syslog/commit/8e8d3e420c85ca5c66a47230daa4a148484b0f7a))
* give the TCP streams one set of detail codes ([#863](https://github.com/cososo-ltd/solid-syslog/issues/863)) ([b612a1d](https://github.com/cososo-ltd/solid-syslog/commit/b612a1da5469ef39b36698b1efda1689d89a98c4))
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
  yet be authorised by certificate fingerprint (RFC 5425 section 5.1)

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
