/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  Source identity for the LwipSocketTcpStream adapter; the detail codes it
 *  reports are the portable ones in SolidSyslogTcpStreamErrors.h. */
#ifndef SOLIDSYSLOGLWIPSOCKETTCPSTREAMERRORS_H
#define SOLIDSYSLOGLWIPSOCKETTCPSTREAMERRORS_H

#include "SolidSyslogExternC.h"
#include "SolidSyslogTcpStreamErrors.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogErrorSource;

    /** Identity for events raised by a LwipSocketTcpStream. A handler matches by
     *  address (event->Source == &SolidSyslogLwipSocketTcpStreamErrorSource), then
     *  reads event->Detail as an enum SolidSyslogTcpStreamErrors. */
    extern const struct SolidSyslogErrorSource SolidSyslogLwipSocketTcpStreamErrorSource;

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGLWIPSOCKETTCPSTREAMERRORS_H */
