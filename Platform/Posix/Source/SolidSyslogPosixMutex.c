#include "SolidSyslogPosixMutex.h"

#include <pthread.h>
#include <stddef.h>

#include "SolidSyslogMacros.h"
#include "SolidSyslogMutexDefinition.h"

struct SolidSyslogPosixMutex
{
    struct SolidSyslogMutex base;
    pthread_mutex_t mutex;
};

SOLIDSYSLOG_STATIC_ASSERT(
    sizeof(struct SolidSyslogPosixMutex) <= SOLIDSYSLOG_POSIXMUTEX_SIZE,
    "SOLIDSYSLOG_POSIXMUTEX_SIZE is too small for SolidSyslogPosixMutex layout"
);

static void PosixMutex_Lock(struct SolidSyslogMutex* self);
static void PosixMutex_Unlock(struct SolidSyslogMutex* self);

struct SolidSyslogMutex* SolidSyslogPosixMutex_Create(SolidSyslogPosixMutexStorage* storage)
{
    struct SolidSyslogPosixMutex* posix = (struct SolidSyslogPosixMutex*) storage;
    posix->base.Lock = PosixMutex_Lock;
    posix->base.Unlock = PosixMutex_Unlock;
    pthread_mutex_init(&posix->mutex, NULL);
    return &posix->base;
}

void SolidSyslogPosixMutex_Destroy(struct SolidSyslogMutex* mutex)
{
    struct SolidSyslogPosixMutex* posix = (struct SolidSyslogPosixMutex*) mutex;
    pthread_mutex_destroy(&posix->mutex);
    posix->base.Lock = NULL;
    posix->base.Unlock = NULL;
}

static void PosixMutex_Lock(struct SolidSyslogMutex* self)
{
    struct SolidSyslogPosixMutex* posix = (struct SolidSyslogPosixMutex*) self;
    pthread_mutex_lock(&posix->mutex);
}

static void PosixMutex_Unlock(struct SolidSyslogMutex* self)
{
    struct SolidSyslogPosixMutex* posix = (struct SolidSyslogPosixMutex*) self;
    pthread_mutex_unlock(&posix->mutex);
}
