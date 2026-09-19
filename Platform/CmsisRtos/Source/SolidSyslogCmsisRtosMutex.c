/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogCmsisRtosMutex.h"

#include <stdbool.h>
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

static inline osMutexAttr_t CmsisRtosMutex_AttributesFor(void* controlBlock, uint32_t controlBlockBytes);
static inline struct SolidSyslogCmsisRtosMutex* CmsisRtosMutex_SelfFromBase(struct SolidSyslogMutex* base);
static inline bool CmsisRtosMutex_HasRtosMutex(const struct SolidSyslogCmsisRtosMutex* self);

void SolidSyslogCmsisRtosMutex_Initialise(struct SolidSyslogMutex* base, void* controlBlock, uint32_t controlBlockBytes)
{
    osMutexAttr_t attributes = CmsisRtosMutex_AttributesFor(controlBlock, controlBlockBytes);
    struct SolidSyslogCmsisRtosMutex* self = CmsisRtosMutex_SelfFromBase(base);
    self->Id = osMutexNew(&attributes);
    if (CmsisRtosMutex_HasRtosMutex(self) == true)
    {
        self->Base.Lock = CmsisRtosMutex_Lock;
        self->Base.Unlock = CmsisRtosMutex_Unlock;
    }
    else
    {
        /* The control block was too small for the object this implementation
         * builds, a size only the integrator can know. Present the NullMutex
         * rather than a handle with nothing behind it, and say so. */
        *base = *SolidSyslogNullMutex_Get();
        CmsisRtosMutex_Report(
            SOLIDSYSLOG_BAD_CONFIG_FATAL_SEVERITY,
            SOLIDSYSLOG_CAT_BAD_CONFIG,
            SOLIDSYSLOG_MUTEX_ERROR_CREATE_FAILED
        );
    }
}

static inline osMutexAttr_t CmsisRtosMutex_AttributesFor(void* controlBlock, uint32_t controlBlockBytes)
{
    /* Priority inheritance is what stops a low-priority task holding the buffer
     * lock from being preempted indefinitely while a high-priority task waits
     * on it. An implementation whose mutexes always inherit ignores the bit;
     * one that does not needs asking.
     *
     * The control block is the caller's, unread here and passed straight
     * through: CMSIS-RTOS2 leaves its size to the implementation, so the
     * library cannot size one. NULL and zero together are the form CMSIS
     * defines for letting the implementation allocate instead. */
    osMutexAttr_t attributes = {NULL, osMutexPrioInherit, controlBlock, controlBlockBytes};
    return attributes;
}

static inline struct SolidSyslogCmsisRtosMutex* CmsisRtosMutex_SelfFromBase(struct SolidSyslogMutex* base)
{
    return (struct SolidSyslogCmsisRtosMutex*) base;
}

static inline bool CmsisRtosMutex_HasRtosMutex(const struct SolidSyslogCmsisRtosMutex* self)
{
    return self->Id != NULL;
}

void SolidSyslogCmsisRtosMutex_Cleanup(struct SolidSyslogMutex* base)
{
    struct SolidSyslogCmsisRtosMutex* self = CmsisRtosMutex_SelfFromBase(base);
    if (CmsisRtosMutex_HasRtosMutex(self) == true)
    {
        (void) osMutexDelete(self->Id);
    }
    /* Overwrite the abstract base with the shared NullMutex vtable so
     * use-after-destroy is a safe no-op rather than a NULL-fn-pointer crash. */
    *base = *SolidSyslogNullMutex_Get();
}

static void CmsisRtosMutex_Lock(struct SolidSyslogMutex* base)
{
    (void) osMutexAcquire(CmsisRtosMutex_SelfFromBase(base)->Id, osWaitForever);
}

static void CmsisRtosMutex_Unlock(struct SolidSyslogMutex* base)
{
    (void) osMutexRelease(CmsisRtosMutex_SelfFromBase(base)->Id);
}
