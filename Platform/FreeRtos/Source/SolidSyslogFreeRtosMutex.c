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

static void FreeRtosMutex_Lock(struct SolidSyslogMutex* self);
static void FreeRtosMutex_Unlock(struct SolidSyslogMutex* self);
static inline FreeRtosMutex* FreeRtosMutex_From(struct SolidSyslogMutex* self);
static inline SemaphoreHandle_t FreeRtosMutex_AsHandle(FreeRtosMutex* self);

struct SolidSyslogMutex* SolidSyslogFreeRtosMutex_Create(SolidSyslogFreeRtosMutexStorage* storage)
{
    FreeRtosMutex* mutex = (FreeRtosMutex*) storage;
    mutex->Base.Lock = FreeRtosMutex_Lock;
    mutex->Base.Unlock = FreeRtosMutex_Unlock;
    (void) xSemaphoreCreateMutexStatic(&mutex->Buffer);
    return &mutex->Base;
}

void SolidSyslogFreeRtosMutex_Destroy(struct SolidSyslogMutex* mutex)
{
    FreeRtosMutex* self = FreeRtosMutex_From(mutex);
    vSemaphoreDelete(FreeRtosMutex_AsHandle(self));
    self->Base.Lock = NULL;
    self->Base.Unlock = NULL;
}

static inline FreeRtosMutex* FreeRtosMutex_From(struct SolidSyslogMutex* self)
{
    return (FreeRtosMutex*) self;
}

static inline SemaphoreHandle_t FreeRtosMutex_AsHandle(FreeRtosMutex* self)
{
    return (SemaphoreHandle_t) &self->Buffer;
}

static void FreeRtosMutex_Lock(struct SolidSyslogMutex* self)
{
    (void) xSemaphoreTake(FreeRtosMutex_AsHandle(FreeRtosMutex_From(self)), portMAX_DELAY);
}

static void FreeRtosMutex_Unlock(struct SolidSyslogMutex* self)
{
    (void) xSemaphoreGive(FreeRtosMutex_AsHandle(FreeRtosMutex_From(self)));
}
