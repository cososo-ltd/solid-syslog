/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogWolfSslSettings.h"

#include "SolidSyslogWolfSslAesGcmPolicy.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
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

static inline struct SolidSyslogWolfSslAesGcmPolicy* WolfSslAesGcmPolicy_SelfFromBase(
    struct SolidSyslogSecurityPolicy* base
);
static bool WolfSslAesGcmPolicy_SealRecord(
    struct SolidSyslogSecurityPolicy* base,
    const struct SolidSyslogSecurityRecord* record
);
static enum SolidSyslogSecurityPolicyOpenResult WolfSslAesGcmPolicy_OpenRecord(
    struct SolidSyslogSecurityPolicy* base,
    const struct SolidSyslogSecurityRecord* record
);
static bool WolfSslAesGcmPolicy_FetchKey(struct SolidSyslogWolfSslAesGcmPolicy* self, uint8_t* keyOut);
static void WolfSslAesGcmPolicy_ReportFault(
    struct SolidSyslogWolfSslAesGcmPolicy* self,
    uint16_t category,
    enum SolidSyslogAesGcmPolicyErrors code
);

void SolidSyslogWolfSslAesGcmPolicy_Initialise(
    struct SolidSyslogSecurityPolicy* base,
    const struct SolidSyslogWolfSslAesGcmPolicyConfig* config
)
{
    struct SolidSyslogWolfSslAesGcmPolicy* self = WolfSslAesGcmPolicy_SelfFromBase(base);
    self->Base.TrailerSize = SOLIDSYSLOG_WOLFSSL_AES_GCM_POLICY_TRAILER_SIZE;
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

/* Seals in place: draws a fresh nonce into Trailer[0..SOLIDSYSLOG_WOLFSSL_AES_GCM_POLICY_NONCE_SIZE), then
 * GcmEncrypt encrypts the body (Content past HeaderLength) and writes the tag to
 * the remaining trailer bytes, authenticating the header (Content[0..HeaderLength))
 * as associated data. Fetches the key on demand and wipes it before returning. */
static bool WolfSslAesGcmPolicy_SealRecord(
    struct SolidSyslogSecurityPolicy* base,
    const struct SolidSyslogSecurityRecord* record
)
{
    struct SolidSyslogWolfSslAesGcmPolicy* self = WolfSslAesGcmPolicy_SelfFromBase(base);

    bool sealed = false;
    uint8_t key[SOLIDSYSLOG_WOLFSSL_AES_GCM_POLICY_KEY_SIZE];
    if (WolfSslAesGcmPolicy_FetchKey(self, key))
    {
        /* Draw the per-record nonce straight into the trailer; GcmEncrypt reads
         * it back from there and writes the tag immediately after it. */
        if (wc_RNG_GenerateBlock(self->Config.Rng, record->Trailer, SOLIDSYSLOG_WOLFSSL_AES_GCM_POLICY_NONCE_SIZE) == 0)
        {
            if (SolidSyslogWolfSslAesGcmPolicy_GcmEncrypt(record, key))
            {
                sealed = true;
                self->FaultReported = false;
            }
            else
            {
                WolfSslAesGcmPolicy_ReportFault(
                    self,
                    SOLIDSYSLOG_CAT_SECURITY_POLICY_SEAL_FAILED,
                    SOLIDSYSLOG_AES_GCM_POLICY_ERROR_ENCRYPT_FAILED
                );
            }
        }
        else
        {
            WolfSslAesGcmPolicy_ReportFault(
                self,
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
static bool WolfSslAesGcmPolicy_FetchKey(struct SolidSyslogWolfSslAesGcmPolicy* self, uint8_t* keyOut)
{
    size_t keyLength = 0U;
    bool fetched =
        self->Config.GetKey(self->Config.KeyContext, keyOut, SOLIDSYSLOG_WOLFSSL_AES_GCM_POLICY_KEY_SIZE, &keyLength) &&
        (keyLength == (size_t) SOLIDSYSLOG_WOLFSSL_AES_GCM_POLICY_KEY_SIZE);
    if (!fetched)
    {
        WolfSslAesGcmPolicy_ReportFault(
            self,
            SOLIDSYSLOG_CAT_SECURITY_POLICY_KEY_UNAVAILABLE,
            SOLIDSYSLOG_AES_GCM_POLICY_ERROR_KEY_UNAVAILABLE
        );
    }
    return fetched;
}

/* The first fault in a run is the one reported. */
static void WolfSslAesGcmPolicy_ReportFault(
    struct SolidSyslogWolfSslAesGcmPolicy* self,
    uint16_t category,
    enum SolidSyslogAesGcmPolicyErrors code
)
{
    if (!self->FaultReported)
    {
        WolfSslAesGcmPolicy_Report(SOLIDSYSLOG_SEVERITY_ERROR, category, code);
        self->FaultReported = true;
    }
}

static enum SolidSyslogSecurityPolicyOpenResult WolfSslAesGcmPolicy_OpenRecord(
    struct SolidSyslogSecurityPolicy* base,
    const struct SolidSyslogSecurityRecord* record
)
{
    struct SolidSyslogWolfSslAesGcmPolicy* self = WolfSslAesGcmPolicy_SelfFromBase(base);

    enum SolidSyslogSecurityPolicyOpenResult result = SOLIDSYSLOG_SECURITY_POLICY_OPEN_COULD_NOT_RUN;
    uint8_t key[SOLIDSYSLOG_WOLFSSL_AES_GCM_POLICY_KEY_SIZE];
    if (WolfSslAesGcmPolicy_FetchKey(self, key))
    {
        result = SolidSyslogWolfSslAesGcmPolicy_GcmDecrypt(record, key);
        if (result == SOLIDSYSLOG_SECURITY_POLICY_OPEN_COULD_NOT_RUN)
        {
            WolfSslAesGcmPolicy_ReportFault(
                self,
                SOLIDSYSLOG_CAT_SECURITY_POLICY_OPEN_FAILED,
                SOLIDSYSLOG_AES_GCM_POLICY_ERROR_DECRYPT_FAILED
            );
        }
        else
        {
            /* A verdict: the run of faults is over. */
            self->FaultReported = false;
        }
    }
    wc_ForceZero(key, sizeof key);
    return result;
}
