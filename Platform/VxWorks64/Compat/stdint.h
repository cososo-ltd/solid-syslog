/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  The C99 <stdint.h> for VxWorks 6.4 kernel builds, whose header tree has
 *  none. solidsyslog-vxworks64.mk puts this directory on the library's include
 *  path; code that includes SolidSyslog headers needs it on its own.
 *
 *  Every VxWorks 6.4 kernel target is ILP32 - int, long and pointers 32 bits -
 *  and the types are defined for that. A target that is not fails to compile
 *  here rather than getting types of the wrong width.
 *
 *  Compiled as C89, as a kernel project's own sources usually are, the 64-bit
 *  types are absent: C89 has no long long. */

#ifndef SOLIDSYSLOG_VXWORKS64_COMPAT_STDINT_H
#define SOLIDSYSLOG_VXWORKS64_COMPAT_STDINT_H

#include <limits.h>
#include <stddef.h>

#if (UCHAR_MAX != 0xFF) || (USHRT_MAX != 0xFFFF) || (UINT_MAX != 0xFFFFFFFFU) || (ULONG_MAX != 0xFFFFFFFFUL)
#error "stdint.h: this target is not ILP32 with 8-bit chars and 16-bit shorts"
#endif

/* The preprocessor cannot see sizeof; a negative array size stops the build. */
extern char SolidSyslogVxWorks64Compat_PointerIsFourBytes[(sizeof(void*) == 4U) ? 1 : -1];
extern char SolidSyslogVxWorks64Compat_SizeIsFourBytes[(sizeof(size_t) == 4U) ? 1 : -1];

typedef signed char int8_t;
typedef unsigned char uint8_t;
typedef short int16_t;
typedef unsigned short uint16_t;
typedef int int32_t;
typedef unsigned int uint32_t;

typedef int8_t int_least8_t;
typedef uint8_t uint_least8_t;
typedef int16_t int_least16_t;
typedef uint16_t uint_least16_t;
typedef int32_t int_least32_t;
typedef uint32_t uint_least32_t;

typedef int8_t int_fast8_t;
typedef uint8_t uint_fast8_t;
typedef int16_t int_fast16_t;
typedef uint16_t uint_fast16_t;
typedef int32_t int_fast32_t;
typedef uint32_t uint_fast32_t;

typedef long intptr_t;
typedef unsigned long uintptr_t;

#define INT8_MIN (-127 - 1)
#define INT8_MAX 127
#define UINT8_MAX 255
#define INT16_MIN (-32767 - 1)
#define INT16_MAX 32767
#define UINT16_MAX 65535
#define INT32_MIN (-2147483647 - 1)
#define INT32_MAX 2147483647
#define UINT32_MAX 4294967295U

#define INT_LEAST8_MIN INT8_MIN
#define INT_LEAST8_MAX INT8_MAX
#define UINT_LEAST8_MAX UINT8_MAX
#define INT_LEAST16_MIN INT16_MIN
#define INT_LEAST16_MAX INT16_MAX
#define UINT_LEAST16_MAX UINT16_MAX
#define INT_LEAST32_MIN INT32_MIN
#define INT_LEAST32_MAX INT32_MAX
#define UINT_LEAST32_MAX UINT32_MAX

#define INT_FAST8_MIN INT8_MIN
#define INT_FAST8_MAX INT8_MAX
#define UINT_FAST8_MAX UINT8_MAX
#define INT_FAST16_MIN INT16_MIN
#define INT_FAST16_MAX INT16_MAX
#define UINT_FAST16_MAX UINT16_MAX
#define INT_FAST32_MIN INT32_MIN
#define INT_FAST32_MAX INT32_MAX
#define UINT_FAST32_MAX UINT32_MAX

#define INTPTR_MIN (-2147483647L - 1L)
#define INTPTR_MAX 2147483647L
#define UINTPTR_MAX 4294967295UL

#define PTRDIFF_MIN INT32_MIN
#define PTRDIFF_MAX INT32_MAX
#define SIZE_MAX UINT32_MAX

#define INT8_C(value) value
#define UINT8_C(value) value
#define INT16_C(value) value
#define UINT16_C(value) value
#define INT32_C(value) value
#define UINT32_C(value) value##U

#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L)
typedef long long int64_t;
typedef unsigned long long uint64_t;
typedef int64_t int_least64_t;
typedef uint64_t uint_least64_t;
typedef int64_t int_fast64_t;
typedef uint64_t uint_fast64_t;
typedef int64_t intmax_t;
typedef uint64_t uintmax_t;

#define INT64_MIN (-9223372036854775807LL - 1LL)
#define INT64_MAX 9223372036854775807LL
#define UINT64_MAX 18446744073709551615ULL
#define INT_LEAST64_MIN INT64_MIN
#define INT_LEAST64_MAX INT64_MAX
#define UINT_LEAST64_MAX UINT64_MAX
#define INT_FAST64_MIN INT64_MIN
#define INT_FAST64_MAX INT64_MAX
#define UINT_FAST64_MAX UINT64_MAX
#define INTMAX_MIN INT64_MIN
#define INTMAX_MAX INT64_MAX
#define UINTMAX_MAX UINT64_MAX

#define INT64_C(value) value##LL
#define UINT64_C(value) value##ULL
#define INTMAX_C(value) value##LL
#define UINTMAX_C(value) value##ULL
#else
typedef long intmax_t;
typedef unsigned long uintmax_t;

#define INTMAX_MIN INTPTR_MIN
#define INTMAX_MAX INTPTR_MAX
#define UINTMAX_MAX UINTPTR_MAX

#define INTMAX_C(value) value##L
#define UINTMAX_C(value) value##UL
#endif

#endif
