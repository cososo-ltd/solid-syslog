#ifndef FREERTOSSEMAPHOREFAKE_H
#define FREERTOSSEMAPHOREFAKE_H

#include "ExternC.h"

#include "FreeRTOS.h"
#include "semphr.h"

EXTERN_C_BEGIN

    void FreeRtosSemaphoreFake_Reset(void);

    /* xSemaphoreCreateMutexStatic accessors */
    unsigned FreeRtosSemaphoreFake_CreateMutexStaticCallCount(void);

    /* xSemaphoreTake accessors */
    unsigned FreeRtosSemaphoreFake_SemaphoreTakeCallCount(void);

    /* xSemaphoreGive accessors */
    unsigned FreeRtosSemaphoreFake_SemaphoreGiveCallCount(void);

    /* vSemaphoreDelete accessors */
    unsigned FreeRtosSemaphoreFake_SemaphoreDeleteCallCount(void);

EXTERN_C_END

#endif /* FREERTOSSEMAPHOREFAKE_H */
