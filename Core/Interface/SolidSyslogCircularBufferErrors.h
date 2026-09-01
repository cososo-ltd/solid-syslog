/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  Error codes and Source identity for the CircularBuffer. */
#ifndef SOLIDSYSLOGCIRCULARBUFFERERRORS_H
#define SOLIDSYSLOGCIRCULARBUFFERERRORS_H

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
        SOLIDSYSLOG_CIRCULAR_BUFFER_ERROR_MAX /**< One past the last code; never emitted. Bounds the range for iteration. */
    };

    /** The identity for events raised by a CircularBuffer. A handler matches by
     *  address (event->Source == &SolidSyslogCircularBufferErrorSource), then reads
     *  event->Detail as an enum SolidSyslogCircularBufferErrors. */
    extern const struct SolidSyslogErrorSource SolidSyslogCircularBufferErrorSource;

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGCIRCULARBUFFERERRORS_H */
