#include "SolidSyslogNullMutex.h"

#include <stddef.h>

#include "SolidSyslogMutexDefinition.h"

static void NullMutex_Lock(struct SolidSyslogMutex* base);
static void NullMutex_Unlock(struct SolidSyslogMutex* base);

static struct SolidSyslogMutex instance;

struct SolidSyslogMutex* SolidSyslogNullMutex_Create(void)
{
    instance.Lock = NullMutex_Lock;
    instance.Unlock = NullMutex_Unlock;
    return &instance;
}

void SolidSyslogNullMutex_Destroy(void)
{
    instance.Lock = NULL;
    instance.Unlock = NULL;
}

static void NullMutex_Lock(struct SolidSyslogMutex* base)
{
    (void) base;
}

static void NullMutex_Unlock(struct SolidSyslogMutex* base)
{
    (void) base;
}
