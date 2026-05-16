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

static void WindowsMutex_Lock(struct SolidSyslogMutex* base);
static void WindowsMutex_Unlock(struct SolidSyslogMutex* base);

static inline struct SolidSyslogWindowsMutex* WindowsMutex_SelfFromStorage(SolidSyslogWindowsMutexStorage* storage);
static inline struct SolidSyslogWindowsMutex* WindowsMutex_SelfFromBase(struct SolidSyslogMutex* base);

struct SolidSyslogMutex* SolidSyslogWindowsMutex_Create(SolidSyslogWindowsMutexStorage* storage)
{
    struct SolidSyslogWindowsMutex* self = WindowsMutex_SelfFromStorage(storage);
    self->Base.Lock = WindowsMutex_Lock;
    self->Base.Unlock = WindowsMutex_Unlock;
    InitializeCriticalSection(&self->Section);
    return &self->Base;
}

static inline struct SolidSyslogWindowsMutex* WindowsMutex_SelfFromStorage(SolidSyslogWindowsMutexStorage* storage)
{
    return (struct SolidSyslogWindowsMutex*) storage;
}

void SolidSyslogWindowsMutex_Destroy(struct SolidSyslogMutex* base)
{
    struct SolidSyslogWindowsMutex* self = WindowsMutex_SelfFromBase(base);
    DeleteCriticalSection(&self->Section);
    self->Base.Lock = NULL;
    self->Base.Unlock = NULL;
}

static inline struct SolidSyslogWindowsMutex* WindowsMutex_SelfFromBase(struct SolidSyslogMutex* base)
{
    return (struct SolidSyslogWindowsMutex*) base;
}

static void WindowsMutex_Lock(struct SolidSyslogMutex* base)
{
    struct SolidSyslogWindowsMutex* self = WindowsMutex_SelfFromBase(base);
    EnterCriticalSection(&self->Section);
}

static void WindowsMutex_Unlock(struct SolidSyslogMutex* base)
{
    struct SolidSyslogWindowsMutex* self = WindowsMutex_SelfFromBase(base);
    LeaveCriticalSection(&self->Section);
}
