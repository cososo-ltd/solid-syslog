#ifndef SOLIDSYSLOGPASSTHROUGHBUFFER_H
#define SOLIDSYSLOGPASSTHROUGHBUFFER_H

#include "ExternC.h"

EXTERN_C_BEGIN

    struct SolidSyslogSender;
    struct SolidSyslogBuffer;

    struct SolidSyslogBuffer* SolidSyslogPassthroughBuffer_Create(struct SolidSyslogSender * sender);
    void SolidSyslogPassthroughBuffer_Destroy(struct SolidSyslogBuffer * base);

EXTERN_C_END

#endif /* SOLIDSYSLOGPASSTHROUGHBUFFER_H */
