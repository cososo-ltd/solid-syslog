#ifndef SOLIDSYSLOGMACROS_H
#define SOLIDSYSLOGMACROS_H

/* Portable compile-time assertion. Uses the negative-array-size trick
   which works from C89 through C23 and all C++ versions — no C11 required.
   The two-step concat with __LINE__ makes each typedef name unique within a
   translation unit, satisfying MISRA C:2012 Rule 5.6 (typedef name shall be
   a unique identifier). */
/* NOLINTBEGIN(cppcoreguidelines-macro-usage) */
#define SOLIDSYSLOG_STATIC_ASSERT_CONCAT_INNER(a, b) a##b
#define SOLIDSYSLOG_STATIC_ASSERT_CONCAT(a, b)       SOLIDSYSLOG_STATIC_ASSERT_CONCAT_INNER(a, b)
#define SOLIDSYSLOG_STATIC_ASSERT(cond, msg)                                                            \
    typedef char SOLIDSYSLOG_STATIC_ASSERT_CONCAT(solidsyslog_static_assert_, __LINE__)[(cond) ? 1 : -1]
/* NOLINTEND(cppcoreguidelines-macro-usage) */

#endif /* SOLIDSYSLOGMACROS_H */
