/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogWolfSslSettings.h"

#include "SolidSyslogWolfSslStreamPrivate.h"

#if SOLIDSYSLOG_WOLFSSL_ANCHORED_PIN_CHECK == SOLIDSYSLOG_WOLFSSL_ANCHORED_PIN_CHECK_AFTER_HANDSHAKE

#include <wolfssl/ssl.h>

void SolidSyslogWolfSslStream_AuthoriseKeptPeer(struct SolidSyslogWolfSslStream* self)
{
    WOLFSSL_X509* peer = wolfSSL_get_peer_certificate(self->Session);
    int length = 0;
    const unsigned char* der = wolfSSL_X509_get_der(peer, &length);
    (void) SolidSyslogWolfSslStream_AuthoriseLeaf(self, der, (word32) length);
    wolfSSL_X509_free(peer);
}

#endif
