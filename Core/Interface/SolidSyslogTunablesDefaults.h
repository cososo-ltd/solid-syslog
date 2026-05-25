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
/* NOLINTNEXTLINE(cppcoreguidelines-macro-usage) -- macro form required for preprocessor visibility (floor #if) and C array-size const-expr. */
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
/* NOLINTNEXTLINE(cppcoreguidelines-macro-usage) -- macro form required for preprocessor visibility (floor #if) and C array-size const-expr. */
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
/* NOLINTNEXTLINE(cppcoreguidelines-macro-usage) -- macro form required for preprocessor visibility (floor #if) and C array-size const-expr. */
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

/*
 * Number of SolidSyslogWindowsMutex instances the library's internal
 * static pool can simultaneously hold. Each instance carries a
 * CRITICAL_SECTION.
 *
 * Default 1 — almost all integrators wire a single WindowsMutex into
 * a CircularBuffer or other thread-safe primitive. Bump via
 * SOLIDSYSLOG_USER_TUNABLES_FILE if more than one is genuinely
 * needed.
 *
 * Floor: 1. Sub-floor values rejected at compile time.
 */
#ifndef SOLIDSYSLOG_WINDOWS_MUTEX_POOL_SIZE
/* NOLINTNEXTLINE(cppcoreguidelines-macro-usage) -- macro form required for preprocessor visibility (floor #if) and C array-size const-expr. */
#define SOLIDSYSLOG_WINDOWS_MUTEX_POOL_SIZE 1U
#endif

#if SOLIDSYSLOG_WINDOWS_MUTEX_POOL_SIZE < 1
#error "SOLIDSYSLOG_WINDOWS_MUTEX_POOL_SIZE must be >= 1"
#endif

/*
 * Number of SolidSyslogWinsockDatagram instances the library's internal
 * static pool can simultaneously hold. Each instance carries the AF_INET
 * SOCKET handle and a one-shot connect flag.
 *
 * Default 1 — almost all integrators wire a single WinsockDatagram into
 * a UdpSender. Bump via SOLIDSYSLOG_USER_TUNABLES_FILE if more than one
 * is genuinely needed.
 *
 * Floor: 1. Sub-floor values rejected at compile time.
 */
#ifndef SOLIDSYSLOG_WINSOCK_DATAGRAM_POOL_SIZE
/* NOLINTNEXTLINE(cppcoreguidelines-macro-usage) -- macro form required for preprocessor visibility (floor #if) and C array-size const-expr. */
#define SOLIDSYSLOG_WINSOCK_DATAGRAM_POOL_SIZE 1U
#endif

#if SOLIDSYSLOG_WINSOCK_DATAGRAM_POOL_SIZE < 1
#error "SOLIDSYSLOG_WINSOCK_DATAGRAM_POOL_SIZE must be >= 1"
#endif

/*
 * Number of SolidSyslogWinsockResolver instances the library's internal
 * static pool can simultaneously hold. The resolver is stateless (its
 * slot just holds the vtable); the pool exists for lifecycle symmetry
 * with the stateful FreeRtosResolver / GetAddrInfoResolver siblings.
 *
 * Default 1.
 *
 * Floor: 1. Sub-floor values rejected at compile time.
 */
#ifndef SOLIDSYSLOG_WINSOCK_RESOLVER_POOL_SIZE
/* NOLINTNEXTLINE(cppcoreguidelines-macro-usage) -- macro form required for preprocessor visibility (floor #if) and C array-size const-expr. */
#define SOLIDSYSLOG_WINSOCK_RESOLVER_POOL_SIZE 1U
#endif

#if SOLIDSYSLOG_WINSOCK_RESOLVER_POOL_SIZE < 1
#error "SOLIDSYSLOG_WINSOCK_RESOLVER_POOL_SIZE must be >= 1"
#endif

/*
 * Number of SolidSyslogWindowsFile instances the library's internal
 * static pool can simultaneously hold. Each instance carries an `int`
 * MSVC CRT file descriptor.
 *
 * Default 1. Integrators using FileBlockDevice with BlockStore may want
 * to bump this in line with SOLIDSYSLOG_BLOCK_STORE_POOL_SIZE.
 *
 * Floor: 1. Sub-floor values rejected at compile time.
 */
#ifndef SOLIDSYSLOG_WINDOWS_FILE_POOL_SIZE
/* NOLINTNEXTLINE(cppcoreguidelines-macro-usage) -- macro form required for preprocessor visibility (floor #if) and C array-size const-expr. */
#define SOLIDSYSLOG_WINDOWS_FILE_POOL_SIZE 1U
#endif

#if SOLIDSYSLOG_WINDOWS_FILE_POOL_SIZE < 1
#error "SOLIDSYSLOG_WINDOWS_FILE_POOL_SIZE must be >= 1"
#endif

/*
 * Number of SolidSyslogWinsockTcpStream instances the library's internal
 * static pool can simultaneously hold. Each instance carries the SOCKET
 * for the non-blocking TCP connection.
 *
 * Default 2 — the BDD target needs a plain-TCP stream and a
 * TLS-underlying-TCP stream concurrently, matching the POSIX pool size.
 *
 * Floor: 1. Sub-floor values rejected at compile time.
 */
#ifndef SOLIDSYSLOG_WINSOCK_TCP_STREAM_POOL_SIZE
/* NOLINTNEXTLINE(cppcoreguidelines-macro-usage) -- macro form required for preprocessor visibility (floor #if) and C array-size const-expr. */
#define SOLIDSYSLOG_WINSOCK_TCP_STREAM_POOL_SIZE 2U
#endif

#if SOLIDSYSLOG_WINSOCK_TCP_STREAM_POOL_SIZE < 1
#error "SOLIDSYSLOG_WINSOCK_TCP_STREAM_POOL_SIZE must be >= 1"
#endif

/*
 * Number of SolidSyslogFreeRtosMutex instances the library's internal
 * static pool can simultaneously hold. Each instance carries a
 * StaticSemaphore_t — the kernel-primitive layout the adapter wraps
 * via xSemaphoreCreateMutexStatic.
 *
 * Default 1 — almost all FreeRTOS integrators wire a single mutex
 * into a CircularBuffer or other thread-safe primitive. Bump via
 * SOLIDSYSLOG_USER_TUNABLES_FILE if more than one is genuinely
 * needed.
 *
 * Floor: 1. Sub-floor values rejected at compile time.
 */
#ifndef SOLIDSYSLOG_FREE_RTOS_MUTEX_POOL_SIZE
/* NOLINTNEXTLINE(cppcoreguidelines-macro-usage) -- macro form required for preprocessor visibility (floor #if) and C array-size const-expr. */
#define SOLIDSYSLOG_FREE_RTOS_MUTEX_POOL_SIZE 1U
#endif

#if SOLIDSYSLOG_FREE_RTOS_MUTEX_POOL_SIZE < 1
#error "SOLIDSYSLOG_FREE_RTOS_MUTEX_POOL_SIZE must be >= 1"
#endif

/*
 * Number of SolidSyslogFreeRtosDatagram instances the library's
 * internal static pool can simultaneously hold. Each instance carries
 * a FreeRTOS-Plus-TCP Socket_t.
 *
 * Default 1 — almost all FreeRTOS integrators wire a single datagram
 * into a UdpSender. Bump via SOLIDSYSLOG_USER_TUNABLES_FILE if more
 * than one is genuinely needed.
 *
 * Floor: 1. Sub-floor values rejected at compile time.
 */
#ifndef SOLIDSYSLOG_FREE_RTOS_DATAGRAM_POOL_SIZE
/* NOLINTNEXTLINE(cppcoreguidelines-macro-usage) -- macro form required for preprocessor visibility (floor #if) and C array-size const-expr. */
#define SOLIDSYSLOG_FREE_RTOS_DATAGRAM_POOL_SIZE 1U
#endif

#if SOLIDSYSLOG_FREE_RTOS_DATAGRAM_POOL_SIZE < 1
#error "SOLIDSYSLOG_FREE_RTOS_DATAGRAM_POOL_SIZE must be >= 1"
#endif

/*
 * Number of SolidSyslogFreeRtosResolver instances the library's
 * internal static pool can simultaneously hold. Each instance carries
 * the 4-byte IPv4 octets the integrator pins it to.
 *
 * Default 1 — the resolver pairs with a single hardcoded
 * destination; the typical FreeRTOS integrator wires one. Bump via
 * SOLIDSYSLOG_USER_TUNABLES_FILE if more than one is needed.
 *
 * Floor: 1. Sub-floor values rejected at compile time.
 */
#ifndef SOLIDSYSLOG_FREE_RTOS_RESOLVER_POOL_SIZE
/* NOLINTNEXTLINE(cppcoreguidelines-macro-usage) -- macro form required for preprocessor visibility (floor #if) and C array-size const-expr. */
#define SOLIDSYSLOG_FREE_RTOS_RESOLVER_POOL_SIZE 1U
#endif

#if SOLIDSYSLOG_FREE_RTOS_RESOLVER_POOL_SIZE < 1
#error "SOLIDSYSLOG_FREE_RTOS_RESOLVER_POOL_SIZE must be >= 1"
#endif

/*
 * Number of SolidSyslogFreeRtosTcpStream instances the library's
 * internal static pool can simultaneously hold. Each instance carries
 * a FreeRTOS-Plus-TCP Socket_t for the bounded-blocking-connect
 * non-blocking TCP transport.
 *
 * Default 2 — matches the POSIX and Windows TCP stream pool defaults
 * so a future TLS-via-mbedTLS path (S08.07) wrapping an underlying
 * FreeRtosTcpStream does not silently fall back to NullStream on the
 * second Create. Plain-TCP-only integrators pay 8 bytes of extra
 * static state; mbedTLS integrators are spared an override.
 *
 * Floor: 1. Sub-floor values rejected at compile time.
 */
#ifndef SOLIDSYSLOG_FREE_RTOS_TCP_STREAM_POOL_SIZE
/* NOLINTNEXTLINE(cppcoreguidelines-macro-usage) -- macro form required for preprocessor visibility (floor #if) and C array-size const-expr. */
#define SOLIDSYSLOG_FREE_RTOS_TCP_STREAM_POOL_SIZE 2U
#endif

#if SOLIDSYSLOG_FREE_RTOS_TCP_STREAM_POOL_SIZE < 1
#error "SOLIDSYSLOG_FREE_RTOS_TCP_STREAM_POOL_SIZE must be >= 1"
#endif

/*
 * Number of SolidSyslogStdAtomicCounter instances the library's
 * internal static pool can simultaneously hold. Each instance carries
 * a single _Atomic uint32_t (the sequenceId counter).
 *
 * Default 1 — RFC 5424 sequenceIds are scoped per SolidSyslog instance,
 * and almost all integrators run a single SolidSyslog instance per
 * process. Bump via SOLIDSYSLOG_USER_TUNABLES_FILE if more than one is
 * genuinely needed (e.g. several independent SolidSyslog instances).
 *
 * Floor: 1. Sub-floor values rejected at compile time.
 */
#ifndef SOLIDSYSLOG_STD_ATOMIC_COUNTER_POOL_SIZE
/* NOLINTNEXTLINE(cppcoreguidelines-macro-usage) -- macro form required for preprocessor visibility (floor #if) and C array-size const-expr. */
#define SOLIDSYSLOG_STD_ATOMIC_COUNTER_POOL_SIZE 1U
#endif

#if SOLIDSYSLOG_STD_ATOMIC_COUNTER_POOL_SIZE < 1
#error "SOLIDSYSLOG_STD_ATOMIC_COUNTER_POOL_SIZE must be >= 1"
#endif

/*
 * Number of SolidSyslogWindowsAtomicCounter instances the library's
 * internal static pool can simultaneously hold. Each instance carries
 * a single `volatile LONG` (the sequenceId counter, manipulated via
 * `InterlockedCompareExchange`).
 *
 * Default 1 — RFC 5424 sequenceIds are scoped per SolidSyslog instance,
 * and almost all integrators run a single SolidSyslog instance per
 * process. Bump via SOLIDSYSLOG_USER_TUNABLES_FILE if more than one is
 * genuinely needed.
 *
 * Floor: 1. Sub-floor values rejected at compile time.
 */
#ifndef SOLIDSYSLOG_WINDOWS_ATOMIC_COUNTER_POOL_SIZE
/* NOLINTNEXTLINE(cppcoreguidelines-macro-usage) -- macro form required for preprocessor visibility (floor #if) and C array-size const-expr. */
#define SOLIDSYSLOG_WINDOWS_ATOMIC_COUNTER_POOL_SIZE 1U
#endif

#if SOLIDSYSLOG_WINDOWS_ATOMIC_COUNTER_POOL_SIZE < 1
#error "SOLIDSYSLOG_WINDOWS_ATOMIC_COUNTER_POOL_SIZE must be >= 1"
#endif

/*
 * Number of SolidSyslogFatFsFile instances the library's internal static
 * pool can simultaneously hold. Each instance carries a FatFs FIL object
 * (~56 B header + 512 B sector buffer when FF_MAX_SS=512, FF_FS_TINY=0)
 * plus an IsOpen flag.
 *
 * Default 1 — store-and-forward integrations wire a single file under
 * BlockStore + FileBlockDevice; that's the dominant pattern. Bump via
 * SOLIDSYSLOG_USER_TUNABLES_FILE if more than one is genuinely needed.
 *
 * Floor: 1. Sub-floor values rejected at compile time.
 */
#ifndef SOLIDSYSLOG_FATFS_FILE_POOL_SIZE
/* NOLINTNEXTLINE(cppcoreguidelines-macro-usage) -- macro form required for preprocessor visibility (floor #if) and C array-size const-expr. */
#define SOLIDSYSLOG_FATFS_FILE_POOL_SIZE 1U
#endif

#if SOLIDSYSLOG_FATFS_FILE_POOL_SIZE < 1
#error "SOLIDSYSLOG_FATFS_FILE_POOL_SIZE must be >= 1"
#endif

/*
 * Number of SolidSyslogTlsStream instances the library's internal static
 * pool can simultaneously hold. Each instance carries an SSL_CTX*, SSL*,
 * BIO_METHOD*, and the integrator's TlsStreamConfig (transport pointer,
 * sleep callback, cert/key/CA paths).
 *
 * Default 1 — TLS senders are scoped per destination and almost all
 * integrators wire a single TLS sender per process. Bump via
 * SOLIDSYSLOG_USER_TUNABLES_FILE if more than one is genuinely needed
 * (e.g. multi-destination egress with separate TLS sessions per peer).
 *
 * Floor: 1. Sub-floor values rejected at compile time.
 */
#ifndef SOLIDSYSLOG_TLS_STREAM_POOL_SIZE
/* NOLINTNEXTLINE(cppcoreguidelines-macro-usage) -- macro form required for preprocessor visibility (floor #if) and C array-size const-expr. */
#define SOLIDSYSLOG_TLS_STREAM_POOL_SIZE 1U
#endif

#if SOLIDSYSLOG_TLS_STREAM_POOL_SIZE < 1
#error "SOLIDSYSLOG_TLS_STREAM_POOL_SIZE must be >= 1"
#endif

/*
 * Number of SolidSyslogMbedTlsStream instances the library's internal static
 * pool can simultaneously hold. Each instance carries an mbedtls_ssl_context,
 * mbedtls_ssl_config, and the integrator's MbedTlsStreamConfig (transport
 * pointer, sleep callback, mbedTLS handle pointers — Rng, CaChain, optional
 * ClientCertChain/ClientKey).
 *
 * Default 1 — TLS senders are scoped per destination and almost all
 * integrators wire a single TLS sender per process. Bump via
 * SOLIDSYSLOG_USER_TUNABLES_FILE if more than one is genuinely needed
 * (e.g. multi-destination egress with separate TLS sessions per peer).
 *
 * Floor: 1. Sub-floor values rejected at compile time.
 */
#ifndef SOLIDSYSLOG_MBED_TLS_STREAM_POOL_SIZE
/* NOLINTNEXTLINE(cppcoreguidelines-macro-usage) -- macro form required for preprocessor visibility (floor #if) and C array-size const-expr. */
#define SOLIDSYSLOG_MBED_TLS_STREAM_POOL_SIZE 1U
#endif

#if SOLIDSYSLOG_MBED_TLS_STREAM_POOL_SIZE < 1
#error "SOLIDSYSLOG_MBED_TLS_STREAM_POOL_SIZE must be >= 1"
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
 * SOLIDSYSLOG_POSIX_TCP_STREAM_POOL_SIZE / _STREAM_SENDER_POOL_SIZE:
 * single-transport integrators pay ~32 bytes of unused slots per platform;
 * multi-transport integrators get the canonical wiring out of the box.
 * Bump via SOLIDSYSLOG_USER_TUNABLES_FILE if more than three concurrent
 * senders are needed.
 *
 * Floor: 1. Sub-floor values rejected at compile time.
 */
#ifndef SOLIDSYSLOG_ADDRESS_POOL_SIZE
/* NOLINTNEXTLINE(cppcoreguidelines-macro-usage) -- macro form required for preprocessor visibility (floor #if) and C array-size const-expr. */
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
/* NOLINTNEXTLINE(cppcoreguidelines-macro-usage) -- macro form required for preprocessor visibility (floor #if) and C array-size const-expr. */
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
/* NOLINTNEXTLINE(cppcoreguidelines-macro-usage) -- macro form required for preprocessor visibility (floor #if) and C array-size const-expr. */
#define SOLIDSYSLOG_TLS_HANDSHAKE_TIMEOUT_MS 5000U
#endif

#if SOLIDSYSLOG_TLS_HANDSHAKE_TIMEOUT_MS < 1
#error "SOLIDSYSLOG_TLS_HANDSHAKE_TIMEOUT_MS must be >= 1"
#endif

#endif /* SOLIDSYSLOG_TUNABLES_DEFAULTS_H */
