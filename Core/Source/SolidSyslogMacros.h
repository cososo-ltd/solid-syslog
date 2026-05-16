#ifndef SOLIDSYSLOGMACROS_H
#define SOLIDSYSLOGMACROS_H

/* Compile-time assertion. The library compiles at --std=c11 so we use the
   standard C11 _Static_assert primitive directly; the msg parameter is named
   at the call site for human readability and stringified by the caller via
   the SOLIDSYSLOG_STATIC_ASSERT_STRING wrapper. */
/* NOLINTBEGIN(cppcoreguidelines-macro-usage) */
#define SOLIDSYSLOG_STATIC_ASSERT_STRING_INNER(s) #s
#define SOLIDSYSLOG_STATIC_ASSERT_STRING(s) SOLIDSYSLOG_STATIC_ASSERT_STRING_INNER(s)
#define SOLIDSYSLOG_STATIC_ASSERT(cond, msg) _Static_assert((cond), SOLIDSYSLOG_STATIC_ASSERT_STRING(msg))
/* NOLINTEND(cppcoreguidelines-macro-usage) */

#endif /* SOLIDSYSLOGMACROS_H */
