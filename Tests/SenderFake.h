#ifndef SENDERFAKE_H
#define SENDERFAKE_H

#include <stddef.h>

#include "SolidSyslogExternC.h"

struct SolidSyslogSender;

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogSender* SenderFake_Create(void);
    void SenderFake_Destroy(struct SolidSyslogSender * sender);
    void SenderFake_Reset(struct SolidSyslogSender * sender);
    int SenderFake_SendCallCount(struct SolidSyslogSender * sender);
    int SenderFake_DisconnectCallCount(struct SolidSyslogSender * sender);
    const char* SenderFake_LastBufferAsString(struct SolidSyslogSender * sender);
    size_t SenderFake_LastSize(struct SolidSyslogSender * sender);
    void SenderFake_FailNextSend(struct SolidSyslogSender * sender);
    /* Accepts the next @p accepted sends, then fails one. */
    void SenderFake_FailSendAfter(struct SolidSyslogSender * sender, int accepted);

SOLIDSYSLOG_EXTERN_C_END

#endif /* SENDERFAKE_H */
