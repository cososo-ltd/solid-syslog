#include "SolidSyslogNullMutex.h"

#include "SolidSyslogMutexDefinition.h"

static void NullMutex_Lock(struct SolidSyslogMutex* base);
static void NullMutex_Unlock(struct SolidSyslogMutex* base);

struct SolidSyslogMutex* SolidSyslogNullMutex_Get(void)
{
    static struct SolidSyslogMutex instance = {
        .Lock = NullMutex_Lock,
        .Unlock = NullMutex_Unlock,
    };
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
