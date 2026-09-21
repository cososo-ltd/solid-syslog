/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  Source identity for the LwipSocketDatagram adapter; the detail codes it reports
 *  are the portable ones in SolidSyslogDatagramErrors.h. */
#ifndef SOLIDSYSLOGLWIPSOCKETDATAGRAMERRORS_H
#define SOLIDSYSLOGLWIPSOCKETDATAGRAMERRORS_H

#include "SolidSyslogDatagramErrors.h"
#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogErrorSource;

    /** Identity for events raised by a LwipSocketDatagram. A handler matches by
     *  address (event->Source == &SolidSyslogLwipSocketDatagramErrorSource), then
     *  reads event->Detail as an enum SolidSyslogDatagramErrors. */
    extern const struct SolidSyslogErrorSource SolidSyslogLwipSocketDatagramErrorSource;

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGLWIPSOCKETDATAGRAMERRORS_H */
