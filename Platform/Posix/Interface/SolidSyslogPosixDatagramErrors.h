/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  Source identity for the PosixDatagram adapter; the detail codes it reports are
 *  the portable ones in SolidSyslogDatagramErrors.h. */
#ifndef SOLIDSYSLOGPOSIXDATAGRAMERRORS_H
#define SOLIDSYSLOGPOSIXDATAGRAMERRORS_H

#include "SolidSyslogExternC.h"
#include "SolidSyslogDatagramErrors.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogErrorSource;

    /** Identity for events raised by a PosixDatagram. A handler matches by address
     *  (event->Source == &SolidSyslogPosixDatagramErrorSource), then reads event->Detail as an
     *  enum SolidSyslogDatagramErrors. */
    extern const struct SolidSyslogErrorSource SolidSyslogPosixDatagramErrorSource;

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGPOSIXDATAGRAMERRORS_H */
