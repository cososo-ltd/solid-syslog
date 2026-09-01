/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  Error codes and Source identity for the OpenSslAesGcmPolicy adapter. */
#ifndef SOLIDSYSLOGOPENSSLAESGCMPOLICYERRORS_H
#define SOLIDSYSLOGOPENSSLAESGCMPOLICYERRORS_H

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogErrorSource;

    /** Detail codes for events whose Source is SolidSyslogOpenSslAesGcmPolicyErrorSource. A
     *  handler reads these off event->Detail after matching event->Source; the
     *  members name their own fault. */
    enum SolidSyslogOpenSslAesGcmPolicyErrors
    {
        SOLIDSYSLOG_OPENSSL_AES_GCM_POLICY_ERROR_POOL_EXHAUSTED,
        SOLIDSYSLOG_OPENSSL_AES_GCM_POLICY_ERROR_UNKNOWN_DESTROY,
        SOLIDSYSLOG_OPENSSL_AES_GCM_POLICY_ERROR_BAD_CONFIG,
        SOLIDSYSLOG_OPENSSL_AES_GCM_POLICY_ERROR_KEY_UNAVAILABLE,
        SOLIDSYSLOG_OPENSSL_AES_GCM_POLICY_ERROR_NONCE_FAILED,
        SOLIDSYSLOG_OPENSSL_AES_GCM_POLICY_ERROR_ENCRYPT_FAILED,
        SOLIDSYSLOG_OPENSSL_AES_GCM_POLICY_ERROR_DECRYPT_FAILED,
        SOLIDSYSLOG_OPENSSL_AES_GCM_POLICY_ERROR_MAX /**< One past the last code; never emitted. Bounds the range for iteration. */
    };

    /** Identity for events raised by an OpenSslAesGcmPolicy. A handler matches by
     *  address (event->Source == &SolidSyslogOpenSslAesGcmPolicyErrorSource), then reads
     *  event->Detail as an enum SolidSyslogOpenSslAesGcmPolicyErrors. */
    extern const struct SolidSyslogErrorSource SolidSyslogOpenSslAesGcmPolicyErrorSource;

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGOPENSSLAESGCMPOLICYERRORS_H */
