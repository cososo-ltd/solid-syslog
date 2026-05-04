#ifndef BUFFERFAKE_H
#define BUFFERFAKE_H

#include "ExternC.h"

EXTERN_C_BEGIN

    struct SolidSyslogBuffer* BufferFake_Create(void);
    void                      BufferFake_Destroy(void);

EXTERN_C_END

#endif /* BUFFERFAKE_H */
