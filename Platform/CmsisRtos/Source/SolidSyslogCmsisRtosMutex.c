/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogCmsisRtosMutex.h"

#include <stddef.h>
#include <stdint.h>

#include "cmsis_os2.h"

#include "SolidSyslogCmsisRtosMutexPrivate.h"
#include "SolidSyslogError.h"
#include "SolidSyslogErrorCategory.h"
#include "SolidSyslogMutexDefinition.h"
#include "SolidSyslogNullMutex.h"

const struct SolidSyslogErrorSource SolidSyslogCmsisRtosMutexErrorSource = {"CmsisRtosMutex"};

static void CmsisRtosMutex_Lock(struct SolidSyslogMutex* base);
static void CmsisRtosMutex_Unlock(struct SolidSyslogMutex* base);

static inline struct SolidSyslogCmsisRtosMutex* CmsisRtosMutex_SelfFromBase(struct SolidSyslogMutex* base);

void SolidSyslogCmsisRtosMutex_Initialise(struct SolidSyslogMutex* base, void* controlBlock, uint32_t controlBlockBytes)
{
    /* Priority inheritance is what stops a low-priority task holding the
     * buffer lock from being preempted indefinitely while a high-priority task
     * waits on it. An implementation whose mutexes always inherit ignores the
     * bit; one that does not needs asking. */
    osMutexAttr_t attributes = {NULL, osMutexPrioInherit, controlBlock, controlBlockBytes};
    struct SolidSyslogCmsisRtosMutex* self = CmsisRtosMutex_SelfFromBase(base);
    self->Id = osMutexNew(&attributes);
    if (self->Id != NULL)
    {
        self->Base.Lock = CmsisRtosMutex_Lock;
        self->Base.Unlock = CmsisRtosMutex_Unlock;
    }
    else
    {
        *base = *SolidSyslogNullMutex_Get();
        CmsisRtosMutex_Report(
            SOLIDSYSLOG_BAD_CONFIG_FATAL_SEVERITY,
            SOLIDSYSLOG_CAT_BAD_CONFIG,
            SOLIDSYSLOG_MUTEX_ERROR_CREATE_FAILED
        );
    }
}

static inline struct SolidSyslogCmsisRtosMutex* CmsisRtosMutex_SelfFromBase(struct SolidSyslogMutex* base)
{
    return (struct SolidSyslogCmsisRtosMutex*) base;
}

static void CmsisRtosMutex_Lock(struct SolidSyslogMutex* base)
{
    (void) osMutexAcquire(CmsisRtosMutex_SelfFromBase(base)->Id, osWaitForever);
}

static void CmsisRtosMutex_Unlock(struct SolidSyslogMutex* base)
{
    (void) osMutexRelease(CmsisRtosMutex_SelfFromBase(base)->Id);
}

void SolidSyslogCmsisRtosMutex_Cleanup(struct SolidSyslogMutex* base)
{
    struct SolidSyslogCmsisRtosMutex* self = CmsisRtosMutex_SelfFromBase(base);
    if (self->Id != NULL)
    {
        (void) osMutexDelete(self->Id);
    }
    /* Overwrite the abstract base with the shared NullMutex vtable so
     * use-after-destroy is a safe no-op rather than a NULL-fn-pointer crash. */
    *base = *SolidSyslogNullMutex_Get();
}
