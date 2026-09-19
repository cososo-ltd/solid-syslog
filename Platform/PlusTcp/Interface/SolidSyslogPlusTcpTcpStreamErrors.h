/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  Source identity for the PlusTcpTcpStream adapter; the detail codes it reports are
 *  the portable ones in SolidSyslogTcpStreamErrors.h. */
#ifndef SOLIDSYSLOGPLUSTCPTCPSTREAMERRORS_H
#define SOLIDSYSLOGPLUSTCPTCPSTREAMERRORS_H

#include "SolidSyslogExternC.h"
#include "SolidSyslogTcpStreamErrors.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogErrorSource;

    /** Identity for events raised by a PlusTcpTcpStream. A handler matches by address
     *  (event->Source == &SolidSyslogPlusTcpTcpStreamErrorSource), then reads event->Detail as an
     *  enum SolidSyslogTcpStreamErrors. */
    extern const struct SolidSyslogErrorSource SolidSyslogPlusTcpTcpStreamErrorSource;

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGPLUSTCPTCPSTREAMERRORS_H */
