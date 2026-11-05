/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  Error codes and Source identity for the PosixMessageQueueBuffer adapter. */
#ifndef SOLIDSYSLOGPOSIXMESSAGEQUEUEBUFFERERRORS_H
#define SOLIDSYSLOGPOSIXMESSAGEQUEUEBUFFERERRORS_H

#include <stdint.h>

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogErrorSource;

    /** Detail codes for events whose Source is SolidSyslogPosixMessageQueueBufferErrorSource.
     *  A handler reads these off event->Detail after matching event->Source; the
     *  members name their own fault (MQ_OPEN at Create, SEND on enqueue, RECEIVE
     *  on drain). */
    enum SolidSyslogPosixMessageQueueBufferErrors
    {
        SOLIDSYSLOG_POSIX_MESSAGE_QUEUE_BUFFER_ERROR_POOL_EXHAUSTED,
        SOLIDSYSLOG_POSIX_MESSAGE_QUEUE_BUFFER_ERROR_UNKNOWN_DESTROY,
        SOLIDSYSLOG_POSIX_MESSAGE_QUEUE_BUFFER_ERROR_MQ_OPEN_FAILED,
        /** mq_send failed other than on a full queue; raised under
         *  SOLIDSYSLOG_CAT_BUFFER_BACKEND_FAILED, once per run of failed sends;
         *  the next record the queue takes ends the run. */
        SOLIDSYSLOG_POSIX_MESSAGE_QUEUE_BUFFER_ERROR_SEND_FAILED,
        /** mq_receive failed other than on an empty queue; raised under
         *  SOLIDSYSLOG_CAT_BUFFER_BACKEND_FAILED, once per run of failed
         *  receives; the next record received ends the run. */
        SOLIDSYSLOG_POSIX_MESSAGE_QUEUE_BUFFER_ERROR_RECEIVE_FAILED,
        /** The queue was full (EAGAIN) and the discard policy refused the
         *  record; raised under SOLIDSYSLOG_CAT_BUFFER_RECORDS_LOST, once per
         *  run of refusals. */
        SOLIDSYSLOG_POSIX_MESSAGE_QUEUE_BUFFER_ERROR_RECORD_REFUSED,
        /** The queue was full and the Oldest policy took a record out of it to
         *  make room for the one being written; raised under
         *  SOLIDSYSLOG_CAT_BUFFER_RECORDS_LOST, once per run of sends that
         *  evicted. */
        SOLIDSYSLOG_POSIX_MESSAGE_QUEUE_BUFFER_ERROR_RECORDS_EVICTED,
        /** Create was given no mutex; raised under SOLIDSYSLOG_CAT_BAD_CONFIG. */
        SOLIDSYSLOG_POSIX_MESSAGE_QUEUE_BUFFER_ERROR_NULL_MUTEX,
        SOLIDSYSLOG_POSIX_MESSAGE_QUEUE_BUFFER_ERROR_MAX /**< One past the last code; never emitted. Bounds iteration. */
    };

    /** The detail's name, read beside the category's: MQ_OPEN_FAILED for
     *  SOLIDSYSLOG_POSIX_MESSAGE_QUEUE_BUFFER_ERROR_MQ_OPEN_FAILED. NULL for a code outside the enum. */
    const char* SolidSyslogPosixMessageQueueBufferErrors_Name(int32_t detail);

    /** Identity for events raised by a PosixMessageQueueBuffer. A handler matches
     *  by address (event->Source == &SolidSyslogPosixMessageQueueBufferErrorSource), then
     *  reads event->Detail as an enum SolidSyslogPosixMessageQueueBufferErrors. */
    extern const struct SolidSyslogErrorSource SolidSyslogPosixMessageQueueBufferErrorSource;

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGPOSIXMESSAGEQUEUEBUFFERERRORS_H */
