/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  Error codes and Source identity for the MbedTlsStream adapter. */
#ifndef SOLIDSYSLOGMBEDTLSSTREAMERRORS_H
#define SOLIDSYSLOGMBEDTLSSTREAMERRORS_H

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogErrorSource;

    /** Detail codes for events whose Source is MbedTlsStreamErrorSource. A handler
     *  reads these off event->Detail after matching event->Source; the members
     *  name their own fault. HANDSHAKE_REJECTED and HANDSHAKE_TIMEOUT are distinct
     *  so a handler can tell a peer that refused the handshake from one that never
     *  finished it within the bounded budget. */
    enum SolidSyslogMbedTlsStreamErrors
    {
        SOLIDSYSLOG_MBEDTLS_STREAM_ERROR_POOL_EXHAUSTED,
        SOLIDSYSLOG_MBEDTLS_STREAM_ERROR_UNKNOWN_DESTROY,
        SOLIDSYSLOG_MBEDTLS_STREAM_ERROR_DEFAULTS_NOT_APPLIED,
        SOLIDSYSLOG_MBEDTLS_STREAM_ERROR_SESSION_INIT_FAILED,
        SOLIDSYSLOG_MBEDTLS_STREAM_ERROR_SERVER_NAME_NOT_SET,
        SOLIDSYSLOG_MBEDTLS_STREAM_ERROR_HANDSHAKE_REJECTED,
        SOLIDSYSLOG_MBEDTLS_STREAM_ERROR_HANDSHAKE_TIMEOUT,
        SOLIDSYSLOG_MBEDTLS_STREAM_ERROR_CLIENT_CREDENTIAL_INCOMPLETE,
        SOLIDSYSLOG_MBEDTLS_STREAM_ERROR_CLIENT_CREDENTIAL_NOT_INSTALLED,
        SOLIDSYSLOG_MBEDTLS_STREAM_ERROR_MAX /**< One past the last code; never emitted. Bounds the range for iteration. */
    };

    /** Identity for events raised by an MbedTlsStream. A handler matches by address
     *  (event->Source == &MbedTlsStreamErrorSource), then reads event->Detail as an
     *  enum SolidSyslogMbedTlsStreamErrors. */
    extern const struct SolidSyslogErrorSource MbedTlsStreamErrorSource;

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGMBEDTLSSTREAMERRORS_H */
