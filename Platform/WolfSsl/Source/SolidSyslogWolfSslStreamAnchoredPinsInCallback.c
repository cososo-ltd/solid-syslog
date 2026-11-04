/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogWolfSslSettings.h"

#include <stdbool.h>

#include "SolidSyslogWolfSslStreamPrivate.h"

/* The callback reaches the leaf on every handshake, verified or not, so the pin
 * was checked there and nothing is kept to check again. */
#if SOLIDSYSLOG_WOLFSSL_ANCHORED_PIN_CHECK == SOLIDSYSLOG_WOLFSSL_ANCHORED_PIN_CHECK_IN_CALLBACK

bool SolidSyslogWolfSslStream_ChecksPinsAlongsideAnchors(void)
{
    return true;
}

void SolidSyslogWolfSslStream_AuthoriseKeptPeer(struct SolidSyslogWolfSslStream* self)
{
    (void) self;
}

#endif
