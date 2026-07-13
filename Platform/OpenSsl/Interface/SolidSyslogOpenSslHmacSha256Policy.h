/** @file
 *  A keyed HMAC-SHA256 security policy (OpenSSL reference integration) that
 *  authenticates each stored record — tamper-detection for store-and-forward
 *  without encryption.
 *
 *  What the policy does through its vtable is the substance:
 *
 *  - SealRecord computes HMAC-SHA256 over the whole record content (the
 *    header/body split matters only to AEAD policies, so it is ignored here) and
 *    writes the 32-byte tag into the record trailer. The key is fetched on demand
 *    via GetKey and wiped after the computation.
 *  - OpenRecord recomputes the tag and compares it to the stored trailer in
 *    constant time (no early exit, no timing oracle); a mismatch returns false.
 *  - It fails closed — returns false, so nothing is sealed / verified — if the
 *    key is unavailable, shorter than 32 bytes (RFC 2104 / NIST SP 800-107: an
 *    HMAC key should be at least the hash output length), or the HMAC computation
 *    fails. */
#ifndef SOLIDSYSLOGOPENSSLHMACSHA256POLICY_H
#define SOLIDSYSLOGOPENSSLHMACSHA256POLICY_H

#include "ExternC.h"
#include "SolidSyslogKeyFunction.h"

struct SolidSyslogSecurityPolicy;

EXTERN_C_BEGIN

    /** Supplies the key SolidSyslogOpenSslHmacSha256Policy seals and verifies with. */
    struct SolidSyslogOpenSslHmacSha256PolicyConfig
    {
        SolidSyslogKeyFunction GetKey; /**< Fetches the HMAC key on demand; required. Must report at least 32
                                        *  bytes (SHA-256 output size) or seal/verify fails closed. */
        void* KeyContext; /**< Passed back to GetKey unchanged; NULL is fine. */
    };

    /** Draw a policy from the pool. A NULL config or NULL GetKey (bad config), or
     *  an exhausted pool (default size 1), falls back to the shared
     *  NullSecurityPolicy. */
    struct SolidSyslogSecurityPolicy* SolidSyslogOpenSslHmacSha256Policy_Create(
        const struct SolidSyslogOpenSslHmacSha256PolicyConfig* config
    );
    /** Release the pool slot. The policy owns no resources — the key is never
     *  stored on the instance — so this only frees the slot. */
    void SolidSyslogOpenSslHmacSha256Policy_Destroy(struct SolidSyslogSecurityPolicy * base);

EXTERN_C_END

#endif /* SOLIDSYSLOGOPENSSLHMACSHA256POLICY_H */
