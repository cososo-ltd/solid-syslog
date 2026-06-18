#ifndef FREERTOSTASKFAKE_H
#define FREERTOSTASKFAKE_H

#include "ExternC.h"
#include "FreeRTOS.h"

EXTERN_C_BEGIN

    void FreeRtosTaskFake_Reset(void);

    unsigned FreeRtosTaskFake_VTaskDelayCallCount(void);
    TickType_t FreeRtosTaskFake_LastVTaskDelayTicks(void);

    void FreeRtosTaskFake_SetTickCount(TickType_t ticks);

EXTERN_C_END

#endif /* FREERTOSTASKFAKE_H */
