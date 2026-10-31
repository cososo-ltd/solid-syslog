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
 *  /solidsyslog_<pid>_<slot>, whose name is unlinked as soon as it is open. */
#ifndef SOLIDSYSLOGPOSIXMESSAGEQUEUEBUFFER_H
#define SOLIDSYSLOGPOSIXMESSAGEQUEUEBUFFER_H

#include <stddef.h>

#include "SolidSyslogDiscardPolicy.h"
#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogBuffer;
    struct SolidSyslogMutex;

    /** @p maxMessages sizes the queue, whose messages are
     *  SOLIDSYSLOG_MAX_MESSAGE_SIZE bytes. The buffer creates its own queue and
     *  unlinks the name as soon as it is open, so nothing can gain a descriptor
     *  to it by name, and the kernel frees it when the last descriptor closes -
     *  this process's, or a forked child's. A queue that already exists under
     *  the name is refused and left in place; that, like any other failed open,
     *  is reported as ERROR with the errno as the native code, and Create
     *  returns the shared NullBuffer, as it does when the pool is exhausted.
     *
     *  @p mutex makes Write safe for tasks logging at once, and guards what Read
     *  and IsFull share with it, so inject SolidSyslogNullMutex_Get() only when
     *  one task both logs and calls SolidSyslog_Service. It must outlive the
     *  buffer. It guards this process's own state, not the queue, which the
     *  kernel shares between processes itself.
     *
     *  @p getDiscardPolicy is asked, with @p discardPolicyContext, on a send
     *  the full queue refuses, from the task that logs; give it the function
     *  the logger has, so the pipeline applies one policy. NULL applies Oldest.
     *  Under Oldest the buffer receives one record out of the queue and sends
     *  again, once; the record it takes out lands in a scratch of
     *  SOLIDSYSLOG_MAX_MESSAGE_SIZE bytes on the logging task's stack. Newest and
     *  Halt refuse the record
     *  being written. */
    struct SolidSyslogBuffer* SolidSyslogPosixMessageQueueBuffer_Create(
        struct SolidSyslogMutex * mutex,
        size_t maxMessages,
        SolidSyslogDiscardPolicyFunction getDiscardPolicy,
        void* discardPolicyContext
    );
    /** Release the pool slot; closes the underlying queue. */
    void SolidSyslogPosixMessageQueueBuffer_Destroy(struct SolidSyslogBuffer * base);

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGPOSIXMESSAGEQUEUEBUFFER_H */
