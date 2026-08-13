/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  Error codes and Source identity for the OriginSd. */
#ifndef SOLIDSYSLOGORIGINSDERRORS_H
#define SOLIDSYSLOGORIGINSDERRORS_H

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogErrorSource;

    /** Detail codes for events whose Source is OriginSdErrorSource. A handler
     *  reads these off event->Detail after matching event->Source; the members
     *  name their own fault. */
    enum SolidSyslogOriginSdErrors
    {
        SOLIDSYSLOG_ORIGIN_SD_ERROR_POOL_EXHAUSTED,
        SOLIDSYSLOG_ORIGIN_SD_ERROR_UNKNOWN_DESTROY,
        SOLIDSYSLOG_ORIGIN_SD_ERROR_MAX /**< One past the last code; never emitted. Bounds the range for iteration. */
    };

    /** The identity for events raised by an OriginSd. A handler matches by
     *  address (event->Source == &OriginSdErrorSource), then reads
     *  event->Detail as an enum SolidSyslogOriginSdErrors. */
    extern const struct SolidSyslogErrorSource OriginSdErrorSource;

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGORIGINSDERRORS_H */
