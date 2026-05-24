#ifndef MQFAKE_H
#define MQFAKE_H

#include <mqueue.h>
#include <stddef.h>

#include "ExternC.h"

EXTERN_C_BEGIN

    void MqFake_Reset(void);

    /* open configuration — one-shot, consumed by the next mq_open call */
    void MqFake_FailNextOpen(int errnoValue);

    /* open accessors */
    int MqFake_OpenCallCount(void);
    const char* MqFake_LastOpenName(void);
    int MqFake_LastOpenOflag(void);
    long MqFake_LastOpenMaxMessages(void);
    size_t MqFake_LastOpenMaxMessageSize(void);
    const char* MqFake_OpenNameAt(int callIndex);

    /* send configuration — one-shot, consumed by the next mq_send call */
    void MqFake_FailNextSend(int errnoValue);

    /* send accessors */
    int MqFake_SendCallCount(void);
    mqd_t MqFake_LastSendMqd(void);
    const char* MqFake_LastSendBufAsString(void);
    size_t MqFake_LastSendLen(void);

    /* receive configuration — one-shot, consumed by the next mq_receive call */
    void MqFake_FailNextReceive(int errnoValue);

    /* receive accessors */
    int MqFake_ReceiveCallCount(void);
    mqd_t MqFake_LastReceiveMqd(void);
    size_t MqFake_LastReceiveMaxLen(void);

    /* close accessors */
    int MqFake_CloseCallCount(void);
    mqd_t MqFake_LastClosedMqd(void);

    /* unlink accessors */
    int MqFake_UnlinkCallCount(void);
    const char* MqFake_LastUnlinkName(void);

EXTERN_C_END

#endif /* MQFAKE_H */
