/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogWolfSslSettings.h"

#include "SolidSyslogWolfSslAesGcmPolicy.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <wolfssl/wolfcrypt/aes.h>
#include <wolfssl/wolfcrypt/error-crypt.h>
#include <wolfssl/wolfcrypt/memory.h>
#include <wolfssl/wolfcrypt/random.h>
#include <wolfssl/wolfcrypt/types.h>

#include "SolidSyslogError.h"
#include "SolidSyslogErrorCategory.h"
#include "SolidSyslogWolfSslAesGcmPolicyErrors.h"
#include "SolidSyslogWolfSslAesGcmPolicyPrivate.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogSecurityPolicyCategories.h"
#include "SolidSyslogSecurityPolicyDefinition.h"

const struct SolidSyslogErrorSource SolidSyslogWolfSslAesGcmPolicyErrorSource = {
    "WolfSslAesGcmPolicy",
    SolidSyslogAesGcmPolicyErrors_Name
};

enum
{
    AES_GCM_POLICY_KEY_SIZE = 32,
    AES_GCM_POLICY_NONCE_SIZE = 12,
    AES_GCM_POLICY_TAG_SIZE = 16,
    /* Trailer is nonce || tag - fits SOLIDSYSLOG_MAX_INTEGRITY_SIZE (32). */
    AES_GCM_POLICY_TRAILER_SIZE = AES_GCM_POLICY_NONCE_SIZE + AES_GCM_POLICY_TAG_SIZE
};

static inline struct SolidSyslogWolfSslAesGcmPolicy* WolfSslAesGcmPolicy_SelfFromBase(
    struct SolidSyslogSecurityPolicy* base
);
static bool WolfSslAesGcmPolicy_SealRecord(
    struct SolidSyslogSecurityPolicy* self,
    const struct SolidSyslogSecurityRecord* record
);
static enum SolidSyslogSecurityPolicyOpenResult WolfSslAesGcmPolicy_OpenRecord(
    struct SolidSyslogSecurityPolicy* self,
    const struct SolidSyslogSecurityRecord* record
);
static bool WolfSslAesGcmPolicy_FetchKey(struct SolidSyslogWolfSslAesGcmPolicy* policy, uint8_t* keyOut);
static void WolfSslAesGcmPolicy_ReportFault(
    struct SolidSyslogWolfSslAesGcmPolicy* policy,
    uint16_t category,
    enum SolidSyslogAesGcmPolicyErrors code
);
static bool WolfSslAesGcmPolicy_GcmEncrypt(const struct SolidSyslogSecurityRecord* record, const uint8_t* key);
static enum SolidSyslogSecurityPolicyOpenResult WolfSslAesGcmPolicy_GcmDecrypt(
    const struct SolidSyslogSecurityRecord* record,
    const uint8_t* key
);

void SolidSyslogWolfSslAesGcmPolicy_Initialise(
    struct SolidSyslogSecurityPolicy* base,
    const struct SolidSyslogWolfSslAesGcmPolicyConfig* config
)
{
    struct SolidSyslogWolfSslAesGcmPolicy* self = WolfSslAesGcmPolicy_SelfFromBase(base);
    self->Base.TrailerSize = AES_GCM_POLICY_TRAILER_SIZE;
    self->Base.SealRecord = WolfSslAesGcmPolicy_SealRecord;
    self->Base.OpenRecord = WolfSslAesGcmPolicy_OpenRecord;
    self->Config = *config;
    self->FaultReported = false;
}

void SolidSyslogWolfSslAesGcmPolicy_Cleanup(struct SolidSyslogSecurityPolicy* base)
{
    /* No owned resources to release - the key is fetched on demand via the
     * GetKey callback and never stored on the instance, and the RNG is
     * caller-owned. */
    (void) base;
}

static inline struct SolidSyslogWolfSslAesGcmPolicy* WolfSslAesGcmPolicy_SelfFromBase(
    struct SolidSyslogSecurityPolicy* base
)
{
    /* Base is the first member of the instance struct - see Private.h. */
    return (struct SolidSyslogWolfSslAesGcmPolicy*) base;
}

/* Seals in place: draws a fresh nonce into Trailer[0..AES_GCM_POLICY_NONCE_SIZE), then
 * GcmEncrypt encrypts the body (Content past HeaderLength) and writes the tag to
 * the remaining trailer bytes, authenticating the header (Content[0..HeaderLength))
 * as associated data. Fetches the key on demand and wipes it before returning. */
static bool WolfSslAesGcmPolicy_SealRecord(
    struct SolidSyslogSecurityPolicy* self,
    const struct SolidSyslogSecurityRecord* record
)
{
    struct SolidSyslogWolfSslAesGcmPolicy* policy = WolfSslAesGcmPolicy_SelfFromBase(self);

    bool sealed = false;
    uint8_t key[AES_GCM_POLICY_KEY_SIZE];
    if (WolfSslAesGcmPolicy_FetchKey(policy, key))
    {
        /* Draw the per-record nonce straight into the trailer; GcmEncrypt reads
         * it back from there and writes the tag immediately after it. */
        if (wc_RNG_GenerateBlock(policy->Config.Rng, record->Trailer, AES_GCM_POLICY_NONCE_SIZE) == 0)
        {
            if (WolfSslAesGcmPolicy_GcmEncrypt(record, key))
            {
                sealed = true;
                policy->FaultReported = false;
            }
            else
            {
                WolfSslAesGcmPolicy_ReportFault(
                    policy,
                    SOLIDSYSLOG_CAT_SECURITY_POLICY_SEAL_FAILED,
                    SOLIDSYSLOG_AES_GCM_POLICY_ERROR_ENCRYPT_FAILED
                );
            }
        }
        else
        {
            WolfSslAesGcmPolicy_ReportFault(
                policy,
                SOLIDSYSLOG_CAT_SECURITY_POLICY_SEAL_FAILED,
                SOLIDSYSLOG_AES_GCM_POLICY_ERROR_NONCE_FAILED
            );
        }
    }
    wc_ForceZero(key, sizeof key);
    return sealed;
}

/* Fetches the AES-256 key on demand. Fails closed (and reports) if the key is
 * unavailable or not exactly 32 bytes - AES-256 admits no other key length. */
static bool WolfSslAesGcmPolicy_FetchKey(struct SolidSyslogWolfSslAesGcmPolicy* policy, uint8_t* keyOut)
{
    size_t keyLength = 0U;
    bool fetched = policy->Config.GetKey(policy->Config.KeyContext, keyOut, AES_GCM_POLICY_KEY_SIZE, &keyLength) &&
                   (keyLength == (size_t) AES_GCM_POLICY_KEY_SIZE);
    if (!fetched)
    {
        WolfSslAesGcmPolicy_ReportFault(
            policy,
            SOLIDSYSLOG_CAT_SECURITY_POLICY_KEY_UNAVAILABLE,
            SOLIDSYSLOG_AES_GCM_POLICY_ERROR_KEY_UNAVAILABLE
        );
    }
    return fetched;
}

/* The first fault in a run is the one reported. */
static void WolfSslAesGcmPolicy_ReportFault(
    struct SolidSyslogWolfSslAesGcmPolicy* policy,
    uint16_t category,
    enum SolidSyslogAesGcmPolicyErrors code
)
{
    if (!policy->FaultReported)
    {
        WolfSslAesGcmPolicy_Report(SOLIDSYSLOG_SEVERITY_ERROR, category, code);
        policy->FaultReported = true;
    }
}

/* One AES-256-GCM seal with a transient wolfCrypt context: the body is
 * encrypted in place, the header authenticated, and the tag written after the
 * nonce already in the trailer. Any step failing is one fault to the caller;
 * the context is freed whenever it was initialised. */
static bool WolfSslAesGcmPolicy_GcmEncrypt(const struct SolidSyslogSecurityRecord* record, const uint8_t* key)
{
    uint8_t* body = &record->Content[record->HeaderLength];
    uint16_t bodyLength = (uint16_t) (record->ContentLength - record->HeaderLength);

    Aes aes;
    bool ok = false;
    if (wc_AesInit(&aes, NULL, INVALID_DEVID) == 0)
    {
        ok = (wc_AesGcmSetKey(&aes, key, AES_GCM_POLICY_KEY_SIZE) == 0) &&
             (wc_AesGcmEncrypt(
                  &aes,
                  body,
                  body,
                  bodyLength,
                  record->Trailer,
                  AES_GCM_POLICY_NONCE_SIZE,
                  &record->Trailer[AES_GCM_POLICY_NONCE_SIZE],
                  AES_GCM_POLICY_TAG_SIZE,
                  record->Content,
                  record->HeaderLength
              ) == 0);
        wc_AesFree(&aes);
    }
    return ok;
}

static enum SolidSyslogSecurityPolicyOpenResult WolfSslAesGcmPolicy_OpenRecord(
    struct SolidSyslogSecurityPolicy* self,
    const struct SolidSyslogSecurityRecord* record
)
{
    struct SolidSyslogWolfSslAesGcmPolicy* policy = WolfSslAesGcmPolicy_SelfFromBase(self);

    enum SolidSyslogSecurityPolicyOpenResult result = SOLIDSYSLOG_SECURITY_POLICY_OPEN_COULD_NOT_RUN;
    uint8_t key[AES_GCM_POLICY_KEY_SIZE];
    if (WolfSslAesGcmPolicy_FetchKey(policy, key))
    {
        result = WolfSslAesGcmPolicy_GcmDecrypt(record, key);
        if (result == SOLIDSYSLOG_SECURITY_POLICY_OPEN_COULD_NOT_RUN)
        {
            WolfSslAesGcmPolicy_ReportFault(
                policy,
                SOLIDSYSLOG_CAT_SECURITY_POLICY_OPEN_FAILED,
                SOLIDSYSLOG_AES_GCM_POLICY_ERROR_DECRYPT_FAILED
            );
        }
        else
        {
            /* A verdict: the run of faults is over. */
            policy->FaultReported = false;
        }
    }
    wc_ForceZero(key, sizeof key);
    return result;
}

/* The verdict comes from wc_AesGcmDecrypt's return code: 0 is authentic,
 * AES_GCM_AUTH_E is tamper or wrong key (the expected rejection - silent, like
 * the HMAC verify), anything else a genuine wolfCrypt error, which reaches no
 * verdict. */
static enum SolidSyslogSecurityPolicyOpenResult WolfSslAesGcmPolicy_GcmDecrypt(
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
        if (wc_AesGcmSetKey(&aes, key, AES_GCM_POLICY_KEY_SIZE) == 0)
        {
            int verdict = wc_AesGcmDecrypt(
                &aes,
                body,
                body,
                bodyLength,
                record->Trailer,
                AES_GCM_POLICY_NONCE_SIZE,
                &record->Trailer[AES_GCM_POLICY_NONCE_SIZE],
                AES_GCM_POLICY_TAG_SIZE,
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
