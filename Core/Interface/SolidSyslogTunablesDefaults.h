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

#endif /* SOLIDSYSLOG_TUNABLES_DEFAULTS_H */
