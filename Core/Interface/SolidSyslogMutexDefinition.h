/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  The Mutex vtable (Lock / Unlock) - the contract an implementor fills in (the
 *  Mutex extension point). */
#ifndef SOLIDSYSLOGMUTEXDEFINITION_H
#define SOLIDSYSLOGMUTEXDEFINITION_H

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    /** The one synchronisation seam a CircularBuffer needs: it guards the ring's
     *  producer (Log/Write) against its consumer (Service/Read). Core takes the
     *  lock only around each buffer operation and never nests it, so a plain
     *  non-recursive mutex suffices; Lock may block until the holder releases.
     *  Single-task builds inject SolidSyslogNullMutex, whose Lock/Unlock are
     *  no-ops. */
    struct SolidSyslogMutex
    {
        void (*Lock)(struct SolidSyslogMutex* base);
        void (*Unlock)(struct SolidSyslogMutex* base);
    };

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGMUTEXDEFINITION_H */
