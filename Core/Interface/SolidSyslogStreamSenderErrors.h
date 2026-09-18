/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  Error codes and Source identity for the StreamSender. */
#ifndef SOLIDSYSLOGSTREAMSENDERERRORS_H
#define SOLIDSYSLOGSTREAMSENDERERRORS_H

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogErrorSource;

    /** Detail codes for events whose Source is SolidSyslogStreamSenderErrorSource. A
     *  handler reads these off event->Detail after matching event->Source; the
     *  members name their own fault. */
    enum SolidSyslogStreamSenderErrors
    {
        SOLIDSYSLOG_STREAM_SENDER_ERROR_NULL_CONFIG,
        SOLIDSYSLOG_STREAM_SENDER_ERROR_NULL_RESOLVER,
        SOLIDSYSLOG_STREAM_SENDER_ERROR_NULL_STREAM,
        SOLIDSYSLOG_STREAM_SENDER_ERROR_NULL_ADDRESS,
        SOLIDSYSLOG_STREAM_SENDER_ERROR_POOL_EXHAUSTED,
        SOLIDSYSLOG_STREAM_SENDER_ERROR_UNKNOWN_DESTROY,
        SOLIDSYSLOG_STREAM_SENDER_ERROR_DELIVERY_FAILED,
        SOLIDSYSLOG_STREAM_SENDER_ERROR_DELIVERY_RESTORED,
        SOLIDSYSLOG_STREAM_SENDER_ERROR_MAX /**< One past the last code; never emitted. Bounds the range for iteration. */
    };

    /** The identity for events raised by a StreamSender. A handler matches by
     *  address (event->Source == &SolidSyslogStreamSenderErrorSource), then reads
     *  event->Detail as an enum SolidSyslogStreamSenderErrors. */
    extern const struct SolidSyslogErrorSource SolidSyslogStreamSenderErrorSource;

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGSTREAMSENDERERRORS_H */
