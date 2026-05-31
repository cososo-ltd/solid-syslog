#ifndef SOLIDSYSLOGMACROS_H
#define SOLIDSYSLOGMACROS_H

/* Compile-time assertion. C11 (and C++) have a native primitive that carries
   the message into the diagnostic and leaves no unused tag behind, so it is
   used on the day-to-day build. A strict C99 toolchain — the optional
   portability target, exercised by the pre-release `c99` preset (see
   docs/local-checks.md) — has no _Static_assert, so it falls back to a
   uniquely-named struct holding one unsigned bit-field whose width goes
   negative (a constraint violation every C99 compiler rejects) when cond is
   false. The bit-field is unsigned and one bit wide on success: MISRA 6.1 and
   6.2 compliant. The fallback has no home for msg in the diagnostic, so it is
   intentionally unreferenced there. */
/* NOLINTBEGIN(cppcoreguidelines-macro-usage) */
#if defined(__cplusplus) || (defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L))
#define SOLIDSYSLOG_STATIC_ASSERT_STRING_INNER(s) #s
#define SOLIDSYSLOG_STATIC_ASSERT_STRING(s) SOLIDSYSLOG_STATIC_ASSERT_STRING_INNER(s)
#define SOLIDSYSLOG_STATIC_ASSERT(cond, msg) _Static_assert((cond), SOLIDSYSLOG_STATIC_ASSERT_STRING(msg))
#else
#define SOLIDSYSLOG_STATIC_ASSERT_PASTE_INNER(a, b) a##b
#define SOLIDSYSLOG_STATIC_ASSERT_PASTE(a, b) SOLIDSYSLOG_STATIC_ASSERT_PASTE_INNER(a, b)
#define SOLIDSYSLOG_STATIC_ASSERT(cond, msg)                                   \
    struct SOLIDSYSLOG_STATIC_ASSERT_PASTE(SolidSyslogStaticAssert_, __LINE__) \
    {                                                                          \
        unsigned int SolidSyslogStaticAssertViolated : ((cond) ? 1 : -1);      \
    }
#endif
/* NOLINTEND(cppcoreguidelines-macro-usage) */

#endif /* SOLIDSYSLOGMACROS_H */
