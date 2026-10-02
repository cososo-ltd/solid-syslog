/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  The C99 <stdbool.h> for VxWorks 6.4 kernel builds, whose header tree has
 *  none. solidsyslog-vxworks64.mk puts this directory on the library's include
 *  path; code that includes SolidSyslog headers needs it on its own.
 *
 *  Compiled as C99 - the library always is - bool is _Bool. Compiled as C89,
 *  as a kernel project's own sources usually are, bool is unsigned char: the
 *  same size and the same 0 and 1, so a C89 caller and the C99 library agree
 *  on every bool they pass between them. Unlike _Bool, unsigned char does not
 *  turn other values into 1, so C89 code must give a bool only true, false or
 *  the result of a comparison. */

#ifndef SOLIDSYSLOG_VXWORKS64_COMPAT_STDBOOL_H
#define SOLIDSYSLOG_VXWORKS64_COMPAT_STDBOOL_H

#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L)
#define bool _Bool
#else
#define bool unsigned char
#endif

#define true 1
#define false 0
#define __bool_true_false_are_defined 1

#endif
