/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  The C-linkage guard macros (SOLIDSYSLOG_EXTERN_C_BEGIN /
 *  SOLIDSYSLOG_EXTERN_C_END) that wrap every public header so a C++ consumer
 *  links the declarations with C linkage. */
#ifndef SOLIDSYSLOGEXTERNC_H
#define SOLIDSYSLOGEXTERNC_H

#ifdef __cplusplus
#define SOLIDSYSLOG_EXTERN_C_BEGIN \
    extern "C"                     \
    {
#define SOLIDSYSLOG_EXTERN_C_END }
#else
#define SOLIDSYSLOG_EXTERN_C_BEGIN
#define SOLIDSYSLOG_EXTERN_C_END
#endif

#endif /* SOLIDSYSLOGEXTERNC_H */
