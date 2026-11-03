/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogWolfSslSettings.h"

#include <stdbool.h>
#include <stdint.h>

#include "SolidSyslogWolfSslAesGcmPolicyPrivate.h"
#include "SolidSyslogSecurityPolicyDefinition.h"

/* The Xilinx AF_ALG port reads and writes WC_AES_BLOCK_SIZE bytes beyond the
 * body, which here is the record's trailer. The definitions are withheld on
 * that build, so the link names what is missing. */
#ifndef WOLFSSL_AFALG_XILINX_AES

#include <wolfssl/wolfcrypt/aes.h>
#include <wolfssl/wolfcrypt/error-crypt.h>
#include <wolfssl/wolfcrypt/types.h>

/* One AES-256-GCM seal with a transient wolfCrypt context: the body is
 * encrypted in place, the header authenticated, and the tag written after the
 * nonce already in the trailer. Any step failing is one fault to the caller;
 * the context is freed whenever it was initialised. */
bool SolidSyslogWolfSslAesGcmPolicy_GcmEncrypt(const struct SolidSyslogSecurityRecord* record, const uint8_t* key)
{
    uint8_t* body = &record->Content[record->HeaderLength];
    uint16_t bodyLength = (uint16_t) (record->ContentLength - record->HeaderLength);

    Aes aes;
    bool ok = false;
    if (wc_AesInit(&aes, NULL, INVALID_DEVID) == 0)
    {
        ok = (wc_AesGcmSetKey(&aes, key, SOLIDSYSLOG_WOLFSSL_AES_GCM_POLICY_KEY_SIZE) == 0) &&
             (wc_AesGcmEncrypt(
                  &aes,
                  body,
                  body,
                  bodyLength,
                  record->Trailer,
                  SOLIDSYSLOG_WOLFSSL_AES_GCM_POLICY_NONCE_SIZE,
                  &record->Trailer[SOLIDSYSLOG_WOLFSSL_AES_GCM_POLICY_NONCE_SIZE],
                  SOLIDSYSLOG_WOLFSSL_AES_GCM_POLICY_TAG_SIZE,
                  record->Content,
                  record->HeaderLength
              ) == 0);
        wc_AesFree(&aes);
    }
    return ok;
}

/* The verdict comes from wc_AesGcmDecrypt's return code: 0 is authentic,
 * AES_GCM_AUTH_E is tamper or wrong key (the expected rejection - silent, like
 * the HMAC verify), anything else a genuine wolfCrypt error, which reaches no
 * verdict. */
enum SolidSyslogSecurityPolicyOpenResult SolidSyslogWolfSslAesGcmPolicy_GcmDecrypt(
    const struct SolidSyslogSecurityRecord* record,
    const uint8_t* key
)
{
    uint8_t* body = &record->Content[record->HeaderLength];
    uint16_t bodyLength = (uint16_t) (record->ContentLength - record->HeaderLength);

    Aes aes;
    enum SolidSyslogSecurityPolicyOpenResult result = SOLIDSYSLOG_SECURITY_POLICY_OPEN_COULD_NOT_RUN;
    if (wc_AesInit(&aes, NULL, INVALID_DEVID) == 0)
    {
        if (wc_AesGcmSetKey(&aes, key, SOLIDSYSLOG_WOLFSSL_AES_GCM_POLICY_KEY_SIZE) == 0)
        {
            int verdict = wc_AesGcmDecrypt(
                &aes,
                body,
                body,
                bodyLength,
                record->Trailer,
                SOLIDSYSLOG_WOLFSSL_AES_GCM_POLICY_NONCE_SIZE,
                &record->Trailer[SOLIDSYSLOG_WOLFSSL_AES_GCM_POLICY_NONCE_SIZE],
                SOLIDSYSLOG_WOLFSSL_AES_GCM_POLICY_TAG_SIZE,
                record->Content,
                record->HeaderLength
            );
            if (verdict == 0)
            {
                result = SOLIDSYSLOG_SECURITY_POLICY_OPEN_OPENED;
            }
            else if (verdict == AES_GCM_AUTH_E)
            {
                result = SOLIDSYSLOG_SECURITY_POLICY_OPEN_REJECTED;
            }
            else
            {
                /* Genuine wolfCrypt failure - no verdict. */
            }
        }
        wc_AesFree(&aes);
    }
    return result;
}

#endif /* WOLFSSL_AFALG_XILINX_AES */
