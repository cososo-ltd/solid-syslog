/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  A Mutex over the VxWorks 6.4 mutual-exclusion semaphore, for thread-safe
 *  buffers and pools in a kernel (VIP) build. */
#ifndef SOLIDSYSLOGVXWORKS64MUTEX_H
#define SOLIDSYSLOGVXWORKS64MUTEX_H

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogMutex;

    struct SolidSyslogMutex* SolidSyslogVxWorks64Mutex_Create(void);
    void SolidSyslogVxWorks64Mutex_Destroy(struct SolidSyslogMutex * base);

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGVXWORKS64MUTEX_H */
