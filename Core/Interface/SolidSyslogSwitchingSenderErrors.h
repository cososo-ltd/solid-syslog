/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  Error codes and Source identity for the SwitchingSender. */
#ifndef SOLIDSYSLOGSWITCHINGSENDERERRORS_H
#define SOLIDSYSLOGSWITCHINGSENDERERRORS_H

#include <stdint.h>

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogErrorSource;

    /** Detail codes for events whose Source is SolidSyslogSwitchingSenderErrorSource. A
     *  handler reads these off event->Detail after matching event->Source; the
     *  members name their own fault. */
    enum SolidSyslogSwitchingSenderErrors
    {
        SOLIDSYSLOG_SWITCHING_SENDER_ERROR_NULL_CONFIG,
        SOLIDSYSLOG_SWITCHING_SENDER_ERROR_NULL_SENDERS,
        SOLIDSYSLOG_SWITCHING_SENDER_ERROR_NULL_SELECTOR,
        SOLIDSYSLOG_SWITCHING_SENDER_ERROR_POOL_EXHAUSTED,
        SOLIDSYSLOG_SWITCHING_SENDER_ERROR_UNKNOWN_DESTROY,
        SOLIDSYSLOG_SWITCHING_SENDER_ERROR_NULL_INNER_SENDER, /**< An element of Senders within SenderCount is NULL. */
        SOLIDSYSLOG_SWITCHING_SENDER_ERROR_NO_SENDERS, /**< SenderCount is 0, so no Send could reach a sender. */
        /** The Selector answered an index at or beyond SenderCount, so the record
         *  went to the NullSender and is lost. Raised as a CRITICAL under
         *  SOLIDSYSLOG_CAT_SENDER_RECORDS_LOST, once per run of such Sends; the next
         *  Send the Selector routes to a sender in range ends the run. */
        SOLIDSYSLOG_SWITCHING_SENDER_ERROR_SELECTOR_OUT_OF_RANGE,
        SOLIDSYSLOG_SWITCHING_SENDER_ERROR_MAX /**< One past the last code; never emitted. Bounds the range for iteration. */
    };

    /** The detail's name, read beside the category's: NULL_CONFIG for
     *  SOLIDSYSLOG_SWITCHING_SENDER_ERROR_NULL_CONFIG. NULL for a code outside the enum. */
    const char* SolidSyslogSwitchingSenderErrors_Name(int32_t detail);

    /** The identity for events raised by a SwitchingSender. A handler matches by
     *  address (event->Source == &SolidSyslogSwitchingSenderErrorSource), then reads
     *  event->Detail as an enum SolidSyslogSwitchingSenderErrors. */
    extern const struct SolidSyslogErrorSource SolidSyslogSwitchingSenderErrorSource;

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGSWITCHINGSENDERERRORS_H */
