#ifndef SOLIDSYSLOGMACROS_H
#define SOLIDSYSLOGMACROS_H

/* Portable compile-time assertion. Uses the negative-array-size trick
   which works from C89 through C23 and all C++ versions — no C11 required. */
/* NOLINTNEXTLINE(cppcoreguidelines-macro-usage) */
#define SOLIDSYSLOG_STATIC_ASSERT(cond, msg) typedef char solidsyslog_static_assert_[(cond) ? 1 : -1]

#endif /* SOLIDSYSLOGMACROS_H */
