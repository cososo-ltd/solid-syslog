#include "SolidSyslogFreeRtosMutex.h"

#include <stddef.h>

#include "FreeRTOS.h"
#include "semphr.h"

#include "SolidSyslogFreeRtosMutexPrivate.h"
#include "SolidSyslogMutexDefinition.h"
#include "SolidSyslogNullMutex.h"

static void FreeRtosMutex_Lock(struct SolidSyslogMutex* base);
static void FreeRtosMutex_Unlock(struct SolidSyslogMutex* base);

static inline struct SolidSyslogFreeRtosMutex* FreeRtosMutex_SelfFromBase(struct SolidSyslogMutex* base);
static inline SemaphoreHandle_t FreeRtosMutex_AsHandle(struct SolidSyslogFreeRtosMutex* self);

void FreeRtosMutex_Initialise(struct SolidSyslogMutex* base)
{
    struct SolidSyslogFreeRtosMutex* self = FreeRtosMutex_SelfFromBase(base);
    /* xSemaphoreCreateMutexStatic returns NULL only when
     * configSUPPORT_STATIC_ALLOCATION is not 1 — a compile-time config
     * gate, not a runtime failure mode. Guarded anyway so a misconfigured
     * integrator falls back to the NullMutex vtable instead of corrupting
     * Lock/Unlock with a dangling handle, mirroring PosixMutex's defence
     * against pthread_mutex_init failure. */
    if (xSemaphoreCreateMutexStatic(&self->Buffer) != NULL)
    {
        self->Base.Lock = FreeRtosMutex_Lock;
        self->Base.Unlock = FreeRtosMutex_Unlock;
    }
    else
    {
        *base = *SolidSyslogNullMutex_Get();
    }
}

static inline struct SolidSyslogFreeRtosMutex* FreeRtosMutex_SelfFromBase(struct SolidSyslogMutex* base)
{
    return (struct SolidSyslogFreeRtosMutex*) base;
}

void FreeRtosMutex_Cleanup(struct SolidSyslogMutex* base)
{
    struct SolidSyslogFreeRtosMutex* self = FreeRtosMutex_SelfFromBase(base);
    if (self->Base.Lock == FreeRtosMutex_Lock)
    {
        vSemaphoreDelete(FreeRtosMutex_AsHandle(self));
    }
    /* Overwrite the abstract base with the shared NullMutex vtable so
     * use-after-destroy is a safe no-op rather than a NULL-fn-pointer crash. */
    *base = *SolidSyslogNullMutex_Get();
}

static inline SemaphoreHandle_t FreeRtosMutex_AsHandle(struct SolidSyslogFreeRtosMutex* self)
{
    return (SemaphoreHandle_t) &self->Buffer;
}

static void FreeRtosMutex_Lock(struct SolidSyslogMutex* base)
{
    (void) xSemaphoreTake(FreeRtosMutex_AsHandle(FreeRtosMutex_SelfFromBase(base)), portMAX_DELAY);
}

static void FreeRtosMutex_Unlock(struct SolidSyslogMutex* base)
{
    (void) xSemaphoreGive(FreeRtosMutex_AsHandle(FreeRtosMutex_SelfFromBase(base)));
}
