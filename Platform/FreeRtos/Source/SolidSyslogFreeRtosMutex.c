#include "SolidSyslogFreeRtosMutex.h"

#include "FreeRTOS.h"
#include "semphr.h"

#include "SolidSyslogMacros.h"
#include "SolidSyslogMutexDefinition.h"

typedef struct SolidSyslogFreeRtosMutex FreeRtosMutex;

/* xSemaphoreCreateMutexStatic returns a handle that is the same pointer
 * as the StaticSemaphore_t passed in, so the per-instance struct doesn't
 * carry a separate SemaphoreHandle_t — the kernel-primitive layout matches
 * the Posix (pthread_mutex_t) and Windows (CRITICAL_SECTION) adapters. */
struct SolidSyslogFreeRtosMutex
{
    struct SolidSyslogMutex Base;
    StaticSemaphore_t Buffer;
};

SOLIDSYSLOG_STATIC_ASSERT(
    sizeof(FreeRtosMutex) <= SOLIDSYSLOG_FREERTOSMUTEX_SIZE,
    "SOLIDSYSLOG_FREERTOSMUTEX_SIZE is too small for SolidSyslogFreeRtosMutex layout"
);

static void FreeRtosMutex_Lock(struct SolidSyslogMutex* base);
static void FreeRtosMutex_Unlock(struct SolidSyslogMutex* base);

static inline FreeRtosMutex* FreeRtosMutex_SelfFromStorage(SolidSyslogFreeRtosMutexStorage* storage);
static inline FreeRtosMutex* FreeRtosMutex_SelfFromBase(struct SolidSyslogMutex* base);
static inline SemaphoreHandle_t FreeRtosMutex_AsHandle(FreeRtosMutex* self);

struct SolidSyslogMutex* SolidSyslogFreeRtosMutex_Create(SolidSyslogFreeRtosMutexStorage* storage)
{
    FreeRtosMutex* self = FreeRtosMutex_SelfFromStorage(storage);
    self->Base.Lock = FreeRtosMutex_Lock;
    self->Base.Unlock = FreeRtosMutex_Unlock;
    (void) xSemaphoreCreateMutexStatic(&self->Buffer);
    return &self->Base;
}

static inline FreeRtosMutex* FreeRtosMutex_SelfFromStorage(SolidSyslogFreeRtosMutexStorage* storage)
{
    return (FreeRtosMutex*) storage;
}

void SolidSyslogFreeRtosMutex_Destroy(struct SolidSyslogMutex* base)
{
    FreeRtosMutex* self = FreeRtosMutex_SelfFromBase(base);
    vSemaphoreDelete(FreeRtosMutex_AsHandle(self));
    self->Base.Lock = NULL;
    self->Base.Unlock = NULL;
}

static inline FreeRtosMutex* FreeRtosMutex_SelfFromBase(struct SolidSyslogMutex* base)
{
    return (FreeRtosMutex*) base;
}

static inline SemaphoreHandle_t FreeRtosMutex_AsHandle(FreeRtosMutex* self)
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
