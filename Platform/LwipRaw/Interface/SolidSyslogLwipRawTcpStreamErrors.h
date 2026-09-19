/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  Source identity for the LwipRawTcpStream adapter; the detail codes it reports are
 *  the portable ones in SolidSyslogTcpStreamErrors.h. */
#ifndef SOLIDSYSLOGLWIPRAWTCPSTREAMERRORS_H
#define SOLIDSYSLOGLWIPRAWTCPSTREAMERRORS_H

#include "SolidSyslogExternC.h"
#include "SolidSyslogTcpStreamErrors.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogErrorSource;

    /** Identity for events raised by a LwipRawTcpStream. A handler matches by address
     *  (event->Source == &SolidSyslogLwipRawTcpStreamErrorSource), then reads event->Detail as an
     *  enum SolidSyslogTcpStreamErrors. */
    extern const struct SolidSyslogErrorSource SolidSyslogLwipRawTcpStreamErrorSource;

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGLWIPRAWTCPSTREAMERRORS_H */
