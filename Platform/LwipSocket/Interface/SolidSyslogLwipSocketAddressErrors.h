/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  Source identity for the LwipSocketAddress adapter; the detail codes it
 *  reports are the portable ones in SolidSyslogAddressErrors.h. */
#ifndef SOLIDSYSLOGLWIPSOCKETADDRESSERRORS_H
#define SOLIDSYSLOGLWIPSOCKETADDRESSERRORS_H

#include "SolidSyslogExternC.h"
#include "SolidSyslogAddressErrors.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogErrorSource;

    /** Identity for events raised by a LwipSocketAddress. A handler matches by
     *  address (event->Source == &SolidSyslogLwipSocketAddressErrorSource), then
     *  reads event->Detail as an enum SolidSyslogAddressErrors. */
    extern const struct SolidSyslogErrorSource SolidSyslogLwipSocketAddressErrorSource;

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGLWIPSOCKETADDRESSERRORS_H */
