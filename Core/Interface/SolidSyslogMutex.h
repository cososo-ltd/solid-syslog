/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  The mutex role: mutual exclusion (Lock / Unlock) around buffer and pool
 *  critical sections. These calls dispatch to the injected mutex's vtable, so
 *  behaviour - including whether Lock blocks - is that mutex's. */
#ifndef SOLIDSYSLOGMUTEX_H
#define SOLIDSYSLOGMUTEX_H

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogMutex;

    /** Dispatch to the injected mutex's vtable; behaviour, including whether Lock
     *  blocks, is that mutex's (see SolidSyslogMutexDefinition.h). */
    void SolidSyslogMutex_Lock(struct SolidSyslogMutex * mutex);
    void SolidSyslogMutex_Unlock(struct SolidSyslogMutex * mutex);

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGMUTEX_H */
