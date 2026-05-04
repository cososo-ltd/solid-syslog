#include "SolidSyslogMutex.h"

#include "SolidSyslogMutexDefinition.h"

void SolidSyslogMutex_Lock(struct SolidSyslogMutex* mutex)
{
    mutex->Lock(mutex);
}

void SolidSyslogMutex_Unlock(struct SolidSyslogMutex* mutex)
{
    mutex->Unlock(mutex);
}
