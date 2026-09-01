/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  Error codes and Source identity for the UdpSender. */
#ifndef SOLIDSYSLOGUDPSENDERERRORS_H
#define SOLIDSYSLOGUDPSENDERERRORS_H

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogErrorSource;

    /** Detail codes for events whose Source is SolidSyslogUdpSenderErrorSource. A handler
     *  reads these off event->Detail after matching event->Source; the members
     *  name their own fault. */
    enum SolidSyslogUdpSenderErrors
    {
        SOLIDSYSLOG_UDP_SENDER_ERROR_NULL_CONFIG,
        SOLIDSYSLOG_UDP_SENDER_ERROR_NULL_RESOLVER,
        SOLIDSYSLOG_UDP_SENDER_ERROR_NULL_DATAGRAM,
        SOLIDSYSLOG_UDP_SENDER_ERROR_NULL_ADDRESS,
        SOLIDSYSLOG_UDP_SENDER_ERROR_NULL_ENDPOINT,
        SOLIDSYSLOG_UDP_SENDER_ERROR_SEND_NULL_BUFFER,
        SOLIDSYSLOG_UDP_SENDER_ERROR_POOL_EXHAUSTED,
        SOLIDSYSLOG_UDP_SENDER_ERROR_UNKNOWN_DESTROY,
        SOLIDSYSLOG_UDP_SENDER_ERROR_DELIVERY_FAILED,
        SOLIDSYSLOG_UDP_SENDER_ERROR_DELIVERY_RESTORED,
        SOLIDSYSLOG_UDP_SENDER_ERROR_MAX /**< One past the last code; never emitted. Bounds the range for iteration. */
    };

    /** The identity for events raised by a UdpSender. A handler matches by
     *  address (event->Source == &SolidSyslogUdpSenderErrorSource), then reads
     *  event->Detail as an enum SolidSyslogUdpSenderErrors. */
    extern const struct SolidSyslogErrorSource SolidSyslogUdpSenderErrorSource;

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGUDPSENDERERRORS_H */
