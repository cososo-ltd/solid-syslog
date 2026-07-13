/** @file
 *  AES-256-GCM confidentiality-and-integrity SecurityPolicy via Mbed TLS, for a
 *  store that must encrypt records at rest.
 *
 *  What the policy does through its vtable is the substance:
 *
 *  - Seal encrypts a record in place: draws a fresh 12-byte nonce from the
 *    caller's CTR-DRBG, encrypts the body (Content past HeaderLength),
 *    authenticates the header as associated data, and writes nonce‖tag into the
 *    record trailer. It is keyed and fails closed — if the key is unavailable or
 *    not exactly 32 bytes, Seal returns false and nothing is stored.
 *  - Open reverses it: decrypts the body and verifies the tag over the header and
 *    ciphertext. A tag mismatch (tamper, or wrong key) is the expected rejection
 *    and returns false silently; only a genuine mbedTLS fault is reported.
 *
 *  The key is fetched on demand via GetKey and wiped after every operation — it
 *  is never stored on the instance. */
#ifndef SOLIDSYSLOGMBEDTLSAESGCMPOLICY_H
#define SOLIDSYSLOGMBEDTLSAESGCMPOLICY_H

#include "ExternC.h"
#include "SolidSyslogKeyFunction.h"

struct SolidSyslogSecurityPolicy;
struct mbedtls_ctr_drbg_context;

EXTERN_C_BEGIN

    struct SolidSyslogMbedTlsAesGcmPolicyConfig
    {
        SolidSyslogKeyFunction GetKey; /**< Fetches the 32-byte AES-256 key on demand; required. Any other
                                            reported length fails the operation closed. */
        void* KeyContext; /**< Passed back to GetKey unchanged; NULL is fine. */
        struct mbedtls_ctr_drbg_context* Rng; /**< Seeded CTR-DRBG each record's 12-byte nonce is drawn from;
                                            required and caller-owned. Injected because mbedTLS has no
                                            context-free RNG, unlike the OpenSSL sibling's RAND_bytes. */
    };

    /** Draw an AES-GCM policy from the pool. Bad config (NULL GetKey or NULL Rng)
     *  or an exhausted pool falls back to the shared NullSecurityPolicy. */
    struct SolidSyslogSecurityPolicy* SolidSyslogMbedTlsAesGcmPolicy_Create(
        const struct SolidSyslogMbedTlsAesGcmPolicyConfig* config
    );
    /** Release the pool slot; the policy owns no resources beyond the slot (the
     *  key is fetched per-call, the CTR-DRBG is caller-owned). */
    void SolidSyslogMbedTlsAesGcmPolicy_Destroy(struct SolidSyslogSecurityPolicy * base);

EXTERN_C_END

#endif /* SOLIDSYSLOGMBEDTLSAESGCMPOLICY_H */
