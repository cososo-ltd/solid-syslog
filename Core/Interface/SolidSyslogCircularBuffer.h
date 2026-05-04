#ifndef SOLIDSYSLOGCIRCULARBUFFER_H
#define SOLIDSYSLOGCIRCULARBUFFER_H

#include "ExternC.h"

EXTERN_C_BEGIN

    struct SolidSyslogBuffer;

    struct SolidSyslogBuffer* SolidSyslogCircularBuffer_Create(void);
    void                      SolidSyslogCircularBuffer_Destroy(void);

EXTERN_C_END

#endif /* SOLIDSYSLOGCIRCULARBUFFER_H */
