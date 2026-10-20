/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  Error codes and Source identity for the CircularBuffer. */
#ifndef SOLIDSYSLOGCIRCULARBUFFERERRORS_H
#define SOLIDSYSLOGCIRCULARBUFFERERRORS_H

#include <stdint.h>

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogErrorSource;

    /** Detail codes for events whose Source is SolidSyslogCircularBufferErrorSource. A
     *  handler reads these off event->Detail after matching event->Source; the
     *  members name their own fault. */
    enum SolidSyslogCircularBufferErrors
    {
        SOLIDSYSLOG_CIRCULAR_BUFFER_ERROR_POOL_EXHAUSTED,
        SOLIDSYSLOG_CIRCULAR_BUFFER_ERROR_UNKNOWN_DESTROY,
        /** The record at the head of the ring is larger than the read buffer
         *  Service handed to Read, so it cannot be delivered and is left
         *  un-dequeued - the drain stalls at this record. This cannot occur
         *  under correct configuration (the drain scratch is sized to hold any
         *  record Write accepts), so an emitted code indicates the ring's
         *  length prefix has been corrupted. */
        SOLIDSYSLOG_CIRCULAR_BUFFER_ERROR_RECORD_TOO_LARGE,
        /** The ring had no room for the record and the discard policy refused
         *  it; raised under SOLIDSYSLOG_CAT_BUFFER_RECORDS_LOST, once per run of
         *  refusals. */
        SOLIDSYSLOG_CIRCULAR_BUFFER_ERROR_RECORD_REFUSED,
        /** A write larger than SOLIDSYSLOG_MAX_MESSAGE_SIZE, which only a caller
         *  writing to the buffer directly can make; raised under
         *  SOLIDSYSLOG_CAT_BAD_ARGUMENT, on every such write. */
        SOLIDSYSLOG_CIRCULAR_BUFFER_ERROR_RECORD_EXCEEDS_MAX_MESSAGE_SIZE,
        /** The ring dropped records from its head to make room for the one being
         *  written, as the Oldest policy directs; raised under
         *  SOLIDSYSLOG_CAT_BUFFER_RECORDS_LOST, once per run of writes that
         *  evicted. */
        SOLIDSYSLOG_CIRCULAR_BUFFER_ERROR_RECORDS_EVICTED,
        SOLIDSYSLOG_CIRCULAR_BUFFER_ERROR_MAX /**< One past the last code; never emitted. Bounds the range for iteration. */
    };

    /** The name the documentation uses for @p detail, without the
     *  SOLIDSYSLOG_CIRCULAR_BUFFER_ERROR_ prefix; NULL for a value outside the enum. */
    const char* SolidSyslogCircularBufferErrors_Name(int32_t detail);

    /** The identity for events raised by a CircularBuffer. A handler matches by
     *  address (event->Source == &SolidSyslogCircularBufferErrorSource), then reads
     *  event->Detail as an enum SolidSyslogCircularBufferErrors. */
    extern const struct SolidSyslogErrorSource SolidSyslogCircularBufferErrorSource;

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGCIRCULARBUFFERERRORS_H */
