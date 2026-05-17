#include "SolidSyslogNullMutex.h"

#include <stddef.h>

#include "SolidSyslogMutexDefinition.h"

static void NullMutex_Lock(struct SolidSyslogMutex* base);
static void NullMutex_Unlock(struct SolidSyslogMutex* base);

static struct SolidSyslogMutex NullMutex_Instance;

struct SolidSyslogMutex* SolidSyslogNullMutex_Create(void)
{
    NullMutex_Instance.Lock = NullMutex_Lock;
    NullMutex_Instance.Unlock = NullMutex_Unlock;
    return &NullMutex_Instance;
}

void SolidSyslogNullMutex_Destroy(void)
{
    NullMutex_Instance.Lock = NULL;
    NullMutex_Instance.Unlock = NULL;
}

static void NullMutex_Lock(struct SolidSyslogMutex* base)
{
    (void) base;
}

static void NullMutex_Unlock(struct SolidSyslogMutex* base)
{
    (void) base;
}
