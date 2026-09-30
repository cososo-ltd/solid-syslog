/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  Source identity for the VxWorks64Address adapter; the detail codes it reports are
 *  the portable ones in SolidSyslogAddressErrors.h. */
#ifndef SOLIDSYSLOGVXWORKS64ADDRESSERRORS_H
#define SOLIDSYSLOGVXWORKS64ADDRESSERRORS_H

#include "SolidSyslogExternC.h"
#include "SolidSyslogAddressErrors.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogErrorSource;

    /** Identity for events raised by a VxWorks64Address. A handler matches by address
     *  (event->Source == &SolidSyslogVxWorks64AddressErrorSource), then reads event->Detail as an
     *  enum SolidSyslogAddressErrors. */
    extern const struct SolidSyslogErrorSource SolidSyslogVxWorks64AddressErrorSource;

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGVXWORKS64ADDRESSERRORS_H */
