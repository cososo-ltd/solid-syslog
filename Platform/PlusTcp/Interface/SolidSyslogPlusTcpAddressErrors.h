/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  Source identity for the PlusTcpAddress adapter; the detail codes it reports are
 *  the portable ones in SolidSyslogAddressErrors.h. */
#ifndef SOLIDSYSLOGPLUSTCPADDRESSERRORS_H
#define SOLIDSYSLOGPLUSTCPADDRESSERRORS_H

#include "SolidSyslogExternC.h"
#include "SolidSyslogAddressErrors.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogErrorSource;

    /** Identity for events raised by a PlusTcpAddress. A handler matches by address
     *  (event->Source == &SolidSyslogPlusTcpAddressErrorSource), then reads event->Detail as an
     *  enum SolidSyslogAddressErrors. */
    extern const struct SolidSyslogErrorSource SolidSyslogPlusTcpAddressErrorSource;

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGPLUSTCPADDRESSERRORS_H */
