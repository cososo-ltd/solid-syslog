/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  Source identity for the VxWorks64Datagram adapter; the detail codes it reports are
 *  the portable ones in SolidSyslogDatagramErrors.h. */
#ifndef SOLIDSYSLOGVXWORKS64DATAGRAMERRORS_H
#define SOLIDSYSLOGVXWORKS64DATAGRAMERRORS_H

#include "SolidSyslogExternC.h"
#include "SolidSyslogDatagramErrors.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogErrorSource;

    /** Identity for events raised by a VxWorks64Datagram. A handler matches by address
     *  (event->Source == &SolidSyslogVxWorks64DatagramErrorSource), then reads event->Detail as an
     *  enum SolidSyslogDatagramErrors. */
    extern const struct SolidSyslogErrorSource SolidSyslogVxWorks64DatagramErrorSource;

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGVXWORKS64DATAGRAMERRORS_H */
