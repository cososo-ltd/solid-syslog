#ifndef BUFFERFAKE_H
#define BUFFERFAKE_H

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogBuffer* BufferFake_Create(void);
    void BufferFake_Destroy(void);

SOLIDSYSLOG_EXTERN_C_END

#endif /* BUFFERFAKE_H */
