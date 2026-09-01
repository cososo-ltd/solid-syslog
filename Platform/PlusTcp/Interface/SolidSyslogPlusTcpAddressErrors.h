/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  Error codes and Source identity for the PlusTcpAddress adapter. */
#ifndef SOLIDSYSLOGPLUSTCPADDRESSERRORS_H
#define SOLIDSYSLOGPLUSTCPADDRESSERRORS_H

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogErrorSource;

    /** Detail codes for events whose Source is SolidSyslogPlusTcpAddressErrorSource. A
     *  handler reads these off event->Detail after matching event->Source; the
     *  members name their own fault. */
    enum SolidSyslogPlusTcpAddressErrors
    {
        SOLIDSYSLOG_PLUSTCP_ADDRESS_ERROR_POOL_EXHAUSTED,
        SOLIDSYSLOG_PLUSTCP_ADDRESS_ERROR_UNKNOWN_DESTROY,
        SOLIDSYSLOG_PLUSTCP_ADDRESS_ERROR_MAX /**< One past the last code; never emitted. Bounds the range for iteration. */
    };

    /** Identity for events raised by a PlusTcpAddress. A handler matches by
     *  address (event->Source == &SolidSyslogPlusTcpAddressErrorSource), then reads
     *  event->Detail as an enum SolidSyslogPlusTcpAddressErrors. */
    extern const struct SolidSyslogErrorSource SolidSyslogPlusTcpAddressErrorSource;

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGPLUSTCPADDRESSERRORS_H */
