/** @file
 *  A keyed AES-256-GCM security policy (OpenSSL reference integration) that
 *  encrypts and authenticates each stored record — confidentiality plus
 *  tamper-detection for store-and-forward.
 *
 *  What the policy does through its vtable is the substance:
 *
 *  - SealRecord encrypts the body in place and authenticates the header as
 *    associated data (the header stays in clear), writing a fresh random nonce
 *    and the GCM tag into the record trailer (nonce ‖ tag, 28 bytes). The key is
 *    fetched on demand via GetKey and wiped before returning. It fails closed —
 *    returns false so nothing is stored — if the key is unavailable or not
 *    exactly 32 bytes (AES-256 admits no other length), if nonce generation
 *    fails, or on any encrypt error.
 *  - OpenRecord decrypts the body and verifies the tag over header + ciphertext.
 *    A tag mismatch is the expected tamper-detected outcome and returns false
 *    silently; only a genuine OpenSSL error is reported. The key is likewise
 *    fetched on demand and wiped. */
#ifndef SOLIDSYSLOGOPENSSLAESGCMPOLICY_H
#define SOLIDSYSLOGOPENSSLAESGCMPOLICY_H

#include "ExternC.h"
#include "SolidSyslogKeyFunction.h"

struct SolidSyslogSecurityPolicy;

EXTERN_C_BEGIN

    /** Supplies the key SolidSyslogOpenSslAesGcmPolicy seals and opens with. */
    struct SolidSyslogOpenSslAesGcmPolicyConfig
    {
        SolidSyslogKeyFunction GetKey; /**< Fetches the 32-byte AES-256 key on demand; required. A key that is
                                        *  unavailable or not exactly 32 bytes fails the seal/open closed. */
        void* KeyContext; /**< Passed back to GetKey unchanged; NULL is fine. */
    };

    /** Draw a policy from the pool. A NULL config or NULL GetKey (bad config), or
     *  an exhausted pool (default size 1), falls back to the shared
     *  NullSecurityPolicy. */
    struct SolidSyslogSecurityPolicy* SolidSyslogOpenSslAesGcmPolicy_Create(
        const struct SolidSyslogOpenSslAesGcmPolicyConfig* config
    );
    /** Release the pool slot. The policy owns no resources — the key is never
     *  stored on the instance — so this only frees the slot. */
    void SolidSyslogOpenSslAesGcmPolicy_Destroy(struct SolidSyslogSecurityPolicy * base);

EXTERN_C_END

#endif /* SOLIDSYSLOGOPENSSLAESGCMPOLICY_H */
