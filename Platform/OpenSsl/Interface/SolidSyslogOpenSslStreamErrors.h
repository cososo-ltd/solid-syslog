/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  Error codes and Source identity for the OpenSslStream adapter. */
#ifndef SOLIDSYSLOGOPENSSLSTREAMERRORS_H
#define SOLIDSYSLOGOPENSSLSTREAMERRORS_H

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogErrorSource;

    /** Detail codes for events whose Source is OpenSslStreamErrorSource. A handler
     *  reads these off event->Detail after matching event->Source; the members
     *  name their own fault. */
    enum SolidSyslogOpenSslStreamErrors
    {
        SOLIDSYSLOG_OPENSSL_STREAM_ERROR_POOL_EXHAUSTED,
        SOLIDSYSLOG_OPENSSL_STREAM_ERROR_UNKNOWN_DESTROY,
        SOLIDSYSLOG_OPENSSL_STREAM_ERROR_CONTEXT_INIT_FAILED,
        SOLIDSYSLOG_OPENSSL_STREAM_ERROR_SESSION_INIT_FAILED,
        SOLIDSYSLOG_OPENSSL_STREAM_ERROR_SERVER_NAME_NOT_SET,
        SOLIDSYSLOG_OPENSSL_STREAM_ERROR_HANDSHAKE_REJECTED,
        SOLIDSYSLOG_OPENSSL_STREAM_ERROR_HANDSHAKE_TIMEOUT,
        SOLIDSYSLOG_OPENSSL_STREAM_ERROR_CLIENT_CREDENTIAL_INCOMPLETE,
        SOLIDSYSLOG_OPENSSL_STREAM_ERROR_CLIENT_CREDENTIAL_MISMATCHED,
        SOLIDSYSLOG_OPENSSL_STREAM_ERROR_CLIENT_CREDENTIAL_NOT_INSTALLED,
        SOLIDSYSLOG_OPENSSL_STREAM_ERROR_NULL_CONFIG,
        SOLIDSYSLOG_OPENSSL_STREAM_ERROR_NULL_TRANSPORT,
        SOLIDSYSLOG_OPENSSL_STREAM_ERROR_NULL_SLEEP,
        SOLIDSYSLOG_OPENSSL_STREAM_ERROR_MAX /**< One past the last code; never emitted. Bounds the range for iteration. */
    };

    /** Identity for events raised by a OpenSslStream. A handler matches by address
     *  (event->Source == &OpenSslStreamErrorSource), then reads event->Detail as an
     *  enum SolidSyslogOpenSslStreamErrors. */
    extern const struct SolidSyslogErrorSource OpenSslStreamErrorSource;

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGOPENSSLSTREAMERRORS_H */
