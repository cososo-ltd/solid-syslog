#include "SolidSyslogWindowsMutex.h"

#include <stddef.h>
#include <windows.h>

#include "SolidSyslogMacros.h"
#include "SolidSyslogMutexDefinition.h"

struct SolidSyslogWindowsMutex
{
    struct SolidSyslogMutex Base;
    CRITICAL_SECTION Section;
};

SOLIDSYSLOG_STATIC_ASSERT(
    sizeof(struct SolidSyslogWindowsMutex) <= SOLIDSYSLOG_WINDOWSMUTEX_SIZE,
    "SOLIDSYSLOG_WINDOWSMUTEX_SIZE is too small for SolidSyslogWindowsMutex layout"
);

static void WindowsMutex_Lock(struct SolidSyslogMutex* self);
static void WindowsMutex_Unlock(struct SolidSyslogMutex* self);

struct SolidSyslogMutex* SolidSyslogWindowsMutex_Create(SolidSyslogWindowsMutexStorage* storage)
{
    struct SolidSyslogWindowsMutex* windows = (struct SolidSyslogWindowsMutex*) storage;
    windows->Base.Lock = WindowsMutex_Lock;
    windows->Base.Unlock = WindowsMutex_Unlock;
    InitializeCriticalSection(&windows->Section);
    return &windows->Base;
}

void SolidSyslogWindowsMutex_Destroy(struct SolidSyslogMutex* mutex)
{
    struct SolidSyslogWindowsMutex* windows = (struct SolidSyslogWindowsMutex*) mutex;
    DeleteCriticalSection(&windows->Section);
    windows->Base.Lock = NULL;
    windows->Base.Unlock = NULL;
}

static void WindowsMutex_Lock(struct SolidSyslogMutex* self)
{
    struct SolidSyslogWindowsMutex* windows = (struct SolidSyslogWindowsMutex*) self;
    EnterCriticalSection(&windows->Section);
}

static void WindowsMutex_Unlock(struct SolidSyslogMutex* self)
{
    struct SolidSyslogWindowsMutex* windows = (struct SolidSyslogWindowsMutex*) self;
    LeaveCriticalSection(&windows->Section);
}
