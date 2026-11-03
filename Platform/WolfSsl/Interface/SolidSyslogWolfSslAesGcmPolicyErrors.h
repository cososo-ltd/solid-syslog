/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  Source identity for the WolfSslAesGcmPolicy adapter; the detail codes it reports are
 *  the portable ones in SolidSyslogAesGcmPolicyErrors.h. */
#ifndef SOLIDSYSLOGWOLFSSLAESGCMPOLICYERRORS_H
#define SOLIDSYSLOGWOLFSSLAESGCMPOLICYERRORS_H

#include "SolidSyslogExternC.h"
#include "SolidSyslogAesGcmPolicyErrors.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogErrorSource;

    /** Identity for events raised by a WolfSslAesGcmPolicy. A handler matches by
     *  address (event->Source == &SolidSyslogWolfSslAesGcmPolicyErrorSource), then reads
     *  event->Detail as an enum SolidSyslogAesGcmPolicyErrors. */
    extern const struct SolidSyslogErrorSource SolidSyslogWolfSslAesGcmPolicyErrorSource;

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGWOLFSSLAESGCMPOLICYERRORS_H */
