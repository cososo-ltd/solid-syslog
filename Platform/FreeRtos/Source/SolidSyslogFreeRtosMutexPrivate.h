#ifndef SOLIDSYSLOGFREERTOSMUTEXPRIVATE_H
#define SOLIDSYSLOGFREERTOSMUTEXPRIVATE_H

#include "FreeRTOS.h"
#include "semphr.h"

#include "SolidSyslogMutexDefinition.h"

/* xSemaphoreCreateMutexStatic returns a handle that is the same pointer
 * as the StaticSemaphore_t passed in, so the per-instance struct doesn't
 * carry a separate SemaphoreHandle_t — the kernel-primitive layout matches
 * the Posix (pthread_mutex_t) and Windows (CRITICAL_SECTION) adapters. */
struct SolidSyslogFreeRtosMutex
{
    struct SolidSyslogMutex Base;
    StaticSemaphore_t Buffer;
};

void FreeRtosMutex_Initialise(struct SolidSyslogMutex* base);
void FreeRtosMutex_Cleanup(struct SolidSyslogMutex* base);

#endif /* SOLIDSYSLOGFREERTOSMUTEXPRIVATE_H */
