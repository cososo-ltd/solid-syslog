# RFC Compliance Matrix

SolidSyslog implements the sender (client) side of four syslog RFCs. This
document tracks which requirements are currently met, partially met, or
planned.

Status key:
- Supported — implemented and tested
- Partial — implemented with known limitations
- Planned — tracked in an issue or epic
- N/A — not applicable to a sender implementation

## RFC 5424 — The Syslog Protocol

| Section | Requirement | Status | Notes |
|---|---|---|---|
| 6.1 | PRI — facility * 8 + severity | Supported | Invalid values fall back to `syslog.err` (facility 5, severity 3) |
| 6.2.1 | VERSION = 1 | Supported | |
| 6.2.2 | TIMESTAMP — ISO 8601 with microseconds | Supported | 6-digit fractional seconds, UTC offset or `Z` |
| 6.2.2 | TIMESTAMP — NILVALUE when clock unavailable | Supported | NilClock produces `-` |
| 6.2.3 | HOSTNAME — max 255 chars, PRINTUSASCII | Supported | Truncated to 255. Non-PRINTUSASCII bytes substituted with `?` via `SolidSyslogFormatter_PrintUsAsciiString` |
| 6.2.4 | APP-NAME — max 48 chars, PRINTUSASCII | Supported | Truncated to 48. Non-PRINTUSASCII bytes substituted with `?` |
| 6.2.5 | PROCID — max 128 chars, PRINTUSASCII | Supported | Truncated to 128. Non-PRINTUSASCII bytes substituted with `?` |
| 6.2.6 | MSGID — max 32 chars, PRINTUSASCII | Supported | Truncated to 32. Non-PRINTUSASCII bytes substituted with `?` |
| 6.3 | STRUCTURED-DATA — SD-ELEMENTs or NILVALUE | Supported | Extensible via `SolidSyslogStructuredData` vtable |
| 6.3.3 | SD-PARAM value escaping (`]`, `\`, `"`) | Supported | `SolidSyslogFormatter_EscapedString` — RFC 3629 UTF-8 validated, ill-formed input substituted per-byte with U+FFFD (Unicode §3.9); `OriginSd` escapes software, swVersion, enterpriseId, and each ip via this primitive; `MetaSd` escapes language via the integrator's `SolidSyslogStringFunction` callback that wraps the same primitive. SD-NAME / SD-ID syntax validation only matters once callers can supply names — landed under [E14](https://github.com/DavidCozens/solid-syslog/issues/64) (Custom Structured Data); the standard SDs (meta / timeQuality / origin) use compile-time-constant names |
| 6.3.3 | timeQuality SD — tzKnown, isSynced, syncAccuracy | Supported | `SolidSyslogTimeQualitySd` |
| 6.3.4, 7.2 | origin SD — software, swVersion, enterpriseId, ip | Supported | `SolidSyslogOriginSd` covers all four §7.2 parameters. `software`, `swVersion`, and `enterpriseId` are static strings supplied via `SolidSyslogOriginSdConfig`; each is escaped per §6.3.3 via `SolidSyslogFormatter_EscapedString` and pre-formatted at Create time into an internal storage buffer. `ip` is repeatable per RFC 5424 §7.2 and sourced via two callbacks (`SolidSyslogOriginIpCountFunction`, `SolidSyslogOriginIpAtFunction`) so multi-homed hosts can reflect runtime address changes; the library asks for a count then loops 0..N-1 framing each `ip="…"` token (with a leading space) while the integrator's at-callback writes one escaped IP value per call (this avoids a formatter rewind primitive). All four parameters are independently optional — a NULL field or NULL callback omits the corresponding parameter from the SD-ELEMENT. Per-IP value capped at 64 chars via `_EscapedString`; no library-side cap on the IP count (bounded by `SOLIDSYSLOG_MAX_MESSAGE_SIZE`). Bare `[origin]` with no parameters is RFC-legal (§7.2 marks all params OPTIONAL, no SHOULD enforcement) and is what the library emits when the integrator wires nothing |
| 6.3.5, 7.3 | meta SD — sequenceId, sysUpTime, language | Supported | `SolidSyslogMetaSd` covers all three IANA-registered parameters. `sequenceId` (§7.3.1) sourced via an injected `SolidSyslogAtomicCounter`. `sysUpTime` (§7.3.2 / RFC 3418 `TimeTicks`) sourced via a `SolidSyslogSysUpTimeFunction` callback returning `uint32_t` hundredths; reference platform integrations are `SolidSyslogPosixSysUpTime` (`clock_gettime(CLOCK_BOOTTIME)`) and `SolidSyslogWindowsSysUpTime` (`GetTickCount64`), with the cast to `uint32_t` providing RFC 3418's natural wrap. `language` (§7.3.3 / BCP 47) sourced via a `SolidSyslogStringFunction` callback that writes through `SolidSyslogFormatter_EscapedString` to satisfy SD-PARAM-VALUE escaping per §6.3.3. All three independently optional — a NULL field in `SolidSyslogMetaSdConfig` omits that parameter from the SD-ELEMENT |
| 6.3.5, 7.3.1 | meta SD — sequenceId wraps at 2147483647 to 1 | Supported | `SolidSyslogAtomicCounter` wraps via CAS-loop in [1, 2³¹ - 1]; never returns 0; never above max. AtomicCounter is a vtable abstraction — concrete impls are `SolidSyslogStdAtomicCounter` (C11 `<stdatomic.h>` + `atomic_compare_exchange_strong_explicit`) on POSIX/clang/gcc/modern MSVC, and `SolidSyslogWindowsAtomicCounter` (`volatile LONG` + `InterlockedCompareExchange`) on legacy MSVC. The integrator picks one at setup time by calling the relevant platform's `_Create`; CMake's `HAVE_STDATOMIC_H` / `HAVE_WINDOWS_INTERLOCKED` checks gate which platform sources are compiled. sequenceId is assigned at the point of message raise (application-layer originator), preserving end-to-end loss-detection across the internal buffer / store-and-forward / transport pipeline. Trade-off: under concurrent raise from multiple threads, a small reorder window may occur in transmitted IDs (adjacent IDs may invert, since buffer/transport scheduling between raise and wire is not under library control). All IDs remain unique and non-zero — SIEMs performing gap detection identify message loss correctly; SIEMs requiring strict monotonic ordering should sort by timestamp |
| 6.4 | MSG — UTF-8 preferred | Supported | RFC 3629 UTF-8 validated at the formatter primitives (`SolidSyslogFormatter_BoundedString`), with ill-formed input substituted per-byte with U+FFFD (Unicode §3.9). MSG is prefixed with the §6.4 UTF-8 BOM (`%xEF.BB.BF`) unconditionally; if the caller's body already begins with a BOM it is stripped so the wire frame contains exactly one ([S12.13](https://github.com/DavidCozens/solid-syslog/issues/219)). Truncation preserves codepoint boundaries at both layers: the formatter clips at `SOLIDSYSLOG_MAX_MESSAGE_SIZE` without splitting a codepoint ([S12.10](https://github.com/DavidCozens/solid-syslog/issues/121)), and on UDP the sender walks back over any partial codepoint when the kernel reports `EMSGSIZE` for the path MTU ([S12.12](https://github.com/DavidCozens/solid-syslog/issues/210)). TCP/TLS streams fragment transparently at the transport layer and so do not need a path-MTU trim |
| 8.1 | Message size — max 2048 recommended | Supported | Default `SOLIDSYSLOG_MAX_MESSAGE_SIZE` = 2048, matching the §8.1 SHOULD value. Per-target override via a CMake variable is planned in [E21 #217](https://github.com/DavidCozens/solid-syslog/issues/217) for memory-constrained MCUs |
| 9 | PRINTUSASCII in header fields (codes 33-126) | Supported | Non-compliant bytes substituted with `?` at format time (HOSTNAME, APP-NAME, PROCID, MSGID) |

## RFC 5426 — Transmission of Syslog Messages over UDP

| Section | Requirement | Status | Notes |
|---|---|---|---|
| 3.1 | One message per UDP datagram | Supported | `SolidSyslogUdpSender` sends one datagram per `Send` call |
| 3.2 | Default port 514 | Supported | `SOLIDSYSLOG_UDP_DEFAULT_PORT` = 514 |
| 3.2 | Message fits in single datagram | Supported | Bounded by `SOLIDSYSLOG_MAX_MESSAGE_SIZE` |
| 3.2 | Avoid IP fragmentation (respect MTU) | Supported | `SolidSyslogUdpSender` lazily connects on first send, enables Linux `IP_MTU_DISCOVER` / Windows equivalent with `IP_PMTUDISC_DO` so the kernel returns `EMSGSIZE` (Winsock `WSAEMSGSIZE`) for path-MTU oversize, queries the path MTU via `getsockopt(IP_MTU)`, and resends a UTF-8-safe trimmed datagram via `SolidSyslogUdpPayload_TrimToCodepointBoundary`. Falls back to `SOLIDSYSLOG_UDP_IPV6_SAFE_PAYLOAD = 1232` (RFC 8200 §5) when the MTU lookup fails ([S12.12](https://github.com/DavidCozens/solid-syslog/issues/210)) |
| 3.3 | Unreliable delivery — no confirmation | N/A | Inherent in UDP. Caller should be aware |
| 4 | No authentication/integrity/confidentiality | N/A | Use TLS transport for security — [E3](https://github.com/DavidCozens/solid-syslog/issues/5) |

## RFC 6587 — Transmission of Syslog Messages over TCP

| Section | Requirement | Status | Notes |
|---|---|---|---|
| 3.2 | Sender initiates TCP connection | Supported | `SolidSyslogStreamSender` connects lazily on first send (S03.04) |
| 3.2 | Default port 601 | Supported | `SOLIDSYSLOG_TCP_DEFAULT_PORT = 601` per IANA assignment (defined in `Core/Interface/SolidSyslogTransport.h`) |
| 3.4.1 | Octet counting framing | Supported | `MSG-LEN SP MSG` prefix on every send |
| 3.4.2 | Non-transparent framing (LF trailer) | N/A | RFC 6587 octet counting (§3.4.1) is the recommended method and is what the library ships; non-transparent framing is the legacy alternative |
| 3.5 | Session closure handling | Supported | On send failure the stream is closed; the next Send transparently reconnects (S03.04) |
| 3.5 | Handle receiver-initiated close | Supported | Detected via send failure path — same reconnect-on-next-Send mechanism (S03.04) |
| 3.5 | Address rotation without app restart | Supported | App bumps `endpointVersion`; sender Disconnects and reconnects on next Send (S03.04) |
| — | Partial write handling (send returns short) | Supported | A short return from `send()` is treated as failure: `Send` returns false, the caller closes and reconnects on the next attempt, store-and-forward replays the message on the fresh socket. `SO_SNDTIMEO` is set at socket open (5 s, hard-coded — see `Platform/Posix/Source/SolidSyslogPosixTcpStream.c` and `Platform/Windows/Source/SolidSyslogWinsockTcpStream.c`) so a wedged peer can't make a single `SolidSyslog_Service` call hang indefinitely. On POSIX, `EINTR` is the only retried errno (portability shim for kernels without `SA_RESTART`); `EAGAIN`/`EWOULDBLOCK` (timeout) and any other error propagate as failure. Winsock has no signal-interruption semantics on `send()`, so the EINTR retry path is omitted; `WSAETIMEDOUT` (timeout) and any other error propagate as failure identically. |

## RFC 5425 — TLS Transport Mapping for Syslog

| Section | Requirement | Status | Notes |
|---|---|---|---|
| 4.1 | TLS over TCP | Supported | `SolidSyslogTlsStream` wraps a TCP `Stream` (typically `SolidSyslogPosixTcpStream`) |
| 4.2 | Default port 6514 | Supported | `SOLIDSYSLOG_TLS_DEFAULT_PORT` constant in `SolidSyslogTransport.h`, alongside the UDP and TCP defaults. Caller-supplied via the endpoint callback so multi-port deployments can override |
| 5.1 | Server certificate validation | Supported | `SSL_VERIFY_PEER` + `SSL_CTX_load_verify_locations` + `SSL_set1_host` hostname check |
| 5.2 | Mutual TLS (client certificate) | Supported | Optional `clientCertChainPath` / `clientKeyPath` on `SolidSyslogTlsStreamConfig`; `SSL_CTX_check_private_key` confirms pairing |
| 5.3 | TLS 1.2+ cipher suites | Supported | `SSL_CTX_set_min_proto_version(TLS1_2_VERSION)` pinned; caller-supplied `cipherList` via `SSL_CTX_set_cipher_list` |
| 5.4 | Octet counting framing (mandatory for TLS) | Supported | Reuses `SolidSyslogStreamSender` (RFC 6587 framing is identical) |
| 5.5 | TLS close_notify handling | Supported | `SSL_shutdown` in `TlsStream_Close` |

## Summary

| RFC | Total requirements | Supported | Partial | Planned | N/A |
|---|---|---|---|---|---|
| RFC 5424 | 17 | 17 | 0 | 0 | 0 |
| RFC 5426 | 6 | 4 | 0 | 0 | 2 |
| RFC 6587 | 8 | 7 | 0 | 0 | 1 |
| RFC 5425 | 7 | 7 | 0 | 0 | 0 |
