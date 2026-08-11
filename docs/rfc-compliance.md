# RFC Compliance Matrix

SolidSyslog implements the sender (client) side of four syslog RFCs. This
document tracks which requirements are met, which are met with known
limitations, and which do not apply.

Status key:

- Supported: implemented and tested
- Partial: implemented with known limitations
- N/A: not applicable to a sender implementation, or applicable and deliberately
  excluded — the note says which, and why

A status describes the library: Core, and the role contracts it defines, with a
conforming platform supplying the roles it needs. Almost every requirement below
depends on which platform components are selected and how they are configured,
including components you write yourself, which the library cannot speak for.
Where a shipped platform does not meet an obligation, its own page records the
exception and links the issue tracking it, and the
[capability matrix](platforms/index.md) shows which platform fills which role.

## RFC 5424 — The Syslog Protocol

Checked against [RFC 5424](https://www.rfc-editor.org/rfc/rfc5424.html), Standards Track.

| Section | Requirement | Status | Notes |
|---|---|---|---|
| [5](https://www.rfc-editor.org/rfc/rfc5424.html#section-5) | Transport MUST NOT deliberately alter the message | Supported | The sender transmits the formatted message as the formatter produced it. The length prefix octet-counting framing adds is defined by the transport mapping (RFC 5425 §4.3.1, RFC 6587 §3.4.1) and frames the message rather than altering it |
| [5.1](https://www.rfc-editor.org/rfc/rfc5424.html#section-5.1) | Minimum transport mapping — TLS MUST, UDP SHOULD | Supported | Both ship. TLS is a Stream wrapped around a byte-transport Stream; UDP is `SolidSyslogUdpSender` over the Datagram role. Which platform supplies each is in the [capability matrix](platforms/index.md). §5.1 also RECOMMENDS deployments use TLS, which is a deployment choice rather than a library one |
| [6](https://www.rfc-editor.org/rfc/rfc5424.html#section-6) | PRINTUSASCII in header fields (codes 33-126) | Supported | Non-compliant bytes substituted with `?` at format time (HOSTNAME, APP-NAME, PROCID, MSGID) |
| [6.1](https://www.rfc-editor.org/rfc/rfc5424.html#section-6.1) | Message length — no upper limit; receivers MUST accept 480 octets and SHOULD accept 2048 | Supported | §6.1 bounds what a receiver accepts rather than what a sender emits, and permits messages above 2048. Default `SOLIDSYSLOG_MAX_MESSAGE_SIZE` = 2048 is therefore a sender-side choice: the largest message every conforming receiver should accept. Override it for memory-constrained MCUs via the standard tunable mechanism |
| [6.2](https://www.rfc-editor.org/rfc/rfc5424.html#section-6.2) | HEADER — seven-bit ASCII in an eight-bit field | Supported | Every HEADER field is written through the `SolidSyslogHeaderField` sink, which substitutes any byte outside PRINTUSASCII (33–126) with `?`. PRINTUSASCII is a subset of seven-bit ASCII, so the stricter substitution satisfies §6.2 as well as the per-field productions |
| [6.2.1](https://www.rfc-editor.org/rfc/rfc5424.html#section-6.2.1) | PRI — facility * 8 + severity | Supported | Invalid values fall back to `syslog.err` (facility 5, severity 3) |
| [6.2.2](https://www.rfc-editor.org/rfc/rfc5424.html#section-6.2.2) | VERSION = 1 | Supported | |
| [6.2.3](https://www.rfc-editor.org/rfc/rfc5424.html#section-6.2.3) | TIMESTAMP — derived from RFC 3339, with an optional 1–6 digit fraction | Supported | §6.2.3 restricts RFC 3339 rather than ISO 8601 generally, and its ABNF bounds TIME-SECFRAC at `1*6DIGIT` where RFC 3339 allows any number of digits. The library always writes the full 6, so microsecond resolution, with a UTC offset or `Z` |
| [6.2.3](https://www.rfc-editor.org/rfc/rfc5424.html#section-6.2.3) | TIMESTAMP — NILVALUE when clock unavailable | Supported | NilClock produces `-` |
| [6.2.3.1](https://www.rfc-editor.org/rfc/rfc5424.html#section-6.2.3.1) | TIMESTAMP examples | N/A | Illustrative. States no requirement of its own |
| [6.2.4](https://www.rfc-editor.org/rfc/rfc5424.html#section-6.2.4) | HOSTNAME — max 255 chars, PRINTUSASCII | Supported | Truncated to 255. Non-PRINTUSASCII bytes substituted with `?`. Written through the public `SolidSyslogHeaderField` sink (`SolidSyslogHeaderField_PrintUsAscii`); the underlying formatter is library-private |
| [6.2.5](https://www.rfc-editor.org/rfc/rfc5424.html#section-6.2.5) | APP-NAME — max 48 chars, PRINTUSASCII | Supported | Truncated to 48. Non-PRINTUSASCII bytes substituted with `?` |
| [6.2.6](https://www.rfc-editor.org/rfc/rfc5424.html#section-6.2.6) | PROCID — max 128 chars, PRINTUSASCII | Supported | Truncated to 128. Non-PRINTUSASCII bytes substituted with `?` |
| [6.2.7](https://www.rfc-editor.org/rfc/rfc5424.html#section-6.2.7) | MSGID — max 32 chars, PRINTUSASCII | Supported | Truncated to 32. Non-PRINTUSASCII bytes substituted with `?` |
| [6.3](https://www.rfc-editor.org/rfc/rfc5424.html#section-6.3) | STRUCTURED-DATA — SD-ELEMENTs or NILVALUE | Supported | Extensible via `SolidSyslogStructuredData` vtable |
| [6.3.1](https://www.rfc-editor.org/rfc/rfc5424.html#section-6.3.1) | SD-ELEMENT — brackets, SD-ID, space-separated SD-PARAMs | Supported | `SolidSyslogSdElement` owns the framing: `SolidSyslogSdElement_Begin` opens the bracket and writes the SD-ID, each `SolidSyslogSdElement_Param` writes a leading space and the name, and `SolidSyslogSdElement_End` closes the bracket. An author writing structured data cannot emit the delimiters directly |
| [6.3.2](https://www.rfc-editor.org/rfc/rfc5424.html#section-6.3.2) | SD-ID and PARAM-NAME conform to SD-NAME | Supported | `SolidSyslogSdElement` owns the brackets, the `@` and enterprise number, the separators and the quoting; it bounds each name to the 32 characters §6.3.2 allows and substitutes non-printable bytes and spaces. The three remaining SD-NAME exclusions — `=`, `]` and `"` — are the author's to observe, and are stated on `SolidSyslogSdElement_Begin`, as is §6.3.2's rule that an SD-ID appears at most once in a message. Names are written by the developer authoring the SD rather than carried from runtime data, so an invalid one fails visibly on the first run rather than on some input |
| [6.3.3](https://www.rfc-editor.org/rfc/rfc5424.html#section-6.3.3) | SD-PARAM value escaping (`]`, `\`, `"`) | Supported | `SolidSyslogSdValue` — every SD-PARAM value is written through this sink, which applies the escaping: RFC 3629 UTF-8 validated, ill-formed input substituted per-byte with U+FFFD (Unicode §3.9). `SolidSyslogOriginSd` streams software, swVersion, enterpriseId, and each ip into it; `SolidSyslogMetaSd` streams language via the integrator's `SolidSyslogSdValueFunction` callback. Both get the same escaping. |
| [6.3.4](https://www.rfc-editor.org/rfc/rfc5424.html#section-6.3.4) | Change control — a defined SD-ID or PARAM-NAME MUST NOT change meaning | N/A | Directed at whoever defines an SD-ID, not at an implementation emitting one. It binds the author of a custom SD-ID, and it binds this library for the IANA-registered elements it ships, whose syntax is fixed by §7 |
| [6.3.5](https://www.rfc-editor.org/rfc/rfc5424.html#section-6.3.5) | STRUCTURED-DATA examples | N/A | Illustrative. States no requirement of its own |
| [6.4](https://www.rfc-editor.org/rfc/rfc5424.html#section-6.4) | MSG — UTF-8 preferred | Supported | RFC 3629 UTF-8 validated at the formatter primitives (`SolidSyslogFormatter_BoundedString`), with ill-formed input substituted per-byte with U+FFFD (Unicode §3.9). MSG is prefixed with the §6.4 UTF-8 BOM (`%xEF.BB.BF`) unconditionally. A leading BOM in the caller's body is stripped, so the wire frame contains exactly one. Truncation preserves codepoint boundaries at both layers: the formatter clips at `SOLIDSYSLOG_MAX_MESSAGE_SIZE` without splitting a codepoint, and on UDP the sender walks back over any partial codepoint when the kernel reports `EMSGSIZE` for the path MTU. TCP/TLS streams fragment transparently at the transport layer and so do not need a path-MTU trim |
| [6.5](https://www.rfc-editor.org/rfc/rfc5424.html#section-6.5) | Message examples | N/A | Illustrative. States no requirement of its own |
| [7](https://www.rfc-editor.org/rfc/rfc5424.html#section-7) | Structured Data IDs — the IANA-registered SD-IDs are OPTIONAL | N/A | Scoping text for §7's elements. Its one requirement — that a syslog application be prepared to receive the defined number of characters in any valid UTF-8 code point, up to 6 octets each — is directed at receiving applications |
| [7.1](https://www.rfc-editor.org/rfc/rfc5424.html#section-7.1) | timeQuality SD — tzKnown, isSynced, syncAccuracy | Supported | `SolidSyslogTimeQualitySd` |
| [7.1.1](https://www.rfc-editor.org/rfc/rfc5424.html#section-7.1.1) | tzKnown — MUST be 1 when the time zone is known, 0 when in doubt | Supported | The `TzKnown` bool in `SolidSyslogTimeQuality` is emitted as `1` or `0`, so no other value can reach the wire. Which one is true is the integrator callback's to answer |
| [7.1.2](https://www.rfc-editor.org/rfc/rfc5424.html#section-7.1.2) | isSynced — MUST be 1 when synchronised to a reliable source, 0 when not | Supported | The `IsSynced` bool is emitted as `1` or `0` on the same basis as tzKnown |
| [7.1.3](https://www.rfc-editor.org/rfc/rfc5424.html#section-7.1.3) | syncAccuracy — MUST NOT be specified when isSynced is 0 | Supported | The obligation is stated on `SolidSyslogTimeQuality.SyncAccuracyMicroseconds`: a callback reporting `IsSynced` false leaves the field at `SOLIDSYSLOG_SYNC_ACCURACY_OMIT`. The library writes the parameter whenever the field holds any other value and does not consult `IsSynced`, so the pairing is the integrator's to keep — [#748](https://github.com/cososo-ltd/solid-syslog/issues/748) tracks enforcing it, which matters here because the values are runtime state rather than fixed configuration |
| [7.1.4](https://www.rfc-editor.org/rfc/rfc5424.html#section-7.1.4) | timeQuality examples | N/A | Illustrative. States no requirement of its own |
| [7.2](https://www.rfc-editor.org/rfc/rfc5424.html#section-7.2) | origin SD — software, swVersion, enterpriseId, ip | Supported | `SolidSyslogOriginSd` covers all four §7.2 parameters. `software`, `swVersion`, and `enterpriseId` are static strings supplied via `SolidSyslogOriginSdConfig`; the config strings are borrowed for the SD's lifetime and each is escaped per §6.3.3 by the `SolidSyslogSdValue` writer it is streamed into at Format time (no pre-formatted scratch storage). `ip` is repeatable per RFC 5424 §7.2 and sourced via two callbacks (`SolidSyslogOriginIpCountFunction`, `SolidSyslogOriginIpAtFunction`) so multi-homed hosts can reflect runtime address changes; the library asks for a count then loops 0 to N-1, opening an `ip` param per token (with a leading space) while the integrator's at-callback writes one IP value per call into the `SolidSyslogSdValue` it is handed, which applies the escaping. All four parameters are independently optional — a NULL field or NULL callback omits the corresponding parameter from the SD-ELEMENT. The library frames and escapes; the IP value length is the integrator's to bound (ultimately by `SOLIDSYSLOG_MAX_MESSAGE_SIZE`), as is the IP count. Bare `[origin]` with no parameters is RFC-legal (§7.2 marks all params OPTIONAL, no SHOULD enforcement) and is what the library emits when the integrator wires nothing |
| [7.2.1](https://www.rfc-editor.org/rfc/rfc5424.html#section-7.2.1) | ip — MUST be the textual representation of an IP address | Supported | The library opens one `ip` PARAM per index and escapes what the integrator's at-callback writes. §7.2.1 lets a multi-homed originator list one address or repeat the parameter, and the count callback chooses. The textual form of each address is the integrator's to produce |
| [7.2.2](https://www.rfc-editor.org/rfc/rfc5424.html#section-7.2.2) | enterpriseId — MUST be an IANA-registered private enterprise number | Supported | Stated on `SolidSyslogOriginSdConfig.EnterpriseId`, carried as supplied and escaped per §6.3.3. The library registers no enterprise number of its own and validates none: which number is yours, and that it is registered, is the integrator's |
| [7.2.3](https://www.rfc-editor.org/rfc/rfc5424.html#section-7.2.3) | software — MUST NOT be longer than 48 characters | Supported | The limit is stated on `SolidSyslogOriginSdConfig.Software`, which is carried and escaped as supplied rather than truncated. It is a fixed configuration string, so an over-long value is visible on the first run — [#748](https://github.com/cososo-ltd/solid-syslog/issues/748) tracks enforcing it |
| [7.2.4](https://www.rfc-editor.org/rfc/rfc5424.html#section-7.2.4) | swVersion — MUST NOT be longer than 32 characters | Supported | Stated on `SolidSyslogOriginSdConfig.SwVersion`, on the same terms as `software` above |
| [7.2.5](https://www.rfc-editor.org/rfc/rfc5424.html#section-7.2.5) | origin example | N/A | Illustrative. States no requirement of its own |
| [7.3](https://www.rfc-editor.org/rfc/rfc5424.html#section-7.3) | meta SD — sequenceId, sysUpTime, language | Supported | `SolidSyslogMetaSd` covers all three IANA-registered parameters. `sequenceId` (§7.3.1) sourced via an injected `SolidSyslogAtomicCounter`. `sysUpTime` (§7.3.2 / RFC 3418 `TimeTicks`) sourced via a `SolidSyslogSysUpTimeFunction` callback returning `uint32_t` hundredths, the type giving RFC 3418's natural wrap; the [capability matrix](platforms/index.md) shows which platforms supply one. `language` (§7.3.3 / BCP 47) sourced via a `SolidSyslogSdValueFunction` callback streaming into a `SolidSyslogSdValue`, which applies SD-PARAM-VALUE escaping per §6.3.3. `sysUpTime` and `language` are independently optional — a NULL field in `SolidSyslogMetaSdConfig` omits that parameter. The counter is not: `SolidSyslogMetaSd_Create` rejects a NULL `Counter` with a `WARNING` and returns the Null structured data, so the element is not emitted at all |
| [7.3.1](https://www.rfc-editor.org/rfc/rfc5424.html#section-7.3.1) | meta SD — sequenceId wraps at 2147483647 to 1 | Supported | `SolidSyslogAtomicCounter` wraps via CAS-loop in [1, 2³¹ - 1]; never returns 0; never above max. [AtomicCounter](api/structSolidSyslogAtomicCounter.md) is a vtable abstraction, so the wrap is the contract's and not any one implementation's; the integrator wires a concrete counter at setup time and the [capability matrix](platforms/index.md) shows which platforms supply one. sequenceId is assigned at the point of message raise (application-layer originator), preserving end-to-end loss-detection across the internal buffer / store-and-forward / transport pipeline. Trade-off: under concurrent raise from multiple threads, a small reorder window may occur in transmitted IDs (adjacent IDs may invert, since buffer/transport scheduling between raise and wire is not under library control). IDs from a wired counter remain unique and non-zero — SIEMs performing gap detection identify message loss correctly; SIEMs requiring strict monotonic ordering should sort by timestamp. Uniqueness is the counter's, not the contract's: exhaust a counter's pool and `Create` falls back to the Null counter, which returns 1 for every record, so gap detection stops being meaningful while logging continues |
| [7.3.2](https://www.rfc-editor.org/rfc/rfc5424.html#section-7.3.2) | sysUpTime — MUST be a decimal integer, digits only | Supported | The callback returns `uint32_t` hundredths and the value is written through `SolidSyslogSdValue_Uint32`, which emits decimal digits only. §7.3.2 also notes the SNMP management portion may differ from the syslog one, which is the integrator's to reconcile |
| [7.3.3](https://www.rfc-editor.org/rfc/rfc5424.html#section-7.3.3) | language — MUST be a BCP 47 language identifier | Supported | Streamed through a `SolidSyslogSdValueFunction` into a `SolidSyslogSdValue`, which applies §6.3.3 escaping. The parameter is optional and the identifier is the integrator's to supply; the library does not parse BCP 47 |
| [8.1](https://www.rfc-editor.org/rfc/rfc5424.html#section-8.1) | UNICODE — shortest-form encoding REQUIRED | Supported | RFC 3629 validation at the formatter primitives rejects overlong encodings, substituting each ill-formed byte with U+FFFD per Unicode §3.9, so a non-shortest-form sequence cannot pass through |
| [8.6](https://www.rfc-editor.org/rfc/rfc5424.html#section-8.6) | Congestion control — TLS REQUIRED to implement, UDP for managed networks | Supported | The implementation obligation is the same one §5.1 states, and is met. Which transport a deployment uses, and whether its network is provisioned for UDP syslog, is the integrator's choice |

## RFC 5425 — TLS Transport Mapping for Syslog

Checked against [RFC 5425](https://www.rfc-editor.org/rfc/rfc5425.html), Standards
Track, and [RFC 9662](https://www.rfc-editor.org/rfc/rfc9662.html), Standards Track,
which updates it.

TLS is a [Stream](api/structSolidSyslogStream.md) wrapped around another Stream,
so these requirements are met by whichever TLS stream the integrator wires; the
[capability matrix](platforms/index.md) shows which platforms supply one. The
statuses below are against [the TLS contract](tls.md), which states what any TLS
stream must do. Where a shipped platform does not yet meet an obligation, its own
page records the exception and links the issue tracking it.

**RFC 5425 is read together with RFC 9662**, *Updates to the Cipher Suites in
Secure Syslog*. It replaces the 2009 cipher-suite requirement, asks that TLS 1.3
be supported and preferred where it is implemented, and normatively references
BCP 195 for how TLS should be used. Rows below cite it where it is the
requirement in force, rather than tabulating it separately.

| Section | Requirement | Status | Notes |
|---|---|---|---|
| [3](https://www.rfc-editor.org/rfc/rfc5425.html#section-3) | TLS to secure syslog | Supported | A TLS `Stream` wraps a byte-transport `Stream` — a TCP one from any platform, or a caller-supplied one. §3's own caveat holds here too: the protection is hop-by-hop, so a relay that terminates the connection is authenticated in place of the originating device |
| [4.1](https://www.rfc-editor.org/rfc/rfc5425.html#section-4.1) | Default port 6514 | Supported | `SOLIDSYSLOG_TLS_DEFAULT_PORT` constant in `SolidSyslogTransport.h`, alongside the UDP and TCP defaults. Caller-supplied via the endpoint callback so multi-port deployments can override |
| [4.2](https://www.rfc-editor.org/rfc/rfc5425.html#section-4.2) | TLS 1.2 as the mandatory-to-implement protocol | Supported | The contract pins the protocol floor at TLS 1.2 in the stream rather than inheriting the TLS library's defaults, so a permissive build cannot negotiate below it. RFC 9662 keeps 1.2 as mandatory-to-implement |
| [RFC 9662 §4](https://www.rfc-editor.org/rfc/rfc9662.html#section-4) | Cipher suites — `TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256` SHOULD be offered, `TLS_RSA_WITH_AES_128_CBC_SHA` MAY be | N/A | Which cipher suites exist is a property of the TLS library linked on the target, not of this library, which neither adds nor removes any. RFC 9662 downgraded the 2009 mandatory suite because it offers no forward secrecy, which is the same reason a hardened build disables it. RFC 9662 §4 is internally awkward — it calls both suites REQUIRED and then states the offer preference above — so it is cited whole rather than paraphrased into something tidier |
| [RFC 9662 §4](https://www.rfc-editor.org/rfc/rfc9662.html#section-4) | TLS 1.3 SHOULD be supported, and MUST be preferred where implemented | Supported | The contract sets a floor and deliberately no ceiling, so the later version is negotiated wherever the peer offers one. This is why no ceiling is set: pinning one to constrain cipher selection would breach the preference requirement |
| [RFC 9662 §6](https://www.rfc-editor.org/rfc/rfc9662.html#section-6) | Early data (0-RTT) MUST NOT be used | Supported | RFC 9662 forbids it because syslog has no replay protection and early data has none between connections. Early data requires a pre-shared key, from an external source or a previous handshake; no shipped TLS stream enables early data, session tickets or PSK, so no connection can carry it. A caller-supplied stream is the caller's to hold to the same rule |
| [4.2.1](https://www.rfc-editor.org/rfc/rfc5425.html#section-4.2.1) | Certificate-based authentication — server | Supported | Peer verification is required, not optional: the certificate must chain to the trust anchors the caller supplies, and the peer identity the caller declares is checked against it |
| [4.2.1](https://www.rfc-editor.org/rfc/rfc5425.html#section-4.2.1) | Certificate-based authentication — client | Supported | A client certificate and its key are optional configuration on the TLS stream, presented only when both are given, and a partially configured pair is reported rather than silently ignored |
| [4.2.1](https://www.rfc-editor.org/rfc/rfc5425.html#section-4.2.1) | Means to generate a key pair and self-signed certificate | N/A | Deliberately excluded. The library consumes trust material and does not mint it, so key generation belongs to the deployment's provisioning. Directed at a syslog application rather than at a component one is built from |
| [4.2.3](https://www.rfc-editor.org/rfc/rfc5425.html#section-4.2.3) | Administrators may select the cryptographic level | Partial | The contract requires an integrator's cipher policy to be passed through where the underlying library allows one to be selected. Neither shipped TLS platform delivers that on the connection actually negotiated — see each platform's page, and `#733` |
| [4.3.1](https://www.rfc-editor.org/rfc/rfc5425.html#section-4.3.1) | Octet-counting framing, and the message length | Supported | Reuses `SolidSyslogStreamSender`, so the frame is `MSG-LEN SP MSG`. `SOLIDSYSLOG_MAX_MESSAGE_SIZE` defaults to 2048, the length §4.3.1 requires every transport receiver to accept |
| [4.4](https://www.rfc-editor.org/rfc/rfc5425.html#section-4.4) | `close_notify` before closing | Supported | Close sends `close_notify` before tearing the connection down |

## RFC 5426 — Transmission of Syslog Messages over UDP

Checked against [RFC 5426](https://www.rfc-editor.org/rfc/rfc5426.html), Standards Track.

| Section | Requirement | Status | Notes |
|---|---|---|---|
| [3.1](https://www.rfc-editor.org/rfc/rfc5426.html#section-3.1) | One message per UDP datagram | Supported | `SolidSyslogUdpSender` sends one datagram per `Send` call |
| [3.2](https://www.rfc-editor.org/rfc/rfc5426.html#section-3.2) | Message fits in single datagram | Supported | Bounded by `SOLIDSYSLOG_MAX_MESSAGE_SIZE` |
| [3.2](https://www.rfc-editor.org/rfc/rfc5426.html#section-3.2) | Avoid IP fragmentation (respect MTU) | Supported | The [Datagram](api/structSolidSyslogDatagram.md) contract carries this: an oversize payload is rejected rather than fragmented, and reported distinctly from a hard failure. `SolidSyslogUdpSender` responds by asking the Datagram for the path's largest payload and resending a UTF-8-safe trimmed datagram via `SolidSyslogUdpPayload_TrimToCodepointBoundary`, falling back to `SOLIDSYSLOG_UDP_IPV6_SAFE_PAYLOAD = 1232` (RFC 8200 §5) where no path MTU is available. That fallback is above the 1180 octets §3.2 calls the safest assumption for IPv6 when the MTU is not known in advance, and well above the 480 it gives for IPv4 — the advice is prose rather than an RFC 2119 requirement, and the status here is against the RECOMMENDED restriction it accompanies. How a platform discovers the MTU is on its own page |
| [3.3](https://www.rfc-editor.org/rfc/rfc5426.html#section-3.3) | Default port 514 | Supported | `SOLIDSYSLOG_UDP_DEFAULT_PORT` = 514 |
| [4.1](https://www.rfc-editor.org/rfc/rfc5426.html#section-4.1) | Unreliable delivery — no confirmation | N/A | Inherent in UDP: §4.1 states the transport mapping provides no mechanism to detect or correct datagram loss. §5.4 restates it as a security consequence. Caller should be aware |
| [5](https://www.rfc-editor.org/rfc/rfc5426.html#section-5) | No authentication/integrity/confidentiality | N/A | Use TLS transport for security. §5.1 covers sender authentication and message forgery, §5.2 confidentiality |

## RFC 6587 — Transmission of Syslog Messages over TCP

Checked against [RFC 6587](https://www.rfc-editor.org/rfc/rfc6587.html), Historic.

| Section | Requirement | Status | Notes |
|---|---|---|---|
| [3.3](https://www.rfc-editor.org/rfc/rfc6587.html#section-3.3) | Sender initiates TCP connection | Supported | `SolidSyslogStreamSender` connects lazily on first send |
| [3.4.1](https://www.rfc-editor.org/rfc/rfc6587.html#section-3.4.1) | Octet counting framing | Supported | `MSG-LEN SP MSG` prefix on every send |
| [3.4.2](https://www.rfc-editor.org/rfc/rfc6587.html#section-3.4.2) | Non-transparent framing (LF trailer) | N/A | Deliberately not implemented. RFC 6587 describes both framings without recommending either, but §3.4 records that non-transparent framing has known problems and that octet counting does not; §3.4.1 is also the framing RFC 5425 mandates for TLS, so the library ships that alone rather than a mode selector |
| [3.5](https://www.rfc-editor.org/rfc/rfc6587.html#section-3.5) | Session closure handling | Supported | On send failure the stream is closed; the next Send transparently reconnects |
| [3.5](https://www.rfc-editor.org/rfc/rfc6587.html#section-3.5) | Handle receiver-initiated close | Supported | Detected via send failure path — same reconnect-on-next-Send mechanism |
| — | Default port 601 | Supported | RFC 6587 standardises no port: §3.3 records that the protocol "has no standardized port assignment", and §4 that operators must select one per deployment. `SOLIDSYSLOG_TCP_DEFAULT_PORT = 601` (defined in `Core/Interface/SolidSyslogTransport.h`) is the IANA `syslog-conn` assignment from RFC 3195, and is caller-overridable via the endpoint callback |
| — | Address rotation without app restart | Supported | A library capability rather than an RFC 6587 requirement. App bumps `endpointVersion`; sender Disconnects and reconnects on next Send |
| — | Partial write handling (send returns short) | Supported | The [Stream](api/structSolidSyslogStream.md) contract makes `Send` all-or-nothing: a short write is a failure, never a partial success, so the stream closes itself, the sender reconnects on its next pass, and store-and-forward replays the message on the fresh connection. The same contract keeps steady-state `Send` and `Read` non-blocking and bounds `Open`, so a wedged peer or a full send buffer cannot stall the servicing pass. The connect bound is `SOLIDSYSLOG_TCP_CONNECT_TIMEOUT_MS` (default 200 ms), overridable at runtime through the per-Stream `GetConnectTimeoutMs(ConnectTimeoutContext)` accessor. How a transport detects a long-term wedge, and what it does about one, is on its own page |

## Summary

| RFC | Total requirements | Supported | Partial | N/A |
|---|---|---|---|---|
| RFC 5424 | 40 | 33 | 0 | 7 |
| RFC 5425 | 12 | 9 | 1 | 2 |
| RFC 5426 | 6 | 4 | 0 | 2 |
| RFC 6587 | 8 | 7 | 0 | 1 |
