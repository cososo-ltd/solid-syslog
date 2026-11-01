/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  The shared scratch: SolidSyslog_Log formats every message in one static
 *  buffer of SOLIDSYSLOG_MAX_MESSAGE_SIZE, taken under the injected mutex, so
 *  the buffer costs RAM once rather than once on the stack of every task that
 *  logs. A task that logs blocks until the buffer is free.
 *
 *  Formatting, the header-field callbacks, any structured-data callbacks and
 *  the buffer write all run while the mutex is held. The mutex is therefore
 *  not reentrant: a callback or error handler that calls SolidSyslog_Log on
 *  the same logger deadlocks, and it must not be the mutex the Buffer takes,
 *  because that one is taken inside it. */
#ifndef SOLIDSYSLOGSHAREDSCRATCH_H
#define SOLIDSYSLOGSHAREDSCRATCH_H

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogMutex;
    struct SolidSyslogScratch;

    /** Take a pool slot holding one message buffer, guarded by @p mutex, which
     *  must outlive it. The pool holds SOLIDSYSLOG_POOL_SIZE: one per logger the
     *  build can hold. The mutex is one of its own, beside any the Buffer
     *  takes, so SOLIDSYSLOG_MUTEX_POOL_SIZE must hold both. A NULL mutex, or
     *  an exhausted pool, is reported and the stack scratch is returned
     *  instead, so logging continues but on the stack of each task that logs. */
    struct SolidSyslogScratch* SolidSyslogSharedScratch_Create(struct SolidSyslogMutex * mutex);
    /** Release the pool slot; does not destroy the injected mutex. */
    void SolidSyslogSharedScratch_Destroy(struct SolidSyslogScratch * base);

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGSHAREDSCRATCH_H */
