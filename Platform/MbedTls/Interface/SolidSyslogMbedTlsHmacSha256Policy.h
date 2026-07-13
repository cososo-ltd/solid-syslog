/** @file
 *  HMAC-SHA256 integrity SecurityPolicy via Mbed TLS, for a store that must
 *  detect tampering of records at rest (authentication only, no confidentiality).
 *
 *  What the policy does through its vtable is the substance:
 *
 *  - Seal authenticates the whole record content as one buffer (the header/body
 *    split matters only to AEAD policies, so HeaderLength is ignored) and writes
 *    the 32-byte tag into the record trailer. It is keyed and fails closed — if
 *    the key is unavailable or shorter than the SHA-256 output (32 bytes), Seal
 *    returns false and nothing is stored.
 *  - Open recomputes the tag and compares it to the stored one in constant time
 *    (no early exit, no timing oracle). A mismatch returns false silently — the
 *    expected tamper verdict — and is not reported.
 *
 *  The key is fetched on demand via GetKey and wiped after every computation — it
 *  is never stored on the instance. */
#ifndef SOLIDSYSLOGMBEDTLSHMACSHA256POLICY_H
#define SOLIDSYSLOGMBEDTLSHMACSHA256POLICY_H

#include "ExternC.h"
#include "SolidSyslogKeyFunction.h"

struct SolidSyslogSecurityPolicy;

EXTERN_C_BEGIN

    struct SolidSyslogMbedTlsHmacSha256PolicyConfig
    {
        SolidSyslogKeyFunction GetKey; /**< Fetches the HMAC key on demand; required. Must report at least 32
                                            bytes (the SHA-256 output size) or seal/verify fails closed. */
        void* KeyContext; /**< Passed back to GetKey unchanged; NULL is fine. */
    };

    /** Draw an HMAC policy from the pool. Bad config (NULL GetKey) or an exhausted
     *  pool falls back to the shared NullSecurityPolicy. */
    struct SolidSyslogSecurityPolicy* SolidSyslogMbedTlsHmacSha256Policy_Create(
        const struct SolidSyslogMbedTlsHmacSha256PolicyConfig* config
    );
    /** Release the pool slot; the policy owns no resources beyond the slot (the
     *  key is fetched per-call). */
    void SolidSyslogMbedTlsHmacSha256Policy_Destroy(struct SolidSyslogSecurityPolicy * base);

EXTERN_C_END

#endif /* SOLIDSYSLOGMBEDTLSHMACSHA256POLICY_H */
