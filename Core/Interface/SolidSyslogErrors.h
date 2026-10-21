/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  Error codes and Source identity for the SolidSyslog instance. */
#ifndef SOLIDSYSLOGERRORS_H
#define SOLIDSYSLOGERRORS_H

#include <stdint.h>

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogErrorSource;

    /** Detail codes for events whose Source is SolidSyslogErrorSource (the
     *  SolidSyslog singleton). A handler reads these off event->Detail after
     *  matching event->Source; the members name their own fault. */
    enum SolidSyslogErrors
    {
        SOLIDSYSLOG_ERROR_CREATE_NULL_CONFIG,
        SOLIDSYSLOG_ERROR_CREATE_NULL_BUFFER,
        SOLIDSYSLOG_ERROR_CREATE_NULL_SENDER,
        SOLIDSYSLOG_ERROR_CREATE_NULL_STORE,
        SOLIDSYSLOG_ERROR_CREATE_INCONSISTENT_SD,
        SOLIDSYSLOG_ERROR_LOG_NULL_MESSAGE,
        SOLIDSYSLOG_ERROR_LOG_NULL_HANDLE,
        SOLIDSYSLOG_ERROR_LOG_INCONSISTENT_SD,
        SOLIDSYSLOG_ERROR_SERVICE_NULL_HANDLE,
        SOLIDSYSLOG_ERROR_POOL_EXHAUSTED,
        SOLIDSYSLOG_ERROR_UNKNOWN_DESTROY,
        /** Service dropped the record it was holding for the sender, because
         *  the buffer behind it had filled while the discard policy answers
         *  Oldest: the held record is the oldest in the queue, so it goes first.
         *  Raised under SOLIDSYSLOG_CAT_BUFFER_RECORDS_LOST, once per run of
         *  such drops, re-armed when a held record is dealt with. */
        SOLIDSYSLOG_ERROR_HELD_RECORD_DROPPED,
        SOLIDSYSLOG_ERROR_MAX /**< One past the last code; never emitted. Bounds the range for iteration. */
    };

    /** The detail's name, read beside the category's: CREATE_NULL_CONFIG for
     *  SOLIDSYSLOG_ERROR_CREATE_NULL_CONFIG. NULL for a code outside the enum. */
    const char* SolidSyslogErrors_Name(int32_t detail);

    /** The identity for events raised by the SolidSyslog singleton. A handler
     *  matches by address (event->Source == &SolidSyslogErrorSource), then reads
     *  event->Detail as an enum SolidSyslogErrors. */
    extern const struct SolidSyslogErrorSource SolidSyslogErrorSource;

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGERRORS_H */
