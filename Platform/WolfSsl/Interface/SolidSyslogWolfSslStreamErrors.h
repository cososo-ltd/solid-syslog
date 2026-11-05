/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  Source identity for the WolfSslStream adapter; the detail codes it reports are
 *  the portable ones in SolidSyslogTlsStreamErrors.h. */
#ifndef SOLIDSYSLOGWOLFSSLSTREAMERRORS_H
#define SOLIDSYSLOGWOLFSSLSTREAMERRORS_H

#include "SolidSyslogExternC.h"
#include "SolidSyslogTlsStreamErrors.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogErrorSource;

    /** Identity for events raised by a WolfSslStream. A handler matches by address
     *  (event->Source == &SolidSyslogWolfSslStreamErrorSource), then reads event->Detail as an
     *  enum SolidSyslogTlsStreamErrors. */
    extern const struct SolidSyslogErrorSource SolidSyslogWolfSslStreamErrorSource;

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGWOLFSSLSTREAMERRORS_H */
