#ifndef CLOCKFAKE_H
#define CLOCKFAKE_H

#include <time.h>

#include "ExternC.h"

struct tm;

EXTERN_C_BEGIN

    void ClockFake_Reset(void);
    void ClockFake_SetTime(time_t seconds, long nanoseconds);
    void ClockFake_SetClockGettimeReturn(int returnValue);
    // cppcheck-suppress constParameter -- API allows NULL to signal failure; const would be misleading
    void ClockFake_SetGmtimeReturn(struct tm * returnValue);

EXTERN_C_END

#endif /* CLOCKFAKE_H */
