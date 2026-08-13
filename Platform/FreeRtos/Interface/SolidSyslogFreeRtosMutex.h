/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  A Mutex wrapping a statically-allocated FreeRTOS mutex semaphore, for
 *  thread-safe buffers and pools on a FreeRTOS target. Requires
 *  configSUPPORT_STATIC_ALLOCATION=1. */
#ifndef SOLIDSYSLOGFREERTOSMUTEX_H
#define SOLIDSYSLOGFREERTOSMUTEX_H

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogMutex;

    /** Create takes no config; an exhausted pool - or a build without
     *  configSUPPORT_STATIC_ALLOCATION=1, where xSemaphoreCreateMutexStatic
     *  yields no handle - falls back to the shared NullMutex, whose Lock and
     *  Unlock are no-ops. */
    struct SolidSyslogMutex* SolidSyslogFreeRtosMutex_Create(void);
    /** Release the pool slot; deletes the underlying FreeRTOS mutex semaphore. */
    void SolidSyslogFreeRtosMutex_Destroy(struct SolidSyslogMutex * base);

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGFREERTOSMUTEX_H */
