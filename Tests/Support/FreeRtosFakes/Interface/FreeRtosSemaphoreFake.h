#ifndef FREERTOSSEMAPHOREFAKE_H
#define FREERTOSSEMAPHOREFAKE_H

#include "SolidSyslogExternC.h"
#include "FreeRTOS.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    void FreeRtosSemaphoreFake_Reset(void);

    unsigned FreeRtosSemaphoreFake_CreateMutexStaticCallCount(void);

    unsigned FreeRtosSemaphoreFake_SemaphoreTakeCallCount(void);

    unsigned FreeRtosSemaphoreFake_SemaphoreGiveCallCount(void);

    unsigned FreeRtosSemaphoreFake_SemaphoreDeleteCallCount(void);

SOLIDSYSLOG_EXTERN_C_END

#endif /* FREERTOSSEMAPHOREFAKE_H */
