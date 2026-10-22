/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  A Mutex wrapping a statically-allocated FreeRTOS mutex semaphore, for
 *  buffers shared between tasks on a FreeRTOS target. Requires
 *  configSUPPORT_STATIC_ALLOCATION=1 and configUSE_MUTEXES=1. */
#ifndef SOLIDSYSLOGFREERTOSMUTEX_H
#define SOLIDSYSLOGFREERTOSMUTEX_H

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogMutex;

    /** Create takes no config; an exhausted pool falls back to the shared
     *  NullMutex, whose Lock and Unlock are no-ops. Without
     *  configSUPPORT_STATIC_ALLOCATION=1 and configUSE_MUTEXES=1 the adapter
     *  compiles to nothing, so the build fails to link rather than falling back. */
    struct SolidSyslogMutex* SolidSyslogFreeRtosMutex_Create(void);
    /** Release the pool slot; deletes the underlying FreeRTOS mutex semaphore. */
    void SolidSyslogFreeRtosMutex_Destroy(struct SolidSyslogMutex * base);

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGFREERTOSMUTEX_H */
