#include "SolidSyslogMbedTlsHmacSha256Policy.h"

#include <mbedtls/md.h>
#include <mbedtls/platform_util.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "SolidSyslogMbedTlsHmacSha256PolicyErrors.h"
#include "SolidSyslogMbedTlsHmacSha256PolicyPrivate.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogSecurityPolicyDefinition.h"
#include "SolidSyslogTunables.h"

enum
{
    HMAC_SHA256_TAG_SIZE = 32
};

static inline struct SolidSyslogMbedTlsHmacSha256Policy* MbedTlsHmacSha256Policy_SelfFromBase(
    struct SolidSyslogSecurityPolicy* base
);
static bool MbedTlsHmacSha256Policy_SealRecord(
    struct SolidSyslogSecurityPolicy* self,
    const struct SolidSyslogSecurityRecord* record
);
static bool MbedTlsHmacSha256Policy_OpenRecord(
    struct SolidSyslogSecurityPolicy* self,
    const struct SolidSyslogSecurityRecord* record
);
static bool MbedTlsHmacSha256Policy_ComputeTag(
    struct SolidSyslogMbedTlsHmacSha256Policy* policy,
    const uint8_t* data,
    uint16_t length,
    uint8_t* tagOut
);
static inline bool MbedTlsHmacSha256Policy_ConstantTimeEquals(const uint8_t* a, const uint8_t* b, size_t length);

void MbedTlsHmacSha256Policy_Initialise(
    struct SolidSyslogSecurityPolicy* base,
    const struct SolidSyslogMbedTlsHmacSha256PolicyConfig* config
)
{
    struct SolidSyslogMbedTlsHmacSha256Policy* self = MbedTlsHmacSha256Policy_SelfFromBase(base);
    self->Base.TrailerSize = HMAC_SHA256_TAG_SIZE;
    self->Base.SealRecord = MbedTlsHmacSha256Policy_SealRecord;
    self->Base.OpenRecord = MbedTlsHmacSha256Policy_OpenRecord;
    self->Config = *config;
}

void MbedTlsHmacSha256Policy_Cleanup(struct SolidSyslogSecurityPolicy* base)
{
    /* No owned resources to release — the key is fetched on demand via the
     * GetKey callback and never stored on the instance. */
    (void) base;
}

static inline struct SolidSyslogMbedTlsHmacSha256Policy* MbedTlsHmacSha256Policy_SelfFromBase(
    struct SolidSyslogSecurityPolicy* base
)
{
    /* Base is the first member of the instance struct — see Private.h. */
    return (struct SolidSyslogMbedTlsHmacSha256Policy*) base;
}

/* HMAC authenticates the whole content as one buffer — the header/body split
 * only matters to AEAD policies, so HeaderLength is ignored here. */
static bool MbedTlsHmacSha256Policy_SealRecord(
    struct SolidSyslogSecurityPolicy* self,
    const struct SolidSyslogSecurityRecord* record
)
{
    /* Bind the trailer to a local before passing it as the writable tag
     * destination — same shape the AES-GCM sibling uses for its nonce/tag. */
    uint8_t* tag = record->Trailer;
    return MbedTlsHmacSha256Policy_ComputeTag(
        MbedTlsHmacSha256Policy_SelfFromBase(self),
        record->Content,
        record->ContentLength,
        tag
    );
}

/* Fetches the key on demand into a transient buffer, computes HMAC-SHA256 over
 * `data` into `tagOut`, then wipes the key buffer — the key never lingers
 * beyond a single computation. Returns false (fail closed) and reports the
 * reason if the key is unavailable or the HMAC computation fails. Shared by
 * seal (writes the record tag) and verify (recomputes for comparison). */
static bool MbedTlsHmacSha256Policy_ComputeTag(
    struct SolidSyslogMbedTlsHmacSha256Policy* policy,
    const uint8_t* data,
    uint16_t length,
    uint8_t* tagOut
)
{
    uint8_t key[SOLIDSYSLOG_MAX_HMAC_KEY_SIZE];
    size_t keyLength = 0;
    bool computed = false;
    if (policy->Config.GetKey(policy->Config.KeyContext, key, sizeof key, &keyLength))
    {
        const mbedtls_md_info_t* sha256 = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
        if (mbedtls_md_hmac(sha256, key, keyLength, data, length, tagOut) == 0)
        {
            computed = true;
        }
        else
        {
            MbedTlsHmacSha256Policy_Report(SOLIDSYSLOG_SEVERITY_ERROR, MBEDTLSHMACSHA256POLICY_ERROR_HMAC_FAILED);
        }
    }
    else
    {
        MbedTlsHmacSha256Policy_Report(SOLIDSYSLOG_SEVERITY_ERROR, MBEDTLSHMACSHA256POLICY_ERROR_KEY_UNAVAILABLE);
    }
    /* Wipe the whole key buffer — the full region GetKey was handed, not just
     * the bytes written — so no key material lingers on the stack. */
    mbedtls_platform_zeroize(key, sizeof key);
    return computed;
}

static bool MbedTlsHmacSha256Policy_OpenRecord(
    struct SolidSyslogSecurityPolicy* self,
    const struct SolidSyslogSecurityRecord* record
)
{
    uint8_t expected[HMAC_SHA256_TAG_SIZE];
    bool verified = false;
    if (MbedTlsHmacSha256Policy_ComputeTag(
            MbedTlsHmacSha256Policy_SelfFromBase(self),
            record->Content,
            record->ContentLength,
            expected
        ))
    {
        verified = MbedTlsHmacSha256Policy_ConstantTimeEquals(expected, record->Trailer, HMAC_SHA256_TAG_SIZE);
    }
    return verified;
}

static inline bool MbedTlsHmacSha256Policy_ConstantTimeEquals(const uint8_t* a, const uint8_t* b, size_t length)
{
    /* Accumulate every byte difference so the loop runs the full length
     * regardless of where a mismatch occurs — no early exit, no timing oracle
     * on the tag comparison. */
    uint8_t difference = 0U;
    for (size_t index = 0; index < length; index++)
    {
        difference |= (uint8_t) (a[index] ^ b[index]);
    }
    return difference == 0U;
}
