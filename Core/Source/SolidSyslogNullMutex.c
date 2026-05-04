#include "SolidSyslogNullMutex.h"

#include <stddef.h>

#include "SolidSyslogMutexDefinition.h"

static void Lock(struct SolidSyslogMutex* self);
static void Unlock(struct SolidSyslogMutex* self);

static struct SolidSyslogMutex instance;

struct SolidSyslogMutex* SolidSyslogNullMutex_Create(void)
{
    instance.Lock   = Lock;
    instance.Unlock = Unlock;
    return &instance;
}

void SolidSyslogNullMutex_Destroy(void)
{
    instance.Lock   = NULL;
    instance.Unlock = NULL;
}

static void Lock(struct SolidSyslogMutex* self)
{
    (void) self;
}

static void Unlock(struct SolidSyslogMutex* self)
{
    (void) self;
}
