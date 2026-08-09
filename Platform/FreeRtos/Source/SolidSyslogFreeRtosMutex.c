#include "FreeRTOS.h"

/* This component requires FreeRTOS built with static allocation and mutexes. */
#if (configSUPPORT_STATIC_ALLOCATION == 1) && (configUSE_MUTEXES == 1)

#include "SolidSyslogFreeRtosMutex.h"

#include <stddef.h>

#include "semphr.h"

#include "SolidSyslogError.h"
#include "SolidSyslogFreeRtosMutexErrors.h"
#include "SolidSyslogFreeRtosMutexPrivate.h"
#include "SolidSyslogMutexDefinition.h"
#include "SolidSyslogNullMutex.h"

const struct SolidSyslogErrorSource FreeRtosMutexErrorSource = {"FreeRtosMutex"};

static void FreeRtosMutex_Lock(struct SolidSyslogMutex* base);
static void FreeRtosMutex_Unlock(struct SolidSyslogMutex* base);

static inline struct SolidSyslogFreeRtosMutex* FreeRtosMutex_SelfFromBase(struct SolidSyslogMutex* base);
static inline SemaphoreHandle_t FreeRtosMutex_AsHandle(struct SolidSyslogFreeRtosMutex* self);

void FreeRtosMutex_Initialise(struct SolidSyslogMutex* base)
{
    struct SolidSyslogFreeRtosMutex* self = FreeRtosMutex_SelfFromBase(base);
    /* The storage is ours, so this cannot fail for want of memory; the kernel
     * returns NULL only when handed a NULL buffer, which this call never does.
     * configSUPPORT_STATIC_ALLOCATION is a compile-time requirement rather than
     * a runtime one — without it the function does not exist to call. The
     * branch is therefore defensive: an unexpected NULL leaves the NullMutex
     * vtable in place rather than a dangling handle in Lock/Unlock. */
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

#else

/* ISO C forbids an empty translation unit. */
typedef int FreeRtosMutex_EmptyTranslationUnit;

#endif /* configSUPPORT_STATIC_ALLOCATION && configUSE_MUTEXES */
