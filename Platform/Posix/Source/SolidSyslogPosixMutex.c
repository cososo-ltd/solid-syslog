#include "SolidSyslogPosixMutex.h"

#include <pthread.h>
#include <stddef.h>

#include "SolidSyslogMacros.h"
#include "SolidSyslogMutexDefinition.h"

struct SolidSyslogPosixMutex
{
    struct SolidSyslogMutex Base;
    pthread_mutex_t Mutex;
};

SOLIDSYSLOG_STATIC_ASSERT(
    sizeof(struct SolidSyslogPosixMutex) <= SOLIDSYSLOG_POSIXMUTEX_SIZE,
    "SOLIDSYSLOG_POSIXMUTEX_SIZE is too small for SolidSyslogPosixMutex layout"
);

static void PosixMutex_Lock(struct SolidSyslogMutex* base);
static void PosixMutex_Unlock(struct SolidSyslogMutex* base);

static inline struct SolidSyslogPosixMutex* PosixMutex_SelfFromStorage(SolidSyslogPosixMutexStorage* storage);
static inline struct SolidSyslogPosixMutex* PosixMutex_SelfFromBase(struct SolidSyslogMutex* base);

struct SolidSyslogMutex* SolidSyslogPosixMutex_Create(SolidSyslogPosixMutexStorage* storage)
{
    struct SolidSyslogPosixMutex* self = PosixMutex_SelfFromStorage(storage);
    self->Base.Lock = PosixMutex_Lock;
    self->Base.Unlock = PosixMutex_Unlock;
    pthread_mutex_init(&self->Mutex, NULL);
    return &self->Base;
}

static inline struct SolidSyslogPosixMutex* PosixMutex_SelfFromStorage(SolidSyslogPosixMutexStorage* storage)
{
    return (struct SolidSyslogPosixMutex*) storage;
}

void SolidSyslogPosixMutex_Destroy(struct SolidSyslogMutex* base)
{
    struct SolidSyslogPosixMutex* self = PosixMutex_SelfFromBase(base);
    pthread_mutex_destroy(&self->Mutex);
    self->Base.Lock = NULL;
    self->Base.Unlock = NULL;
}

static inline struct SolidSyslogPosixMutex* PosixMutex_SelfFromBase(struct SolidSyslogMutex* base)
{
    return (struct SolidSyslogPosixMutex*) base;
}

static void PosixMutex_Lock(struct SolidSyslogMutex* base)
{
    struct SolidSyslogPosixMutex* self = PosixMutex_SelfFromBase(base);
    pthread_mutex_lock(&self->Mutex);
}

static void PosixMutex_Unlock(struct SolidSyslogMutex* base)
{
    struct SolidSyslogPosixMutex* self = PosixMutex_SelfFromBase(base);
    pthread_mutex_unlock(&self->Mutex);
}
