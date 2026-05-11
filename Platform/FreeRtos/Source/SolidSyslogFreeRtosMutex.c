#include "SolidSyslogFreeRtosMutex.h"

#include "FreeRTOS.h"
#include "semphr.h"

#include "SolidSyslogMacros.h"
#include "SolidSyslogMutexDefinition.h"

typedef struct SolidSyslogFreeRtosMutex FreeRtosMutex;

struct SolidSyslogFreeRtosMutex
{
    struct SolidSyslogMutex base;
    StaticSemaphore_t       buffer;
    SemaphoreHandle_t       handle;
};

SOLIDSYSLOG_STATIC_ASSERT(sizeof(FreeRtosMutex) <= SOLIDSYSLOG_FREERTOSMUTEX_SIZE,
                          "SOLIDSYSLOG_FREERTOSMUTEX_SIZE is too small for SolidSyslogFreeRtosMutex layout");

static void                  FreeRtosMutex_Lock(struct SolidSyslogMutex* self);
static void                  FreeRtosMutex_Unlock(struct SolidSyslogMutex* self);
static inline FreeRtosMutex* FreeRtosMutex_From(struct SolidSyslogMutex* self);

struct SolidSyslogMutex* SolidSyslogFreeRtosMutex_Create(SolidSyslogFreeRtosMutexStorage* storage)
{
    FreeRtosMutex* mutex = (FreeRtosMutex*) storage;
    mutex->base.Lock     = FreeRtosMutex_Lock;
    mutex->base.Unlock   = FreeRtosMutex_Unlock;
    mutex->handle        = xSemaphoreCreateMutexStatic(&mutex->buffer);
    return &mutex->base;
}

void SolidSyslogFreeRtosMutex_Destroy(struct SolidSyslogMutex* mutex)
{
    FreeRtosMutex* self = FreeRtosMutex_From(mutex);
    vSemaphoreDelete(self->handle);
    self->base.Lock   = NULL;
    self->base.Unlock = NULL;
    self->handle      = NULL;
}

static inline FreeRtosMutex* FreeRtosMutex_From(struct SolidSyslogMutex* self)
{
    return (FreeRtosMutex*) self;
}

static void FreeRtosMutex_Lock(struct SolidSyslogMutex* self)
{
    (void) xSemaphoreTake(FreeRtosMutex_From(self)->handle, portMAX_DELAY);
}

static void FreeRtosMutex_Unlock(struct SolidSyslogMutex* self)
{
    (void) xSemaphoreGive(FreeRtosMutex_From(self)->handle);
}
