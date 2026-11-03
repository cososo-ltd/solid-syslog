/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  Source identity for the WolfSslHmacSha256Policy adapter; the detail codes it reports are
 *  the portable ones in SolidSyslogHmacSha256PolicyErrors.h. */
#ifndef SOLIDSYSLOGWOLFSSLHMACSHA256POLICYERRORS_H
#define SOLIDSYSLOGWOLFSSLHMACSHA256POLICYERRORS_H

#include "SolidSyslogExternC.h"
#include "SolidSyslogHmacSha256PolicyErrors.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogErrorSource;

    /** Identity for events raised by a WolfSslHmacSha256Policy. A handler matches by
     *  address (event->Source == &SolidSyslogWolfSslHmacSha256PolicyErrorSource), then reads
     *  event->Detail as an enum SolidSyslogHmacSha256PolicyErrors. */
    extern const struct SolidSyslogErrorSource SolidSyslogWolfSslHmacSha256PolicyErrorSource;

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGWOLFSSLHMACSHA256POLICYERRORS_H */
