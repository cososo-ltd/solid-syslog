/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#ifndef SOLIDSYSLOGMACROS_H
#define SOLIDSYSLOGMACROS_H

/* Compile-time assertion. C++11 and C11 have native primitives that carry the
   message into the diagnostic; a strict C99 toolchain - the conformance
   baseline, built on every pull request by the `build-linux-c99` lane (see
   docs/builds.md) - has neither, so it falls back to declaring an array
   whose length goes negative (a constraint violation every C99 compiler
   rejects) when cond is false. The fallback uses a fixed name: repeated
   identical extern declarations in one translation unit are compatible, so no
   per-site uniqueness (and no ## token-pasting) is needed, and it declares no
   object or member that an unused-entity analyser would flag. The fallback has
   no home for msg in the diagnostic, so it is intentionally unreferenced. */
/* NOLINTBEGIN(cppcoreguidelines-macro-usage) */
#define SOLIDSYSLOG_STATIC_ASSERT_STRING_INNER(s) #s
#define SOLIDSYSLOG_STATIC_ASSERT_STRING(s) SOLIDSYSLOG_STATIC_ASSERT_STRING_INNER(s)
#if defined(__cplusplus)
#define SOLIDSYSLOG_STATIC_ASSERT(cond, msg) static_assert((cond), SOLIDSYSLOG_STATIC_ASSERT_STRING(msg))
#elif defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L)
#define SOLIDSYSLOG_STATIC_ASSERT(cond, msg) _Static_assert((cond), SOLIDSYSLOG_STATIC_ASSERT_STRING(msg))
#else
#define SOLIDSYSLOG_STATIC_ASSERT(cond, msg) extern char SolidSyslogStaticAssertViolated[(cond) ? 1 : -1]
#endif
/* NOLINTEND(cppcoreguidelines-macro-usage) */

#endif /* SOLIDSYSLOGMACROS_H */
