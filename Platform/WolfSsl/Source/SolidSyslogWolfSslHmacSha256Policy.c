/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogWolfSslSettings.h"

#include "SolidSyslogWolfSslHmacSha256Policy.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <wolfssl/wolfcrypt/hmac.h>
#include <wolfssl/wolfcrypt/memory.h>
#include <wolfssl/wolfcrypt/types.h>

#include "SolidSyslogError.h"
#include "SolidSyslogErrorCategory.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogSecurityPolicyCategories.h"
#include "SolidSyslogSecurityPolicyDefinition.h"
#include "SolidSyslogTunables.h"
#include "SolidSyslogWolfSslHmacSha256PolicyErrors.h"
#include "SolidSyslogWolfSslHmacSha256PolicyPrivate.h"

const struct SolidSyslogErrorSource SolidSyslogWolfSslHmacSha256PolicyErrorSource = {
    "WolfSslHmacSha256Policy",
    SolidSyslogHmacSha256PolicyErrors_Name
};

enum
{
    HMAC_SHA256_TAG_SIZE = 32,
    /* RFC 2104 / NIST SP 800-107: an HMAC key should be at least the hash
     * output length. A shorter key (or an empty one) yields a well-defined but
     * cryptographically worthless MAC, so we reject it and fail closed. */
    HMAC_SHA256_MIN_KEY_SIZE = 32
};

static inline struct SolidSyslogWolfSslHmacSha256Policy* WolfSslHmacSha256Policy_SelfFromBase(
    struct SolidSyslogSecurityPolicy* base
);
static bool WolfSslHmacSha256Policy_SealRecord(
    struct SolidSyslogSecurityPolicy* self,
    const struct SolidSyslogSecurityRecord* record
);
static enum SolidSyslogSecurityPolicyOpenResult WolfSslHmacSha256Policy_OpenRecord(
    struct SolidSyslogSecurityPolicy* self,
    const struct SolidSyslogSecurityRecord* record
);
static bool WolfSslHmacSha256Policy_ComputeTag(
    struct SolidSyslogWolfSslHmacSha256Policy* policy,
    const uint8_t* data,
    uint16_t length,
    uint8_t* tagOut,
    uint16_t failureCategory
);
static bool WolfSslHmacSha256Policy_FetchKey(
    struct SolidSyslogWolfSslHmacSha256Policy* policy,
    uint8_t* keyOut,
    size_t* keyLengthOut
);
static bool WolfSslHmacSha256Policy_Hmac(
    const uint8_t* key,
    size_t keyLength,
    const uint8_t* data,
    uint16_t length,
    uint8_t* tagOut
);
static void WolfSslHmacSha256Policy_ReportFault(
    struct SolidSyslogWolfSslHmacSha256Policy* policy,
    uint16_t category,
    enum SolidSyslogHmacSha256PolicyErrors code
);
static inline bool WolfSslHmacSha256Policy_KeyLengthIsValid(size_t keyLength);
static inline bool WolfSslHmacSha256Policy_ConstantTimeEquals(const uint8_t* a, const uint8_t* b, size_t length);

void SolidSyslogWolfSslHmacSha256Policy_Initialise(
    struct SolidSyslogSecurityPolicy* base,
    const struct SolidSyslogWolfSslHmacSha256PolicyConfig* config
)
{
    struct SolidSyslogWolfSslHmacSha256Policy* self = WolfSslHmacSha256Policy_SelfFromBase(base);
    self->Base.TrailerSize = HMAC_SHA256_TAG_SIZE;
    self->Base.SealRecord = WolfSslHmacSha256Policy_SealRecord;
    self->Base.OpenRecord = WolfSslHmacSha256Policy_OpenRecord;
    self->Config = *config;
    self->FaultReported = false;
}

void SolidSyslogWolfSslHmacSha256Policy_Cleanup(struct SolidSyslogSecurityPolicy* base)
{
    /* No owned resources to release - the key is fetched on demand via the
     * GetKey callback and never stored on the instance. */
    (void) base;
}

static inline struct SolidSyslogWolfSslHmacSha256Policy* WolfSslHmacSha256Policy_SelfFromBase(
    struct SolidSyslogSecurityPolicy* base
)
{
    /* Base is the first member of the instance struct - see Private.h. */
    return (struct SolidSyslogWolfSslHmacSha256Policy*) base;
}

/* HMAC authenticates the whole content as one buffer - the header/body split
 * only matters to AEAD policies, so HeaderLength is ignored here. */
static bool WolfSslHmacSha256Policy_SealRecord(
    struct SolidSyslogSecurityPolicy* self,
    const struct SolidSyslogSecurityRecord* record
)
{
    uint8_t* tag = record->Trailer;
    return WolfSslHmacSha256Policy_ComputeTag(
        WolfSslHmacSha256Policy_SelfFromBase(self),
        record->Content,
        record->ContentLength,
        tag,
        SOLIDSYSLOG_CAT_SECURITY_POLICY_SEAL_FAILED
    );
}

/* Fetches the key on demand into a transient buffer, computes HMAC-SHA256 over
 * `data` into `tagOut`, then wipes the key buffer - the key never lingers
 * beyond a single computation. Returns false and reports the reason if the key
 * is unavailable or the HMAC computation fails: no tag, so a seal fails and an
 * open could not run. Shared by seal (writes the record tag) and verify
 * (recomputes for comparison). */
static bool WolfSslHmacSha256Policy_ComputeTag(
    struct SolidSyslogWolfSslHmacSha256Policy* policy,
    const uint8_t* data,
    uint16_t length,
    uint8_t* tagOut,
    uint16_t failureCategory
)
{
    uint8_t key[SOLIDSYSLOG_MAX_HMAC_KEY_SIZE];
    size_t keyLength = 0U;
    bool computed = false;
    if (WolfSslHmacSha256Policy_FetchKey(policy, key, &keyLength))
    {
        if (WolfSslHmacSha256Policy_Hmac(key, keyLength, data, length, tagOut))
        {
            /* A tag to seal with or verify against: the run of faults is over. */
            computed = true;
            policy->FaultReported = false;
        }
        else
        {
            WolfSslHmacSha256Policy_ReportFault(
                policy,
                failureCategory,
                SOLIDSYSLOG_HMAC_SHA256_POLICY_ERROR_HMAC_FAILED
            );
        }
    }
    /* Wipe the whole key buffer - the full region GetKey was handed, not just
     * the bytes written - so no key material lingers on the stack. */
    wc_ForceZero(key, sizeof key);
    return computed;
}

/* Fetches the key on demand and validates its length. Fails closed (and
 * reports) if the key is unavailable, or if its length falls outside
 * [HMAC_SHA256_MIN_KEY_SIZE, sizeof buffer]. */
static bool WolfSslHmacSha256Policy_FetchKey(
    struct SolidSyslogWolfSslHmacSha256Policy* policy,
    uint8_t* keyOut,
    size_t* keyLengthOut
)
{
    size_t keyLength = 0U;
    bool fetched = false;
    if (policy->Config.GetKey(policy->Config.KeyContext, keyOut, SOLIDSYSLOG_MAX_HMAC_KEY_SIZE, &keyLength))
    {
        if (WolfSslHmacSha256Policy_KeyLengthIsValid(keyLength))
        {
            *keyLengthOut = keyLength;
            fetched = true;
        }
        else
        {
            WolfSslHmacSha256Policy_ReportFault(
                policy,
                SOLIDSYSLOG_CAT_SECURITY_POLICY_KEY_UNAVAILABLE,
                SOLIDSYSLOG_HMAC_SHA256_POLICY_ERROR_KEY_TOO_SHORT
            );
        }
    }
    else
    {
        WolfSslHmacSha256Policy_ReportFault(
            policy,
            SOLIDSYSLOG_CAT_SECURITY_POLICY_KEY_UNAVAILABLE,
            SOLIDSYSLOG_HMAC_SHA256_POLICY_ERROR_KEY_UNAVAILABLE
        );
    }
    return fetched;
}

/* One HMAC-SHA256 over `data` with a transient wolfCrypt context. Any step
 * failing is one fault to the caller; the context is freed either way. */
static bool WolfSslHmacSha256Policy_Hmac(
    const uint8_t* key,
    size_t keyLength,
    const uint8_t* data,
    uint16_t length,
    uint8_t* tagOut
)
{
    Hmac hmac;
    bool ok = false;
    if (wc_HmacInit(&hmac, NULL, INVALID_DEVID) == 0)
    {
        ok = (wc_HmacSetKey(&hmac, WC_SHA256, key, (word32) keyLength) == 0) &&
             (wc_HmacUpdate(&hmac, data, length) == 0) && (wc_HmacFinal(&hmac, tagOut) == 0);
        wc_HmacFree(&hmac);
    }
    return ok;
}

/* The first fault in a run is the one reported. */
static void WolfSslHmacSha256Policy_ReportFault(
    struct SolidSyslogWolfSslHmacSha256Policy* policy,
    uint16_t category,
    enum SolidSyslogHmacSha256PolicyErrors code
)
{
    if (!policy->FaultReported)
    {
        WolfSslHmacSha256Policy_Report(SOLIDSYSLOG_SEVERITY_ERROR, category, code);
        policy->FaultReported = true;
    }
}

static inline bool WolfSslHmacSha256Policy_KeyLengthIsValid(size_t keyLength)
{
    return (keyLength >= (size_t) HMAC_SHA256_MIN_KEY_SIZE) && (keyLength <= (size_t) SOLIDSYSLOG_MAX_HMAC_KEY_SIZE);
}

static enum SolidSyslogSecurityPolicyOpenResult WolfSslHmacSha256Policy_OpenRecord(
    struct SolidSyslogSecurityPolicy* self,
    const struct SolidSyslogSecurityRecord* record
)
{
    uint8_t expected[HMAC_SHA256_TAG_SIZE];
    enum SolidSyslogSecurityPolicyOpenResult result = SOLIDSYSLOG_SECURITY_POLICY_OPEN_COULD_NOT_RUN;
    if (WolfSslHmacSha256Policy_ComputeTag(
            WolfSslHmacSha256Policy_SelfFromBase(self),
            record->Content,
            record->ContentLength,
            expected,
            SOLIDSYSLOG_CAT_SECURITY_POLICY_OPEN_FAILED
        ))
    {
        result = WolfSslHmacSha256Policy_ConstantTimeEquals(expected, record->Trailer, HMAC_SHA256_TAG_SIZE)
                     ? SOLIDSYSLOG_SECURITY_POLICY_OPEN_OPENED
                     : SOLIDSYSLOG_SECURITY_POLICY_OPEN_REJECTED;
    }
    return result;
}

static inline bool WolfSslHmacSha256Policy_ConstantTimeEquals(const uint8_t* a, const uint8_t* b, size_t length)
{
    /* Accumulate every byte difference so the loop runs the full length
     * regardless of where a mismatch occurs - no early exit, no timing oracle
     * on the tag comparison. */
    uint8_t difference = 0U;
    for (size_t index = 0U; index < length; index++)
    {
        difference |= (uint8_t) (a[index] ^ b[index]);
    }
    return difference == 0U;
}
