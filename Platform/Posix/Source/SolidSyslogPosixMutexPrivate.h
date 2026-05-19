#ifndef SOLIDSYSLOGPOSIXMUTEXPRIVATE_H
#define SOLIDSYSLOGPOSIXMUTEXPRIVATE_H

#include <pthread.h>

#include "SolidSyslogMutexDefinition.h"

struct SolidSyslogPosixMutex
{
    struct SolidSyslogMutex Base;
    pthread_mutex_t Mutex;
};

void PosixMutex_Initialise(struct SolidSyslogMutex* base);
void PosixMutex_Cleanup(struct SolidSyslogMutex* base);

#endif /* SOLIDSYSLOGPOSIXMUTEXPRIVATE_H */
