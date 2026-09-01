/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  An OpenSSL credentials backend that names its material by file path, and
 *  hands those paths to OpenSSL to open and parse. This library performs no
 *  file handling of its own: it neither opens, reads, parses nor buffers the
 *  PEM, so key bytes never pass through it.
 *
 *  Paths are re-read on every connection, so a device issued new credentials
 *  while it is running uses them on its next connect without a restart. */
#ifndef SOLIDSYSLOGOPENSSLPEMFILECREDENTIALS_H
#define SOLIDSYSLOGOPENSSLPEMFILECREDENTIALS_H

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogOpenSslCredentials;

    /** Where this backend's material lives. Every member is a path the caller
     *  owns and must keep valid for the lifetime of the credentials. */
    struct SolidSyslogOpenSslPemFileCredentialsConfig
    {
        /** PEM file of trust anchors the peer certificate must chain to; NULL
         *  installs none, which leaves the peer authorised only if the stream
         *  has another means to do it. */
        const char* CaBundlePath;
        /** PEM leaf certificate (plus intermediates) for mutual TLS; NULL means
         *  no client credential. Certificate and key are all-or-nothing -
         *  supplying one without the other is reported. */
        const char* ClientCertChainPath;
        /** PEM private key matching ClientCertChainPath; NULL means no client
         *  credential. */
        const char* ClientKeyPath;
    };

    /** Draw a credentials instance from the pool. A NULL config is reported and
     *  falls back to the shared Null credentials, as does an exhausted pool. */
    struct SolidSyslogOpenSslCredentials* SolidSyslogOpenSslPemFileCredentials_Create(
        const struct SolidSyslogOpenSslPemFileCredentialsConfig* config
    );
    /** Release the pool slot. */
    void SolidSyslogOpenSslPemFileCredentials_Destroy(struct SolidSyslogOpenSslCredentials * base);

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGOPENSSLPEMFILECREDENTIALS_H */
