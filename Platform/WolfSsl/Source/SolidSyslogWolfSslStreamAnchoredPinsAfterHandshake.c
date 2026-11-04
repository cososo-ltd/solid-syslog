/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogWolfSslSettings.h"

#include "SolidSyslogWolfSslStreamPrivate.h"

#if SOLIDSYSLOG_WOLFSSL_ANCHORED_PIN_CHECK == SOLIDSYSLOG_WOLFSSL_ANCHORED_PIN_CHECK_AFTER_HANDSHAKE

void SolidSyslogWolfSslStream_AuthoriseKeptPeer(struct SolidSyslogWolfSslStream* self)
{
    self->PeerPinned = true;
}

#endif
