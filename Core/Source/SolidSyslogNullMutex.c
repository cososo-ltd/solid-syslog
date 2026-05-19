#include "SolidSyslogNullMutex.h"

#include "SolidSyslogMutexDefinition.h"

static void NullMutex_Lock(struct SolidSyslogMutex* base);
static void NullMutex_Unlock(struct SolidSyslogMutex* base);

static struct SolidSyslogMutex instance = {
    .Lock = NullMutex_Lock,
    .Unlock = NullMutex_Unlock,
};

struct SolidSyslogMutex* SolidSyslogNullMutex_Get(void)
{
    return &instance;
}

static void NullMutex_Lock(struct SolidSyslogMutex* base)
{
    (void) base;
}

static void NullMutex_Unlock(struct SolidSyslogMutex* base)
{
    (void) base;
}
