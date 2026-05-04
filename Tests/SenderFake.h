#ifndef SENDERFAKE_H
#define SENDERFAKE_H

#include <stddef.h>

#include "ExternC.h"

struct SolidSyslogSender;

EXTERN_C_BEGIN

    struct SolidSyslogSender* SenderFake_Create(void);
    void                      SenderFake_Destroy(struct SolidSyslogSender * sender);
    void                      SenderFake_Reset(struct SolidSyslogSender * sender);
    int                       SenderFake_SendCount(struct SolidSyslogSender * sender);
    int                       SenderFake_DisconnectCount(struct SolidSyslogSender * sender);
    const char*               SenderFake_LastBufferAsString(struct SolidSyslogSender * sender);
    size_t                    SenderFake_LastSize(struct SolidSyslogSender * sender);
    void                      SenderFake_FailNextSend(struct SolidSyslogSender * sender);

EXTERN_C_END

#endif /* SENDERFAKE_H */
