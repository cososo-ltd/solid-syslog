#include "SolidSyslogPosixMutex.h"

#include <pthread.h>
#include <stddef.h>

#include "SolidSyslogMutexDefinition.h"
#include "SolidSyslogNullMutex.h"
#include "SolidSyslogPosixMutexPrivate.h"

static void PosixMutex_Lock(struct SolidSyslogMutex* base);
static void PosixMutex_Unlock(struct SolidSyslogMutex* base);

static inline struct SolidSyslogPosixMutex* PosixMutex_SelfFromBase(struct SolidSyslogMutex* base);

void PosixMutex_Initialise(struct SolidSyslogMutex* base)
{
    struct SolidSyslogPosixMutex* self = PosixMutex_SelfFromBase(base);
    self->Base.Lock = PosixMutex_Lock;
    self->Base.Unlock = PosixMutex_Unlock;
    pthread_mutex_init(&self->Mutex, NULL);
}

void PosixMutex_Cleanup(struct SolidSyslogMutex* base)
{
    struct SolidSyslogPosixMutex* self = PosixMutex_SelfFromBase(base);
    pthread_mutex_destroy(&self->Mutex);
    /* Overwrite the abstract base with the shared NullMutex vtable so
     * use-after-destroy is a safe no-op rather than a NULL-fn-pointer crash. */
    *base = *SolidSyslogNullMutex_Get();
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
