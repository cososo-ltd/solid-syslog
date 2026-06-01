#ifndef SOLIDSYSLOG_TUNABLES_DEFAULTS_H
#define SOLIDSYSLOG_TUNABLES_DEFAULTS_H

// IWYU pragma: private, include "SolidSyslogTunables.h"
// Defaults are reached through the SolidSyslogTunables.h umbrella so the optional
// SOLIDSYSLOG_USER_TUNABLES_FILE override gets a chance to win first.

/*
 * Pool-size tunables are named by ROLE, not by platform or vendor.
 *
 * A build links exactly one implementation of each platform/vendor-selected
 * role (one TCP stream backend, one datagram backend, one mutex, one crypto
 * vendor, ...), so a single role tunable serves whichever implementation is
 * compiled in — the integrator reasons about "how many TCP streams", never
 * "how many POSIX streams". SOLIDSYSLOG_ADDRESS_POOL_SIZE established this
 * pattern; the role blocks below follow it.
 *
 * The pool counts INSTANCES, not implementations. If a future build ever
 * wires two implementations of the same role into one executable (e.g. a
 * numeric AND a DNS resolver, or two crypto vendors), size that role's pool
 * to the SUM of the concurrent instances.
 */

/*
 * Maximum bytes the library will format for a single syslog message.
 *
 * Caps the per-Log Formatter buffer (stack-allocated inside SolidSyslog_Log),
 * the FileStore record size, and the minimum BlockStore block size. Per
 * RFC 5424 section 6.1, receivers SHOULD accept up to 2048 bytes; transports
 * may impose lower limits beyond which truncation kicks in. Lower values save
 * stack and store footprint; higher values reduce truncation.
 *
 * Floor: smallest size that can hold a meaningful RFC 5424 security event.
 * A legal all-NILVALUE message is 16 bytes; this allows enough headroom
 * for a real PRI, hostname, MSGID, and short payload.
 * Sub-floor values rejected at compile time.
 */
#ifndef SOLIDSYSLOG_MAX_MESSAGE_SIZE
#define SOLIDSYSLOG_MAX_MESSAGE_SIZE 2048U /* RFC 5424 section 6.1 SHOULD value */
#endif

#if SOLIDSYSLOG_MAX_MESSAGE_SIZE < 64
#error "SOLIDSYSLOG_MAX_MESSAGE_SIZE must be >= 64"
#endif

/*
 * Maximum bytes the library will format for a single on-disk path
 * produced by SolidSyslogFileBlockDevice (path prefix + two-digit
 * sequence number + ".log" + null terminator). Caps both the per-call
 * Formatter storage and the bounded prefix copy.
 *
 * Floor: smallest size that still leaves room for a meaningful prefix
 * after subtracting the 6-byte filename suffix and null terminator.
 * 32 leaves 24 characters for the prefix — enough for any sane
 * filesystem location. Sub-floor values rejected at compile time.
 */
#ifndef SOLIDSYSLOG_MAX_PATH_SIZE
#define SOLIDSYSLOG_MAX_PATH_SIZE 128U
#endif

#if SOLIDSYSLOG_MAX_PATH_SIZE < 32
#error "SOLIDSYSLOG_MAX_PATH_SIZE must be >= 32"
#endif

/*
 * Maximum bytes of integrity-tag the library will reserve per record.
 * Drives the RecordStore per-record buffer width — every record carries
 * a tag this wide regardless of the active SolidSyslogSecurityPolicy.
 * Default 32 is large enough for HMAC-SHA256; CRC-16 uses 2 of those
 * bytes; the rest is unused slack the integrator can recover by
 * dropping this in their tunables override.
 *
 * Floor: 4. Enough for CRC-32 or a truncated MAC. Sub-floor values
 * rejected at compile time.
 */
#ifndef SOLIDSYSLOG_MAX_INTEGRITY_SIZE
#define SOLIDSYSLOG_MAX_INTEGRITY_SIZE 32U
#endif

#if SOLIDSYSLOG_MAX_INTEGRITY_SIZE < 4
#error "SOLIDSYSLOG_MAX_INTEGRITY_SIZE must be >= 4"
#endif

/*
 * Number of SolidSyslog instances the library's internal static pool
 * can simultaneously hold. Each instance is a small bookkeeping struct
 * (collaborator pointers + SD array pointer + count).
 *
 * Most integrators only ever create one SolidSyslog per process; default 1.
 * Bump via SOLIDSYSLOG_USER_TUNABLES_FILE if the integrator needs multiple
 * concurrent SolidSyslog instances.
 *
 * Floor: 1. Sub-floor values rejected at compile time.
 */
#ifndef SOLIDSYSLOG_POOL_SIZE
#define SOLIDSYSLOG_POOL_SIZE 1U
#endif

#if SOLIDSYSLOG_POOL_SIZE < 1
#error "SOLIDSYSLOG_POOL_SIZE must be >= 1"
#endif

/*
 * Number of SolidSyslogCircularBuffer instances the library's internal
 * static pool can simultaneously hold. Each instance is a small
 * bookkeeping struct (vtable, mutex pointer, ring pointer, head/tail/wrap)
 * — roughly 64 bytes on a 64-bit target, 32 on a 32-bit target. The
 * caller's ring memory is separate (passed to _Create).
 *
 * Most integrators only ever create one CircularBuffer per process;
 * default 1. Bump via SOLIDSYSLOG_USER_TUNABLES_FILE if the integrator
 * needs multiple concurrent buffer instances.
 *
 * Floor: 1. Sub-floor values rejected at compile time.
 */
#ifndef SOLIDSYSLOG_CIRCULAR_BUFFER_POOL_SIZE
#define SOLIDSYSLOG_CIRCULAR_BUFFER_POOL_SIZE 1U
#endif

#if SOLIDSYSLOG_CIRCULAR_BUFFER_POOL_SIZE < 1
#error "SOLIDSYSLOG_CIRCULAR_BUFFER_POOL_SIZE must be >= 1"
#endif

/*
 * Role pool: Mutex. Number of mutex instances the library's internal static
 * pool can simultaneously hold, across whichever implementation is compiled
 * in — SolidSyslogPosixMutex (pthread_mutex_t), SolidSyslogWindowsMutex
 * (CRITICAL_SECTION), or SolidSyslogFreeRtosMutex (StaticSemaphore_t).
 *
 * Default 1 — most integrators wire a single mutex into a CircularBuffer or
 * other thread-safe primitive. Targets that need more (e.g. a separate
 * lifecycle mutex alongside a buffer mutex) bump this via
 * SOLIDSYSLOG_USER_TUNABLES_FILE.
 *
 * Floor: 1. Sub-floor values rejected at compile time.
 */
#ifndef SOLIDSYSLOG_MUTEX_POOL_SIZE
#define SOLIDSYSLOG_MUTEX_POOL_SIZE 1U
#endif

#if SOLIDSYSLOG_MUTEX_POOL_SIZE < 1
#error "SOLIDSYSLOG_MUTEX_POOL_SIZE must be >= 1"
#endif

/*
 * Role pool: Datagram (UDP transport). Number of datagram instances the
 * library's internal static pool can simultaneously hold, across whichever
 * implementation is compiled in — SolidSyslogPosixDatagram,
 * SolidSyslogWinsockDatagram, SolidSyslogPlusTcpDatagram, or
 * SolidSyslogLwipRawDatagram.
 *
 * Default 1 — almost all integrators wire a single datagram into a UdpSender.
 * Bump via SOLIDSYSLOG_USER_TUNABLES_FILE if more than one is genuinely
 * needed.
 *
 * Floor: 1. Sub-floor values rejected at compile time.
 */
#ifndef SOLIDSYSLOG_DATAGRAM_POOL_SIZE
#define SOLIDSYSLOG_DATAGRAM_POOL_SIZE 1U
#endif

#if SOLIDSYSLOG_DATAGRAM_POOL_SIZE < 1
#error "SOLIDSYSLOG_DATAGRAM_POOL_SIZE must be >= 1"
#endif

/*
 * Role pool: Resolver. Number of resolver instances the library's internal
 * static pool can simultaneously hold, across whichever implementation is
 * compiled in — SolidSyslogGetAddrInfoResolver, SolidSyslogWinsockResolver,
 * SolidSyslogPlusTcpResolver, SolidSyslogLwipRawResolver, or
 * SolidSyslogLwipRawDnsResolver.
 *
 * Default 1 — almost all integrators wire a single resolver shared across
 * their Senders. If a build wires two resolver implementations into one
 * executable (e.g. the lwIP numeric AND DNS resolver), set this to the sum
 * via SOLIDSYSLOG_USER_TUNABLES_FILE.
 *
 * Floor: 1. Sub-floor values rejected at compile time.
 */
#ifndef SOLIDSYSLOG_RESOLVER_POOL_SIZE
#define SOLIDSYSLOG_RESOLVER_POOL_SIZE 1U
#endif

#if SOLIDSYSLOG_RESOLVER_POOL_SIZE < 1
#error "SOLIDSYSLOG_RESOLVER_POOL_SIZE must be >= 1"
#endif

/*
 * Role pool: File. Number of file instances the library's internal static
 * pool can simultaneously hold, across whichever implementation is compiled
 * in — SolidSyslogPosixFile, SolidSyslogWindowsFile, or SolidSyslogFatFsFile.
 *
 * Default 1 — almost all integrators wire a single file into a
 * FileBlockDevice. Integrators using FileBlockDevice with BlockStore may
 * want to bump this in line with SOLIDSYSLOG_BLOCK_STORE_POOL_SIZE via
 * SOLIDSYSLOG_USER_TUNABLES_FILE.
 *
 * Floor: 1. Sub-floor values rejected at compile time.
 */
#ifndef SOLIDSYSLOG_FILE_POOL_SIZE
#define SOLIDSYSLOG_FILE_POOL_SIZE 1U
#endif

#if SOLIDSYSLOG_FILE_POOL_SIZE < 1
#error "SOLIDSYSLOG_FILE_POOL_SIZE must be >= 1"
#endif

/*
 * Role pool: TCP stream. Number of TCP stream instances the library's
 * internal static pool can simultaneously hold, across whichever
 * implementation is compiled in — SolidSyslogPosixTcpStream,
 * SolidSyslogWinsockTcpStream, SolidSyslogPlusTcpTcpStream, or
 * SolidSyslogLwipRawTcpStream.
 *
 * Default 2 — common multi-transport wirings combine a plain TCP stream with
 * a second TCP stream that underlies a TLS stream (TLS wraps an injected
 * Stream as its byte transport), so a pool of 1 would silently fall the
 * second Create back to NullStream. Bump via SOLIDSYSLOG_USER_TUNABLES_FILE
 * for wirings that need more.
 *
 * Floor: 1. Sub-floor values rejected at compile time.
 */
#ifndef SOLIDSYSLOG_TCP_STREAM_POOL_SIZE
#define SOLIDSYSLOG_TCP_STREAM_POOL_SIZE 2U
#endif

#if SOLIDSYSLOG_TCP_STREAM_POOL_SIZE < 1
#error "SOLIDSYSLOG_TCP_STREAM_POOL_SIZE must be >= 1"
#endif

/*
 * Number of SolidSyslogPosixMessageQueueBuffer instances the library's
 * internal static pool can simultaneously hold. Each instance carries
 * an mqd_t plus the per-process queue name (Formatter storage).
 *
 * Default 1 — almost all integrators wire a single MQ-backed buffer.
 * The queue name derives from the process ID, so bumping above 1 in
 * the same process would race multiple slots onto the same
 * `/solidsyslog_<pid>` name; an integrator needing N > 1 must
 * additionally distinguish the names (out of scope today).
 *
 * Floor: 1. Sub-floor values rejected at compile time.
 */
#ifndef SOLIDSYSLOG_POSIX_MESSAGE_QUEUE_BUFFER_POOL_SIZE
#define SOLIDSYSLOG_POSIX_MESSAGE_QUEUE_BUFFER_POOL_SIZE 1U
#endif

#if SOLIDSYSLOG_POSIX_MESSAGE_QUEUE_BUFFER_POOL_SIZE < 1
#error "SOLIDSYSLOG_POSIX_MESSAGE_QUEUE_BUFFER_POOL_SIZE must be >= 1"
#endif

/*
 * Number of SolidSyslogPassthroughBuffer instances the library's
 * internal static pool can simultaneously hold. Each instance is
 * tiny (vtable + a Sender pointer).
 *
 * PassthroughBuffer is the single-task "direct-send, no buffering"
 * configuration — every integrator typically creates one. Default 1.
 * Bump via SOLIDSYSLOG_USER_TUNABLES_FILE if more than one process or
 * task needs its own passthrough Buffer instance.
 *
 * Floor: 1. Sub-floor values rejected at compile time.
 */
#ifndef SOLIDSYSLOG_PASSTHROUGH_BUFFER_POOL_SIZE
#define SOLIDSYSLOG_PASSTHROUGH_BUFFER_POOL_SIZE 1U
#endif

#if SOLIDSYSLOG_PASSTHROUGH_BUFFER_POOL_SIZE < 1
#error "SOLIDSYSLOG_PASSTHROUGH_BUFFER_POOL_SIZE must be >= 1"
#endif

/*
 * Number of SolidSyslogUdpSender instances the library's internal
 * static pool can simultaneously hold. Each instance carries its
 * config (resolver/datagram/endpoint pointers), the resolved address
 * storage, and connection state.
 *
 * Default 1 — almost all integrators wire a single UDP sender into
 * either SolidSyslogConfig directly or as one branch of a
 * SwitchingSender. Bump via SOLIDSYSLOG_USER_TUNABLES_FILE if more
 * than one is genuinely needed.
 *
 * Floor: 1. Sub-floor values rejected at compile time.
 */
#ifndef SOLIDSYSLOG_UDP_SENDER_POOL_SIZE
#define SOLIDSYSLOG_UDP_SENDER_POOL_SIZE 1U
#endif

#if SOLIDSYSLOG_UDP_SENDER_POOL_SIZE < 1
#error "SOLIDSYSLOG_UDP_SENDER_POOL_SIZE must be >= 1"
#endif

/*
 * Number of SolidSyslogSwitchingSender instances the library's
 * internal static pool can simultaneously hold.
 *
 * Default 1 — a SwitchingSender wraps several inner senders, so one
 * per process is the typical pattern. Bump via SOLIDSYSLOG_USER_TUNABLES_FILE
 * if more than one is genuinely needed.
 *
 * Floor: 1. Sub-floor values rejected at compile time.
 */
#ifndef SOLIDSYSLOG_SWITCHING_SENDER_POOL_SIZE
#define SOLIDSYSLOG_SWITCHING_SENDER_POOL_SIZE 1U
#endif

#if SOLIDSYSLOG_SWITCHING_SENDER_POOL_SIZE < 1
#error "SOLIDSYSLOG_SWITCHING_SENDER_POOL_SIZE must be >= 1"
#endif

/*
 * Number of SolidSyslogStreamSender instances the library's internal
 * static pool can simultaneously hold. Each instance carries its
 * config (resolver/stream/endpoint pointers) and connection state.
 *
 * Default 2 — common multi-transport wirings combine a plain TCP
 * stream sender with a TLS stream sender behind a SwitchingSender so
 * a TLS failure can fall back to plain TCP (or vice-versa). A pool of
 * 1 would starve the second branch and silently resolve it to the
 * shared SolidSyslogNullSender. Bump via SOLIDSYSLOG_USER_TUNABLES_FILE
 * for wirings that need more.
 *
 * Floor: 1. Sub-floor values rejected at compile time.
 */
#ifndef SOLIDSYSLOG_STREAM_SENDER_POOL_SIZE
#define SOLIDSYSLOG_STREAM_SENDER_POOL_SIZE 2U
#endif

#if SOLIDSYSLOG_STREAM_SENDER_POOL_SIZE < 1
#error "SOLIDSYSLOG_STREAM_SENDER_POOL_SIZE must be >= 1"
#endif

/*
 * Number of SolidSyslogBlockStore instances the library's internal
 * static pool can simultaneously hold. Sizes three pools 1:1 — the
 * BlockStore slot itself, plus the TU-internal RecordStore and
 * BlockSequence pools that each BlockStore composes. The 1:1
 * invariant means a BlockStore slot is guaranteed a free RecordStore
 * and BlockSequence on Create; if any inner Create returns its
 * fallback under normal use, the BlockStore as a whole resolves to
 * SolidSyslogNullStore.
 *
 * Default 1 — almost all integrators wire a single store-and-forward
 * BlockStore. Bump via SOLIDSYSLOG_USER_TUNABLES_FILE if more than
 * one is genuinely needed.
 *
 * Floor: 1. Sub-floor values rejected at compile time.
 */
#ifndef SOLIDSYSLOG_BLOCK_STORE_POOL_SIZE
#define SOLIDSYSLOG_BLOCK_STORE_POOL_SIZE 1U
#endif

#if SOLIDSYSLOG_BLOCK_STORE_POOL_SIZE < 1
#error "SOLIDSYSLOG_BLOCK_STORE_POOL_SIZE must be >= 1"
#endif

/*
 * Number of SolidSyslogFileBlockDevice instances the library's internal
 * static pool can simultaneously hold. Each instance carries the cached
 * open-file handle plus the path-prefix pointer.
 *
 * Default 1 — almost all integrators wire a single FileBlockDevice as
 * the backing store for one BlockStore. Bump via
 * SOLIDSYSLOG_USER_TUNABLES_FILE if more than one is genuinely needed.
 *
 * Floor: 1. Sub-floor values rejected at compile time.
 */
#ifndef SOLIDSYSLOG_FILE_BLOCK_DEVICE_POOL_SIZE
#define SOLIDSYSLOG_FILE_BLOCK_DEVICE_POOL_SIZE 1U
#endif

#if SOLIDSYSLOG_FILE_BLOCK_DEVICE_POOL_SIZE < 1
#error "SOLIDSYSLOG_FILE_BLOCK_DEVICE_POOL_SIZE must be >= 1"
#endif

/*
 * Number of SolidSyslogMetaSd instances the library's internal
 * static pool can simultaneously hold. Default 1 — meta SD is typically
 * wired into SolidSyslogConfig.Sd[] once per process.
 *
 * Floor: 1. Sub-floor values rejected at compile time.
 */
#ifndef SOLIDSYSLOG_META_SD_POOL_SIZE
#define SOLIDSYSLOG_META_SD_POOL_SIZE 1U
#endif

#if SOLIDSYSLOG_META_SD_POOL_SIZE < 1
#error "SOLIDSYSLOG_META_SD_POOL_SIZE must be >= 1"
#endif

/*
 * Number of SolidSyslogTimeQualitySd instances the library's internal
 * static pool can simultaneously hold. Default 1.
 *
 * Floor: 1. Sub-floor values rejected at compile time.
 */
#ifndef SOLIDSYSLOG_TIME_QUALITY_SD_POOL_SIZE
#define SOLIDSYSLOG_TIME_QUALITY_SD_POOL_SIZE 1U
#endif

#if SOLIDSYSLOG_TIME_QUALITY_SD_POOL_SIZE < 1
#error "SOLIDSYSLOG_TIME_QUALITY_SD_POOL_SIZE must be >= 1"
#endif

/*
 * Number of SolidSyslogOriginSd instances the library's internal
 * static pool can simultaneously hold. Each instance carries the
 * pre-formatted static-prefix Formatter storage (software, swVersion,
 * enterpriseId) so the per-message Format only splices in the IP
 * params. Larger per-slot footprint than the other SDs.
 *
 * Default 1.
 *
 * Floor: 1. Sub-floor values rejected at compile time.
 */
#ifndef SOLIDSYSLOG_ORIGIN_SD_POOL_SIZE
#define SOLIDSYSLOG_ORIGIN_SD_POOL_SIZE 1U
#endif

#if SOLIDSYSLOG_ORIGIN_SD_POOL_SIZE < 1
#error "SOLIDSYSLOG_ORIGIN_SD_POOL_SIZE must be >= 1"
#endif

/*
 * Period (milliseconds) the SolidSyslogLwipRawTcpStream bounded-connect
 * spin loop sleeps between polls of the lwIP-side connected_cb flag.
 * Each iteration calls the integrator-injected SolidSyslogSleepFunction
 * so the loop never busy-waits — under NO_SYS=1 the integrator's Sleep
 * implementation ticks sys_check_timeouts and drives RX; under NO_SYS=0
 * it yields to the tcpip thread (vTaskDelay or equivalent).
 *
 * Default 10 ms gives 20 polls inside the default 200 ms connect deadline.
 *
 * Floor: 1 ms. Sub-floor values rejected at compile time.
 */
#ifndef SOLIDSYSLOG_LWIP_RAW_TCP_CONNECT_POLL_MS
#define SOLIDSYSLOG_LWIP_RAW_TCP_CONNECT_POLL_MS 10U
#endif

#if SOLIDSYSLOG_LWIP_RAW_TCP_CONNECT_POLL_MS < 1
#error "SOLIDSYSLOG_LWIP_RAW_TCP_CONNECT_POLL_MS must be >= 1"
#endif

/*
 * Bounded deadline (milliseconds) the SolidSyslogLwipRawDnsResolver waits for
 * an async dns_gethostbyname lookup to complete before giving up and reporting
 * a failed resolution. The synchronous Resolve() contract bridges lwIP's async
 * DNS by spinning on the caller's thread (sleeping via the injected
 * SolidSyslogSleepFunction) until the dns_found_callback fires or this deadline
 * elapses.
 *
 * Default 5000 ms — DNS is markedly slower than the 200 ms TCP connect: a cold
 * lookup may traverse a recursive resolver and the network round-trip dominates.
 * There is deliberately no per-instance runtime getter (unlike the TCP connect
 * timeout); DNS timeout rarely needs live tuning. Override at build time via
 * SOLIDSYSLOG_USER_TUNABLES_FILE if a deployment's resolver is slower.
 *
 * Floor: 1 ms. Sub-floor values rejected at compile time.
 */
#ifndef SOLIDSYSLOG_DNS_RESOLVE_TIMEOUT_MS
#define SOLIDSYSLOG_DNS_RESOLVE_TIMEOUT_MS 5000U
#endif

#if SOLIDSYSLOG_DNS_RESOLVE_TIMEOUT_MS < 1
#error "SOLIDSYSLOG_DNS_RESOLVE_TIMEOUT_MS must be >= 1"
#endif

/*
 * Period (milliseconds) the SolidSyslogLwipRawDnsResolver bounded-resolve spin
 * loop sleeps between polls of the lwIP-side dns_found_callback done flag.
 * Each iteration calls the integrator-injected SolidSyslogSleepFunction so the
 * loop never busy-waits — under NO_SYS=1 the integrator's Sleep ticks
 * sys_check_timeouts and drives the DNS retransmit timer; under NO_SYS=0 it
 * yields to the tcpip thread (vTaskDelay or equivalent). Mirrors
 * SOLIDSYSLOG_LWIP_RAW_TCP_CONNECT_POLL_MS.
 *
 * Default 10 ms gives 500 polls inside the default 5000 ms resolve deadline.
 *
 * Floor: 1 ms. Sub-floor values rejected at compile time.
 */
#ifndef SOLIDSYSLOG_LWIP_RAW_DNS_RESOLVE_POLL_MS
#define SOLIDSYSLOG_LWIP_RAW_DNS_RESOLVE_POLL_MS 10U
#endif

#if SOLIDSYSLOG_LWIP_RAW_DNS_RESOLVE_POLL_MS < 1
#error "SOLIDSYSLOG_LWIP_RAW_DNS_RESOLVE_POLL_MS must be >= 1"
#endif

/*
 * Maximum number of struct pbuf* the SolidSyslogLwipRawTcpStream RX queue
 * holds before backpressuring lwIP. Bounds the *count* of queued pbufs,
 * not their byte volume — lwIP's TCP_WND and MEMP_NUM_PBUF cap upstream
 * receive bytes; this knob caps how many segment-sized pbufs can pile up
 * behind a slow Stream_Read drain before the tcp_recv callback returns
 * non-ERR_OK so lwIP retains the pbuf and replays the callback later.
 *
 * Default 8 — sized for the typical mTLS handshake flight (ServerHello +
 * Certificate + ServerKeyExchange + ServerHelloDone is 2-4 segments; 8
 * leaves margin for cert chains and renegotiation traffic).
 *
 * Floor: 1. Sub-floor values rejected at compile time.
 */
#ifndef SOLIDSYSLOG_LWIP_RAW_TCP_RX_QUEUE_SIZE
#define SOLIDSYSLOG_LWIP_RAW_TCP_RX_QUEUE_SIZE 8U
#endif

#if SOLIDSYSLOG_LWIP_RAW_TCP_RX_QUEUE_SIZE < 1
#error "SOLIDSYSLOG_LWIP_RAW_TCP_RX_QUEUE_SIZE must be >= 1"
#endif

/*
 * Role pool: AtomicCounter. Number of atomic-counter instances the library's
 * internal static pool can simultaneously hold, across whichever
 * implementation is compiled in — SolidSyslogStdAtomicCounter (C11
 * <stdatomic.h>) or SolidSyslogWindowsAtomicCounter (legacy MSVC
 * InterlockedCompareExchange). Each instance carries a single counter word
 * (the sequenceId counter).
 *
 * Default 1 — RFC 5424 sequenceIds are scoped per SolidSyslog instance, and
 * almost all integrators run a single SolidSyslog instance per process. Bump
 * via SOLIDSYSLOG_USER_TUNABLES_FILE if more than one is genuinely needed.
 *
 * Floor: 1. Sub-floor values rejected at compile time.
 */
#ifndef SOLIDSYSLOG_ATOMIC_COUNTER_POOL_SIZE
#define SOLIDSYSLOG_ATOMIC_COUNTER_POOL_SIZE 1U
#endif

#if SOLIDSYSLOG_ATOMIC_COUNTER_POOL_SIZE < 1
#error "SOLIDSYSLOG_ATOMIC_COUNTER_POOL_SIZE must be >= 1"
#endif

/*
 * Role pool: TLS stream. Number of TLS stream instances the library's
 * internal static pool can simultaneously hold, across whichever crypto
 * vendor is compiled in — SolidSyslogTlsStream (OpenSSL) or
 * SolidSyslogMbedTlsStream (Mbed TLS). Each instance carries the vendor's
 * session/context handles and the integrator's TLS config.
 *
 * Default 1 — TLS senders are scoped per destination and almost all
 * integrators wire a single TLS sender per process. Bump via
 * SOLIDSYSLOG_USER_TUNABLES_FILE if more than one is genuinely needed
 * (e.g. multi-destination egress with separate TLS sessions per peer).
 *
 * Floor: 1. Sub-floor values rejected at compile time.
 */
#ifndef SOLIDSYSLOG_TLS_STREAM_POOL_SIZE
#define SOLIDSYSLOG_TLS_STREAM_POOL_SIZE 1U
#endif

#if SOLIDSYSLOG_TLS_STREAM_POOL_SIZE < 1
#error "SOLIDSYSLOG_TLS_STREAM_POOL_SIZE must be >= 1"
#endif

/*
 * Role pool: HMAC-SHA256 SecurityPolicy. Number of keyed HMAC policy
 * instances the library's internal static pool can simultaneously hold,
 * across whichever crypto vendor is compiled in —
 * SolidSyslogMbedTlsHmacSha256Policy or SolidSyslogOpenSslHmacSha256Policy.
 * Each instance carries the integrator's key-accessor callback
 * (SolidSyslogKeyFunction) and its context — the policy fetches the key on
 * demand and never stores it.
 *
 * Default 1 — a single at-rest store with one integrity policy is the common
 * case. Bump via SOLIDSYSLOG_USER_TUNABLES_FILE if more than one store with
 * an independent key is genuinely needed.
 *
 * Floor: 1. Sub-floor values rejected at compile time.
 */
#ifndef SOLIDSYSLOG_HMAC_SHA256_POLICY_POOL_SIZE
#define SOLIDSYSLOG_HMAC_SHA256_POLICY_POOL_SIZE 1U
#endif

#if SOLIDSYSLOG_HMAC_SHA256_POLICY_POOL_SIZE < 1
#error "SOLIDSYSLOG_HMAC_SHA256_POLICY_POOL_SIZE must be >= 1"
#endif

/*
 * Maximum HMAC key length, in bytes, a keyed SecurityPolicy will fetch from
 * its SolidSyslogKeyFunction into a transient on-stack buffer (wiped after
 * each use). Sized for the SHA-256 HMAC block (64 bytes): RFC 2104 keys up to
 * the hash block size are used directly; longer keys are pre-hashed by the
 * HMAC itself, so 64 covers the recommended range. Bump via
 * SOLIDSYSLOG_USER_TUNABLES_FILE only if an integrator's GetKey returns a
 * longer key verbatim.
 *
 * Floor: 32 (the SHA-256 output size, the RFC-recommended minimum key length).
 */
#ifndef SOLIDSYSLOG_MAX_HMAC_KEY_SIZE
#define SOLIDSYSLOG_MAX_HMAC_KEY_SIZE 64U
#endif

#if SOLIDSYSLOG_MAX_HMAC_KEY_SIZE < 32
#error "SOLIDSYSLOG_MAX_HMAC_KEY_SIZE must be >= 32"
#endif

/*
 * Number of SolidSyslog{Posix,Winsock,FreeRtos}Address instances the
 * library's internal static pool can simultaneously hold. Each instance
 * carries one platform sockaddr (struct sockaddr_in on POSIX/Windows,
 * struct freertos_sockaddr on FreeRTOS) — ~16 bytes per slot.
 *
 * Default 3 — matches the canonical BDD multi-transport wiring
 * (UDP + plain-TCP + TLS-stream, one Address per Sender) so common
 * integrators are spared an override. Same trade-off as
 * SOLIDSYSLOG_TCP_STREAM_POOL_SIZE / _STREAM_SENDER_POOL_SIZE:
 * single-transport integrators pay ~32 bytes of unused slots per platform;
 * multi-transport integrators get the canonical wiring out of the box.
 * Bump via SOLIDSYSLOG_USER_TUNABLES_FILE if more than three concurrent
 * senders are needed.
 *
 * Floor: 1. Sub-floor values rejected at compile time.
 */
#ifndef SOLIDSYSLOG_ADDRESS_POOL_SIZE
#define SOLIDSYSLOG_ADDRESS_POOL_SIZE 3U
#endif

#if SOLIDSYSLOG_ADDRESS_POOL_SIZE < 1
#error "SOLIDSYSLOG_ADDRESS_POOL_SIZE must be >= 1"
#endif

/*
 * Default bounded-connect deadline applied by every TCP Stream backend
 * (POSIX, Winsock, FreeRTOS) when the integrator does not install a
 * SolidSyslogTcpConnectTimeoutFunction on the config struct. 200 ms is
 * comfortable for loopback / LAN and short enough that ten failing attempts
 * cost 2 s; raise it for WAN deployments behind a high-RTT link.
 *
 * Runtime override: install GetConnectTimeoutMs on the per-Stream config —
 * the getter is invoked on every connect attempt so live tuning takes effect
 * without rebuilding or recreating the stream.
 *
 * Floor: 1 ms. Sub-floor values rejected at compile time.
 */
#ifndef SOLIDSYSLOG_TCP_CONNECT_TIMEOUT_MS
#define SOLIDSYSLOG_TCP_CONNECT_TIMEOUT_MS 200U
#endif

#if SOLIDSYSLOG_TCP_CONNECT_TIMEOUT_MS < 1
#error "SOLIDSYSLOG_TCP_CONNECT_TIMEOUT_MS must be >= 1"
#endif

/*
 * Default bounded TLS handshake deadline applied by every TLS Stream backend
 * (OpenSSL, Mbed TLS) when the integrator does not install a
 * SolidSyslogTlsHandshakeTimeoutFunction on the config struct. 5 s covers a
 * full TLS 1.2 / 1.3 exchange on a healthy LAN with cert validation; raise
 * it for WAN deployments or constrained MCUs that handshake slowly.
 *
 * Runtime override: install GetHandshakeTimeoutMs on the per-Stream config —
 * the getter is invoked on every handshake attempt so live tuning takes
 * effect without rebuilding or recreating the stream.
 *
 * Floor: 1 ms. Sub-floor values rejected at compile time.
 */
#ifndef SOLIDSYSLOG_TLS_HANDSHAKE_TIMEOUT_MS
#define SOLIDSYSLOG_TLS_HANDSHAKE_TIMEOUT_MS 5000U
#endif

#if SOLIDSYSLOG_TLS_HANDSHAKE_TIMEOUT_MS < 1
#error "SOLIDSYSLOG_TLS_HANDSHAKE_TIMEOUT_MS must be >= 1"
#endif

#endif /* SOLIDSYSLOG_TUNABLES_DEFAULTS_H */
