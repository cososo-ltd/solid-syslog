/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  A Mutex over the CMSIS-RTOS2 API, for thread-safe buffers and pools on any
 *  kernel that implements it. */
#ifndef SOLIDSYSLOGCMSISRTOSMUTEX_H
#define SOLIDSYSLOGCMSISRTOSMUTEX_H

#include <stdint.h>

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogMutex;

    /** Create a mutex in the control block you supply.
     *
     *  CMSIS-RTOS2 does not standardise how large a mutex control block is -
     *  each implementation decides - so the library cannot size one for you and
     *  does not try. Pass storage of whatever your implementation needs, which
     *  must outlive the mutex; it is the RTOS object itself, not a copy.
     *
     *  Pass NULL and zero to let the implementation allocate the control block
     *  from its own pool instead, which is the form CMSIS-RTOS2 defines for
     *  that. An implementation configured for static allocation only will
     *  refuse it.
     *
     *  An exhausted pool, and an implementation that refuses the control block,
     *  both fall back to the shared NullMutex, whose Lock and Unlock are
     *  no-ops. Both are reported through the error handler.
     *
     *  @param controlBlock      storage for the RTOS mutex object, or NULL.
     *  @param controlBlockBytes its size in bytes, or zero alongside NULL. */
    struct SolidSyslogMutex* SolidSyslogCmsisRtosMutex_Create(void* controlBlock, uint32_t controlBlockBytes);
    /** Release the pool slot; deletes the underlying CMSIS-RTOS2 mutex. The
     *  control block is yours again once this returns. */
    void SolidSyslogCmsisRtosMutex_Destroy(struct SolidSyslogMutex * base);

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGCMSISRTOSMUTEX_H */
