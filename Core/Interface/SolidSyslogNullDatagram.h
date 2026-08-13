/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  The no-op Datagram Null object: SendTo returns SENT (drops the datagram on the floor so
 *  the Store does not fill with undeliverables), MaxPayload returns the IPv6-safe default,
 *  Open and Close are no-ops. */
#ifndef SOLIDSYSLOGNULLDATAGRAM_H
#define SOLIDSYSLOGNULLDATAGRAM_H

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    /** Open and Close are no-ops. SendTo returns SENT, reporting the datagram as
     *  delivered so the Store drops it rather than accumulating undeliverables.
     *  MaxPayload returns the IPv6-safe default. */
    struct SolidSyslogDatagram* SolidSyslogNullDatagram_Get(void);

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGNULLDATAGRAM_H */
