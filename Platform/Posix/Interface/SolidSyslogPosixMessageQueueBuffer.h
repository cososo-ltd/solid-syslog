/** @file
 *  A Buffer backed by a POSIX message queue, decoupling SolidSyslog_Log
 *  (enqueue) from Service (drain) across tasks or processes.
 *
 *  The queue is non-blocking: Write (mq_send) drops the record and reports an
 *  error when the queue is full, so SolidSyslog_Log never blocks; Read
 *  (mq_receive) is a non-blocking poll where an empty queue is silent and any
 *  other failure is reported. Each pool slot owns a distinct queue
 *  /solidsyslog_<pid>_<slot>, unlinked on Destroy. */
#ifndef SOLIDSYSLOGPOSIXMESSAGEQUEUEBUFFER_H
#define SOLIDSYSLOGPOSIXMESSAGEQUEUEBUFFER_H

#include <stddef.h>

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogBuffer;

    /** @p maxMessageSize and @p maxMessages size the queue; an exhausted pool
     *  falls back to the shared NullBuffer. */
    struct SolidSyslogBuffer* SolidSyslogPosixMessageQueueBuffer_Create(size_t maxMessageSize, long maxMessages);
    /** Release the pool slot; closes and unlinks the underlying queue. */
    void SolidSyslogPosixMessageQueueBuffer_Destroy(struct SolidSyslogBuffer * base);

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGPOSIXMESSAGEQUEUEBUFFER_H */
