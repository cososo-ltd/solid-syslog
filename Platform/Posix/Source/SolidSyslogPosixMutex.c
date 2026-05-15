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

static void PosixMutex_Lock(struct SolidSyslogMutex* self);
static void PosixMutex_Unlock(struct SolidSyslogMutex* self);

struct SolidSyslogMutex* SolidSyslogPosixMutex_Create(SolidSyslogPosixMutexStorage* storage)
{
    struct SolidSyslogPosixMutex* posix = (struct SolidSyslogPosixMutex*) storage;
    posix->Base.Lock = PosixMutex_Lock;
    posix->Base.Unlock = PosixMutex_Unlock;
    pthread_mutex_init(&posix->Mutex, NULL);
    return &posix->Base;
}

void SolidSyslogPosixMutex_Destroy(struct SolidSyslogMutex* mutex)
{
    struct SolidSyslogPosixMutex* posix = (struct SolidSyslogPosixMutex*) mutex;
    pthread_mutex_destroy(&posix->Mutex);
    posix->Base.Lock = NULL;
    posix->Base.Unlock = NULL;
}

static void PosixMutex_Lock(struct SolidSyslogMutex* self)
{
    struct SolidSyslogPosixMutex* posix = (struct SolidSyslogPosixMutex*) self;
    pthread_mutex_lock(&posix->Mutex);
}

static void PosixMutex_Unlock(struct SolidSyslogMutex* self)
{
    struct SolidSyslogPosixMutex* posix = (struct SolidSyslogPosixMutex*) self;
    pthread_mutex_unlock(&posix->Mutex);
}
