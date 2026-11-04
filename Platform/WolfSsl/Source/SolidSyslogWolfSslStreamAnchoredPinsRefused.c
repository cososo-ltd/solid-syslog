/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogWolfSslSettings.h"

#include <stdbool.h>

#include "SolidSyslogWolfSslStreamPrivate.h"

/* No body can check a pin on a chain that verifies, so anchors and pins
 * together are refused at Open, and a pin alone is checked in the callback,
 * which every unanchored chain reaches. Nothing is kept to check again. */
#if SOLIDSYSLOG_WOLFSSL_ANCHORED_PIN_CHECK == SOLIDSYSLOG_WOLFSSL_ANCHORED_PIN_CHECK_REFUSED

bool SolidSyslogWolfSslStream_ChecksPinsAlongsideAnchors(void)
{
    return false;
}

void SolidSyslogWolfSslStream_AuthoriseKeptPeer(struct SolidSyslogWolfSslStream* self)
{
    (void) self;
}

#endif
