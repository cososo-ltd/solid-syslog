/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogVxWorks64Mutex.h"

#include <stdbool.h>
#include <stddef.h>

#include "vxWorks.h"

#include "semLib.h"

#include "SolidSyslogError.h"
#include "SolidSyslogMutexDefinition.h"
#include "SolidSyslogNullMutex.h"
#include "SolidSyslogVxWorks64MutexPrivate.h"

const struct SolidSyslogErrorSource SolidSyslogVxWorks64MutexErrorSource = {"VxWorks64Mutex"};

static void VxWorks64Mutex_Lock(struct SolidSyslogMutex* base);
static void VxWorks64Mutex_Unlock(struct SolidSyslogMutex* base);

static inline struct SolidSyslogVxWorks64Mutex* VxWorks64Mutex_SelfFromBase(struct SolidSyslogMutex* base);
static inline bool VxWorks64Mutex_HasSemaphore(const struct SolidSyslogVxWorks64Mutex* self);

bool SolidSyslogVxWorks64Mutex_Initialise(struct SolidSyslogMutex* base)
{
    struct SolidSyslogVxWorks64Mutex* self = VxWorks64Mutex_SelfFromBase(base);
    self->Id = semMCreate(SEM_Q_PRIORITY | SEM_INVERSION_SAFE | SEM_DELETE_SAFE);
    bool created = VxWorks64Mutex_HasSemaphore(self);
    if (created == true)
    {
        self->Base.Lock = VxWorks64Mutex_Lock;
        self->Base.Unlock = VxWorks64Mutex_Unlock;
    }
    return created;
}

static inline struct SolidSyslogVxWorks64Mutex* VxWorks64Mutex_SelfFromBase(struct SolidSyslogMutex* base)
{
    return (struct SolidSyslogVxWorks64Mutex*) base;
}

static inline bool VxWorks64Mutex_HasSemaphore(const struct SolidSyslogVxWorks64Mutex* self)
{
    return self->Id != NULL;
}

void SolidSyslogVxWorks64Mutex_Cleanup(struct SolidSyslogMutex* base)
{
    struct SolidSyslogVxWorks64Mutex* self = VxWorks64Mutex_SelfFromBase(base);
    if (VxWorks64Mutex_HasSemaphore(self) == true)
    {
        (void) semDelete(self->Id);
    }
    /* Overwrite the abstract base with the shared NullMutex vtable so
     * use-after-destroy is a safe no-op rather than a call on a deleted
     * semaphore. */
    *base = *SolidSyslogNullMutex_Get();
}

static void VxWorks64Mutex_Lock(struct SolidSyslogMutex* base)
{
    (void) semTake(VxWorks64Mutex_SelfFromBase(base)->Id, WAIT_FOREVER);
}

static void VxWorks64Mutex_Unlock(struct SolidSyslogMutex* base)
{
    (void) semGive(VxWorks64Mutex_SelfFromBase(base)->Id);
}
