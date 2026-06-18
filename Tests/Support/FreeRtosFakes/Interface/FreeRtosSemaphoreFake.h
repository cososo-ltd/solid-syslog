#ifndef FREERTOSSEMAPHOREFAKE_H
#define FREERTOSSEMAPHOREFAKE_H

#include "ExternC.h"
#include "FreeRTOS.h"

EXTERN_C_BEGIN

    void FreeRtosSemaphoreFake_Reset(void);

    unsigned FreeRtosSemaphoreFake_CreateMutexStaticCallCount(void);

    unsigned FreeRtosSemaphoreFake_SemaphoreTakeCallCount(void);

    unsigned FreeRtosSemaphoreFake_SemaphoreGiveCallCount(void);

    unsigned FreeRtosSemaphoreFake_SemaphoreDeleteCallCount(void);

EXTERN_C_END

#endif /* FREERTOSSEMAPHOREFAKE_H */
