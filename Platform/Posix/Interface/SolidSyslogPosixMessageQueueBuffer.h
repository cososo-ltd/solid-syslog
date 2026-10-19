/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  A Buffer backed by a POSIX message queue, decoupling SolidSyslog_Log
 *  (enqueue) from Service (drain) across tasks or processes.
 *
 *  The queue is non-blocking: Write (mq_send) never blocks SolidSyslog_Log. A
 *  full queue applies the injected discard policy - Oldest takes one record out
 *  to make room, Newest and Halt drop the record being written - and the loss
 *  is reported as SOLIDSYSLOG_CAT_BUFFER_RECORDS_LOST, once per run, with a
 *  detail saying which; any other mq_send failure is reported as
 *  SOLIDSYSLOG_CAT_BUFFER_BACKEND_FAILED, as a run of its own. Read
 *  (mq_receive) is a non-blocking poll where an empty queue is silent and any
 *  other failure is reported. Each pool slot owns a distinct queue
 *  /solidsyslog_<pid>_<slot>, unlinked on Destroy. */
#ifndef SOLIDSYSLOGPOSIXMESSAGEQUEUEBUFFER_H
#define SOLIDSYSLOGPOSIXMESSAGEQUEUEBUFFER_H

#include <stddef.h>

#include "SolidSyslogDiscardPolicy.h"
#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogBuffer;
    struct SolidSyslogMutex;

    /** @p maxMessageSize and @p maxMessages size the queue; an exhausted pool
     *  falls back to the shared NullBuffer. @p mutex makes Write safe for tasks
     *  logging at once, and must outlive the buffer; inject
     *  SolidSyslogNullMutex_Get() when only one task logs. It guards this
     *  process's own state, not the queue, which the kernel shares between
     *  processes itself.
     *
     *  @p getDiscardPolicy is asked, with @p discardPolicyContext, on a send
     *  the full queue refuses, from the task that logs; give it the function
     *  the logger has, so the pipeline applies one policy. NULL applies Oldest.
     *  Under Oldest the buffer receives one record out of the queue and sends
     *  again, once; the record it takes out is held in a scratch of
     *  SOLIDSYSLOG_MAX_MESSAGE_SIZE bytes, so a @p maxMessageSize above that
     *  leaves Oldest unable to make room, and the record is refused as under
     *  Newest. Newest and Halt refuse the record being written. */
    struct SolidSyslogBuffer* SolidSyslogPosixMessageQueueBuffer_Create(
        struct SolidSyslogMutex * mutex,
        size_t maxMessageSize,
        size_t maxMessages,
        SolidSyslogDiscardPolicyFunction getDiscardPolicy,
        void* discardPolicyContext
    );
    /** Release the pool slot; closes and unlinks the underlying queue. */
    void SolidSyslogPosixMessageQueueBuffer_Destroy(struct SolidSyslogBuffer * base);

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGPOSIXMESSAGEQUEUEBUFFER_H */
