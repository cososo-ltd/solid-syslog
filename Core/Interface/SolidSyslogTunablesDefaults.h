#ifndef SOLIDSYSLOG_TUNABLES_DEFAULTS_H
#define SOLIDSYSLOG_TUNABLES_DEFAULTS_H

// IWYU pragma: private, include "SolidSyslogTunables.h"
// Defaults are reached through the SolidSyslogTunables.h umbrella so the optional
// SOLIDSYSLOG_USER_TUNABLES_FILE override gets a chance to win first.

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
/* NOLINTNEXTLINE(cppcoreguidelines-macro-usage) -- macro form required for preprocessor visibility (floor #if) and C array-size const-expr. */
#define SOLIDSYSLOG_MAX_MESSAGE_SIZE 2048U /* RFC 5424 section 6.1 SHOULD value */
#endif

#if SOLIDSYSLOG_MAX_MESSAGE_SIZE < 64
#error "SOLIDSYSLOG_MAX_MESSAGE_SIZE must be >= 64"
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
/* NOLINTNEXTLINE(cppcoreguidelines-macro-usage) -- macro form required for preprocessor visibility (floor #if) and C array-size const-expr. */
#define SOLIDSYSLOG_CIRCULAR_BUFFER_POOL_SIZE 1U
#endif

#if SOLIDSYSLOG_CIRCULAR_BUFFER_POOL_SIZE < 1
#error "SOLIDSYSLOG_CIRCULAR_BUFFER_POOL_SIZE must be >= 1"
#endif

/*
 * Number of SolidSyslogPosixMutex instances the library's internal
 * static pool can simultaneously hold. Each instance carries a
 * pthread_mutex_t.
 *
 * Default 1 — almost all integrators wire a single PosixMutex into
 * a CircularBuffer or other thread-safe primitive. Bump via
 * SOLIDSYSLOG_USER_TUNABLES_FILE if more than one is genuinely
 * needed.
 *
 * Floor: 1. Sub-floor values rejected at compile time.
 */
#ifndef SOLIDSYSLOG_POSIX_MUTEX_POOL_SIZE
/* NOLINTNEXTLINE(cppcoreguidelines-macro-usage) -- macro form required for preprocessor visibility (floor #if) and C array-size const-expr. */
#define SOLIDSYSLOG_POSIX_MUTEX_POOL_SIZE 1U
#endif

#if SOLIDSYSLOG_POSIX_MUTEX_POOL_SIZE < 1
#error "SOLIDSYSLOG_POSIX_MUTEX_POOL_SIZE must be >= 1"
#endif

/*
 * Number of SolidSyslogPosixDatagram instances the library's internal
 * static pool can simultaneously hold. Each instance carries the
 * AF_INET socket FD and a one-shot connect flag.
 *
 * Default 1 — almost all integrators wire a single PosixDatagram into
 * a UdpSender. Bump via SOLIDSYSLOG_USER_TUNABLES_FILE if more than
 * one is genuinely needed.
 *
 * Floor: 1. Sub-floor values rejected at compile time.
 */
#ifndef SOLIDSYSLOG_POSIX_DATAGRAM_POOL_SIZE
/* NOLINTNEXTLINE(cppcoreguidelines-macro-usage) -- macro form required for preprocessor visibility (floor #if) and C array-size const-expr. */
#define SOLIDSYSLOG_POSIX_DATAGRAM_POOL_SIZE 1U
#endif

#if SOLIDSYSLOG_POSIX_DATAGRAM_POOL_SIZE < 1
#error "SOLIDSYSLOG_POSIX_DATAGRAM_POOL_SIZE must be >= 1"
#endif

/*
 * Number of SolidSyslogGetAddrInfoResolver instances the library's
 * internal static pool can simultaneously hold. The class is
 * effectively stateless today — the pool slot carries the vtable
 * holder.
 *
 * Default 1 — almost all integrators wire a single resolver into
 * one or more Senders. Bump via SOLIDSYSLOG_USER_TUNABLES_FILE if
 * more than one is genuinely needed.
 *
 * Floor: 1. Sub-floor values rejected at compile time.
 */
#ifndef SOLIDSYSLOG_GETADDRINFO_RESOLVER_POOL_SIZE
/* NOLINTNEXTLINE(cppcoreguidelines-macro-usage) -- macro form required for preprocessor visibility (floor #if) and C array-size const-expr. */
#define SOLIDSYSLOG_GETADDRINFO_RESOLVER_POOL_SIZE 1U
#endif

#if SOLIDSYSLOG_GETADDRINFO_RESOLVER_POOL_SIZE < 1
#error "SOLIDSYSLOG_GETADDRINFO_RESOLVER_POOL_SIZE must be >= 1"
#endif

/*
 * Number of SolidSyslogPosixFile instances the library's internal
 * static pool can simultaneously hold. Each instance carries an
 * int file descriptor.
 *
 * Default 1 — almost all integrators wire a single PosixFile into a
 * FileBlockDevice. Bump via SOLIDSYSLOG_USER_TUNABLES_FILE if more
 * than one is genuinely needed.
 *
 * Floor: 1. Sub-floor values rejected at compile time.
 */
#ifndef SOLIDSYSLOG_POSIX_FILE_POOL_SIZE
/* NOLINTNEXTLINE(cppcoreguidelines-macro-usage) -- macro form required for preprocessor visibility (floor #if) and C array-size const-expr. */
#define SOLIDSYSLOG_POSIX_FILE_POOL_SIZE 1U
#endif

#if SOLIDSYSLOG_POSIX_FILE_POOL_SIZE < 1
#error "SOLIDSYSLOG_POSIX_FILE_POOL_SIZE must be >= 1"
#endif

/*
 * Number of SolidSyslogPosixTcpStream instances the library's internal
 * static pool can simultaneously hold. Each instance carries an
 * int file descriptor.
 *
 * Default 2 — the Linux BDD target wires *two* stream-framed senders
 * behind a SwitchingSender (plain TCP, plus TLS that wraps another
 * underlying PosixTcpStream as its transport), so default 1 would
 * silently fall back to NullStream on the second Create. Matches the
 * SOLIDSYSLOG_STREAM_SENDER_POOL_SIZE default for the same reason.
 * Bump via SOLIDSYSLOG_USER_TUNABLES_FILE if more are needed.
 *
 * Floor: 1. Sub-floor values rejected at compile time.
 */
#ifndef SOLIDSYSLOG_POSIX_TCP_STREAM_POOL_SIZE
/* NOLINTNEXTLINE(cppcoreguidelines-macro-usage) -- macro form required for preprocessor visibility (floor #if) and C array-size const-expr. */
#define SOLIDSYSLOG_POSIX_TCP_STREAM_POOL_SIZE 2U
#endif

#if SOLIDSYSLOG_POSIX_TCP_STREAM_POOL_SIZE < 1
#error "SOLIDSYSLOG_POSIX_TCP_STREAM_POOL_SIZE must be >= 1"
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
/* NOLINTNEXTLINE(cppcoreguidelines-macro-usage) -- macro form required for preprocessor visibility (floor #if) and C array-size const-expr. */
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
/* NOLINTNEXTLINE(cppcoreguidelines-macro-usage) -- macro form required for preprocessor visibility (floor #if) and C array-size const-expr. */
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
/* NOLINTNEXTLINE(cppcoreguidelines-macro-usage) -- macro form required for preprocessor visibility (floor #if) and C array-size const-expr. */
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
/* NOLINTNEXTLINE(cppcoreguidelines-macro-usage) -- macro form required for preprocessor visibility (floor #if) and C array-size const-expr. */
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
/* NOLINTNEXTLINE(cppcoreguidelines-macro-usage) -- macro form required for preprocessor visibility (floor #if) and C array-size const-expr. */
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
/* NOLINTNEXTLINE(cppcoreguidelines-macro-usage) -- macro form required for preprocessor visibility (floor #if) and C array-size const-expr. */
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
/* NOLINTNEXTLINE(cppcoreguidelines-macro-usage) -- macro form required for preprocessor visibility (floor #if) and C array-size const-expr. */
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
/* NOLINTNEXTLINE(cppcoreguidelines-macro-usage) -- macro form required for preprocessor visibility (floor #if) and C array-size const-expr. */
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
/* NOLINTNEXTLINE(cppcoreguidelines-macro-usage) -- macro form required for preprocessor visibility (floor #if) and C array-size const-expr. */
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
/* NOLINTNEXTLINE(cppcoreguidelines-macro-usage) -- macro form required for preprocessor visibility (floor #if) and C array-size const-expr. */
#define SOLIDSYSLOG_ORIGIN_SD_POOL_SIZE 1U
#endif

#if SOLIDSYSLOG_ORIGIN_SD_POOL_SIZE < 1
#error "SOLIDSYSLOG_ORIGIN_SD_POOL_SIZE must be >= 1"
#endif

#endif /* SOLIDSYSLOG_TUNABLES_DEFAULTS_H */
