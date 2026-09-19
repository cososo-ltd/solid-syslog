/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  Source identity for the PlusTcpDatagram adapter; the detail codes it reports are
 *  the portable ones in SolidSyslogDatagramErrors.h. */
#ifndef SOLIDSYSLOGPLUSTCPDATAGRAMERRORS_H
#define SOLIDSYSLOGPLUSTCPDATAGRAMERRORS_H

#include "SolidSyslogExternC.h"
#include "SolidSyslogDatagramErrors.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogErrorSource;

    /** Identity for events raised by a PlusTcpDatagram. A handler matches by address
     *  (event->Source == &SolidSyslogPlusTcpDatagramErrorSource), then reads event->Detail as an
     *  enum SolidSyslogDatagramErrors. */
    extern const struct SolidSyslogErrorSource SolidSyslogPlusTcpDatagramErrorSource;

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGPLUSTCPDATAGRAMERRORS_H */
