#include "SolidSyslogWindowsMutex.h"

#include <stddef.h>
#include <windows.h>

#include "SolidSyslogMacros.h"
#include "SolidSyslogMutexDefinition.h"

struct SolidSyslogWindowsMutex
{
    struct SolidSyslogMutex base;
    CRITICAL_SECTION section;
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
    windows->base.Lock = WindowsMutex_Lock;
    windows->base.Unlock = WindowsMutex_Unlock;
    InitializeCriticalSection(&windows->section);
    return &windows->base;
}

void SolidSyslogWindowsMutex_Destroy(struct SolidSyslogMutex* mutex)
{
    struct SolidSyslogWindowsMutex* windows = (struct SolidSyslogWindowsMutex*) mutex;
    DeleteCriticalSection(&windows->section);
    windows->base.Lock = NULL;
    windows->base.Unlock = NULL;
}

static void WindowsMutex_Lock(struct SolidSyslogMutex* self)
{
    struct SolidSyslogWindowsMutex* windows = (struct SolidSyslogWindowsMutex*) self;
    EnterCriticalSection(&windows->section);
}

static void WindowsMutex_Unlock(struct SolidSyslogMutex* self)
{
    struct SolidSyslogWindowsMutex* windows = (struct SolidSyslogWindowsMutex*) self;
    LeaveCriticalSection(&windows->section);
}
