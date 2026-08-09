# RFC Compliance Matrix

SolidSyslog implements the sender (client) side of four syslog RFCs. This
document tracks which requirements are currently met, partially met, or
planned.

Status key:

- Supported: implemented and tested
- Partial: implemented with known limitations
- Planned: tracked in an issue or epic
- N/A: not applicable to a sender implementation, or applicable and deliberately
  excluded — the note says which, and why

## RFC 5424 — The Syslog Protocol

| Section | Requirement | Status | Notes |
|---|---|---|---|
| 6.2.1 | PRI — facility * 8 + severity | Supported | Invalid values fall back to `syslog.err` (facility 5, severity 3) |
| 6.2.2 | VERSION = 1 | Supported | |
| 6.2.3 | TIMESTAMP — ISO 8601 with microseconds | Supported | 6-digit fractional seconds, UTC offset or `Z` |
| 6.2.3 | TIMESTAMP — NILVALUE when clock unavailable | Supported | NilClock produces `-` |
| 6.2.4 | HOSTNAME — max 255 chars, PRINTUSASCII | Supported | Truncated to 255. Non-PRINTUSASCII bytes substituted with `?`. Written through the public `SolidSyslogHeaderField` sink (`SolidSyslogHeaderField_PrintUsAscii`); the underlying formatter is library-private |
| 6.2.5 | APP-NAME — max 48 chars, PRINTUSASCII | Supported | Truncated to 48. Non-PRINTUSASCII bytes substituted with `?` |
| 6.2.6 | PROCID — max 128 chars, PRINTUSASCII | Supported | Truncated to 128. Non-PRINTUSASCII bytes substituted with `?` |
| 6.2.7 | MSGID — max 32 chars, PRINTUSASCII | Supported | Truncated to 32. Non-PRINTUSASCII bytes substituted with `?` |
| 6.3 | STRUCTURED-DATA — SD-ELEMENTs or NILVALUE | Supported | Extensible via `SolidSyslogStructuredData` vtable |
| 6.3.2 | SD-ID / SD-NAME syntax validation | Planned | Not performed. It only bites once callers can supply their own names: the three standard SDs (meta / timeQuality / origin) use compile-time-constant names that are valid by construction. Tracked under Custom Structured Data (`#64`), which is what introduces caller-supplied SD-IDs and PARAM names |
| 6.3.3 | SD-PARAM value escaping (`]`, `\`, `"`) | Supported | `SolidSyslogSdValue` — every SD-PARAM value is written through this sink, which applies the escaping: RFC 3629 UTF-8 validated, ill-formed input substituted per-byte with U+FFFD (Unicode §3.9). `OriginSd` streams software, swVersion, enterpriseId, and each ip into it; `MetaSd` streams language via the integrator's `SolidSyslogSdValueFunction` callback. Both get the same escaping. |
| 7.1 | timeQuality SD — tzKnown, isSynced, syncAccuracy | Supported | `SolidSyslogTimeQualitySd` |
| 7.2 | origin SD — software, swVersion, enterpriseId, ip | Supported | `SolidSyslogOriginSd` covers all four §7.2 parameters. `software`, `swVersion`, and `enterpriseId` are static strings supplied via `SolidSyslogOriginSdConfig`; the config strings are borrowed for the SD's lifetime and each is escaped per §6.3.3 by the `SolidSyslogSdValue` writer it is streamed into at Format time (no pre-formatted scratch storage). `ip` is repeatable per RFC 5424 §7.2 and sourced via two callbacks (`SolidSyslogOriginIpCountFunction`, `SolidSyslogOriginIpAtFunction`) so multi-homed hosts can reflect runtime address changes; the library asks for a count then loops 0..N-1, opening an `ip` param per token (with a leading space) while the integrator's at-callback writes one IP value per call into the `SolidSyslogSdValue` it is handed, which applies the escaping. All four parameters are independently optional — a NULL field or NULL callback omits the corresponding parameter from the SD-ELEMENT. The library frames and escapes; the IP value length is the integrator's to bound (ultimately by `SOLIDSYSLOG_MAX_MESSAGE_SIZE`), as is the IP count. Bare `[origin]` with no parameters is RFC-legal (§7.2 marks all params OPTIONAL, no SHOULD enforcement) and is what the library emits when the integrator wires nothing |
| 7.3 | meta SD — sequenceId, sysUpTime, language | Supported | `SolidSyslogMetaSd` covers all three IANA-registered parameters. `sequenceId` (§7.3.1) sourced via an injected `SolidSyslogAtomicCounter`. `sysUpTime` (§7.3.2 / RFC 3418 `TimeTicks`) sourced via a `SolidSyslogSysUpTimeFunction` callback returning `uint32_t` hundredths, the type giving RFC 3418's natural wrap; the [capability matrix](platforms/index.md) shows which platforms supply one. `language` (§7.3.3 / BCP 47) sourced via a `SolidSyslogSdValueFunction` callback streaming into a `SolidSyslogSdValue`, which applies SD-PARAM-VALUE escaping per §6.3.3. `sysUpTime` and `language` are independently optional — a NULL field in `SolidSyslogMetaSdConfig` omits that parameter. The counter is not: `SolidSyslogMetaSd_Create` rejects a NULL `Counter` with a `WARNING` and returns the Null structured data, so the element is not emitted at all |
| 7.3.1 | meta SD — sequenceId wraps at 2147483647 to 1 | Partial | `SolidSyslogAtomicCounter` wraps via CAS-loop in [1, 2³¹ - 1]; never returns 0; never above max. [AtomicCounter](api/structSolidSyslogAtomicCounter.md) is a vtable abstraction, so the wrap is the contract's and not any one implementation's; the integrator wires a concrete counter at setup time and the [capability matrix](platforms/index.md) shows which platforms supply one. sequenceId is assigned at the point of message raise (application-layer originator), preserving end-to-end loss-detection across the internal buffer / store-and-forward / transport pipeline. Trade-off: under concurrent raise from multiple threads, a small reorder window may occur in transmitted IDs (adjacent IDs may invert, since buffer/transport scheduling between raise and wire is not under library control). IDs from a wired counter remain unique and non-zero — SIEMs performing gap detection identify message loss correctly; SIEMs requiring strict monotonic ordering should sort by timestamp. Uniqueness is the counter's, not the contract's: exhaust a counter's pool and `Create` falls back to the Null counter, which returns 1 for every record, so gap detection stops being meaningful while logging continues |
| 6.4 | MSG — UTF-8 preferred | Supported | RFC 3629 UTF-8 validated at the formatter primitives (`SolidSyslogFormatter_BoundedString`), with ill-formed input substituted per-byte with U+FFFD (Unicode §3.9). MSG is prefixed with the §6.4 UTF-8 BOM (`%xEF.BB.BF`) unconditionally; if the caller's body already begins with a BOM it is stripped so the wire frame contains exactly one. Truncation preserves codepoint boundaries at both layers: the formatter clips at `SOLIDSYSLOG_MAX_MESSAGE_SIZE` without splitting a codepoint, and on UDP the sender walks back over any partial codepoint when the kernel reports `EMSGSIZE` for the path MTU. TCP/TLS streams fragment transparently at the transport layer and so do not need a path-MTU trim |
| 6.1 | Message size — max 2048 recommended | Supported | Default `SOLIDSYSLOG_MAX_MESSAGE_SIZE` = 2048, matching the largest message §6.1 says a transport receiver SHOULD accept; override it for memory-constrained MCUs via the standard tunable mechanism |
| 6 | PRINTUSASCII in header fields (codes 33-126) | Supported | Non-compliant bytes substituted with `?` at format time (HOSTNAME, APP-NAME, PROCID, MSGID) |

## RFC 5426 — Transmission of Syslog Messages over UDP

| Section | Requirement | Status | Notes |
|---|---|---|---|
| 3.1 | One message per UDP datagram | Supported | `SolidSyslogUdpSender` sends one datagram per `Send` call |
| 3.2 | Default port 514 | Supported | `SOLIDSYSLOG_UDP_DEFAULT_PORT` = 514 |
| 3.2 | Message fits in single datagram | Supported | Bounded by `SOLIDSYSLOG_MAX_MESSAGE_SIZE` |
| 3.2 | Avoid IP fragmentation (respect MTU) | Supported | The [Datagram](api/structSolidSyslogDatagram.md) contract carries this: an oversize payload is rejected rather than fragmented, and reported distinctly from a hard failure. `SolidSyslogUdpSender` responds by asking the Datagram for the path's largest payload and resending a UTF-8-safe trimmed datagram via `SolidSyslogUdpPayload_TrimToCodepointBoundary`, falling back to `SOLIDSYSLOG_UDP_IPV6_SAFE_PAYLOAD = 1232` (RFC 8200 §5) where no path MTU is available. How a platform discovers the MTU is on its own page |
| 3.3 | Unreliable delivery — no confirmation | N/A | Inherent in UDP. Caller should be aware |
| 4 | No authentication/integrity/confidentiality | N/A | Use TLS transport for security |

## RFC 6587 — Transmission of Syslog Messages over TCP

| Section | Requirement | Status | Notes |
|---|---|---|---|
| 3.2 | Sender initiates TCP connection | Supported | `SolidSyslogStreamSender` connects lazily on first send |
| 3.2 | Default port 601 | Supported | `SOLIDSYSLOG_TCP_DEFAULT_PORT = 601` per IANA assignment (defined in `Core/Interface/SolidSyslogTransport.h`) |
| 3.4.1 | Octet counting framing | Supported | `MSG-LEN SP MSG` prefix on every send |
| 3.4.2 | Non-transparent framing (LF trailer) | N/A | Deliberately not implemented. RFC 6587 is Historic and describes both framings without recommending either, but §3.4 records that non-transparent framing has known problems and that octet counting does not; §3.4.1 is also the framing RFC 5425 mandates for TLS, so the library ships that alone rather than a mode selector |
| 3.5 | Session closure handling | Supported | On send failure the stream is closed; the next Send transparently reconnects |
| 3.5 | Handle receiver-initiated close | Supported | Detected via send failure path — same reconnect-on-next-Send mechanism |
| 3.5 | Address rotation without app restart | Supported | App bumps `endpointVersion`; sender Disconnects and reconnects on next Send |
| — | Partial write handling (send returns short) | Supported | The [Stream](api/structSolidSyslogStream.md) contract makes `Send` all-or-nothing: a short write is a failure, never a partial success, so the stream closes itself, the sender reconnects on its next pass, and store-and-forward replays the message on the fresh connection. The same contract keeps steady-state `Send` and `Read` non-blocking and bounds `Open`, so a wedged peer or a full send buffer cannot stall the servicing pass. The connect bound is `SOLIDSYSLOG_TCP_CONNECT_TIMEOUT_MS` (default 200 ms), overridable at runtime through the per-Stream `GetConnectTimeoutMs(ConnectTimeoutContext)` accessor. How a transport detects a long-term wedge, and what it does about one, is on its own page |

## RFC 5425 — TLS Transport Mapping for Syslog

TLS is a [Stream](api/structSolidSyslogStream.md) wrapped around another Stream,
so these requirements are met by whichever TLS stream the integrator wires; the
[capability matrix](platforms/index.md) shows which platforms supply one. What
an adapter validates, what it leaves to you, and how credentials reach it are
stated on that platform's own page.

| Section | Requirement | Status | Notes |
|---|---|---|---|
| 4.1 | TLS over TCP | Supported | A TLS `Stream` wraps a byte-transport `Stream` — a TCP one from any platform, or a caller-supplied one |
| 4.2 | Default port 6514 | Supported | `SOLIDSYSLOG_TLS_DEFAULT_PORT` constant in `SolidSyslogTransport.h`, alongside the UDP and TCP defaults. Caller-supplied via the endpoint callback so multi-port deployments can override |
| 5.1 | Server certificate validation | Supported | Peer verification is required, not optional: the certificate must chain to the trust anchors the caller supplies, and the server identity is checked against it. What an adapter does when no identity is given — and whether it says so — is on its page |
| 5.2 | Mutual TLS (client certificate) | Supported | A client certificate and its key are optional config on the TLS stream, and are presented only when both are given. Whether an adapter validates the pair locally, and what a half-supplied credential does, is on its page |
| 5.3 | TLS 1.2+ cipher suites | Supported | The floor is pinned to TLS 1.2 by the adapter rather than inherited from the TLS library's defaults, so a permissive build cannot negotiate below it. Cipher selection within that floor is the integrator's |
| 5.4 | Octet counting framing (mandatory for TLS) | Supported | Reuses `SolidSyslogStreamSender` — RFC 6587 framing is identical |
| 5.5 | TLS close_notify handling | Supported | Close sends `close_notify` before tearing the connection down |

## Summary

| RFC | Total requirements | Supported | Partial | Planned | N/A |
|---|---|---|---|---|---|
| RFC 5424 | 18 | 16 | 1 | 1 | 0 |
| RFC 5426 | 6 | 4 | 0 | 0 | 2 |
| RFC 6587 | 8 | 7 | 0 | 0 | 1 |
| RFC 5425 | 7 | 7 | 0 | 0 | 0 |
