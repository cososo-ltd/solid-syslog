#ifndef WOLFSSLFAKE_H
#define WOLFSSLFAKE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "SolidSyslogExternC.h"

struct WC_RNG;

SOLIDSYSLOG_EXTERN_C_BEGIN

    /* Resets every counter and captured-arg field to its initial state.
     * Call from TEST_GROUP::setup() so each test starts from a clean slate. */
    void WolfSslFake_Reset(void);

    /* wolfCrypt HMAC (wc_Hmac*) - link-time double for the at-rest HMAC
     * SecurityPolicy. Captures the key and the message, and writes a
     * deterministic tag from WolfSslFake_ComputeExpectedTag so a test can
     * predict it; NOT an HMAC. Genuine HMAC-SHA256 correctness is the
     * WolfSslIntegration suite's job. */
    int WolfSslFake_HmacInitCallCount(void);
    int WolfSslFake_HmacSetKeyCallCount(void);
    int WolfSslFake_LastHmacType(void);
    const uint8_t* WolfSslFake_LastHmacKey(void);
    size_t WolfSslFake_LastHmacKeyLen(void);
    const uint8_t* WolfSslFake_LastHmacInput(void);
    size_t WolfSslFake_LastHmacInputLen(void);
    int WolfSslFake_HmacFreeCallCount(void);

    /* Step of the HMAC sequence to fail, so a test can pin the error path of
     * each fallible wolfCrypt call. */
    enum WolfSslFakeHmacStep
    {
        WOLFSSLFAKE_HMAC_STEP_NONE = 0,
        WOLFSSLFAKE_HMAC_STEP_INIT,
        WOLFSSLFAKE_HMAC_STEP_SETKEY,
        WOLFSSLFAKE_HMAC_STEP_UPDATE,
        WOLFSSLFAKE_HMAC_STEP_FINAL
    };

    void WolfSslFake_SetHmacStepFails(enum WolfSslFakeHmacStep step);

    /* The deterministic tag the HMAC double writes: a hash of key then input,
     * so a test can compute what the policy should have written. */
    void WolfSslFake_ComputeExpectedTag(
        const uint8_t* key,
        size_t keyLength,
        const uint8_t* input,
        size_t inputLength,
        uint8_t* tagOut
    );

    /* wc_ForceZero - the wipe the policies apply to key material. */
    int WolfSslFake_ForceZeroCallCount(void);
    const void* WolfSslFake_LastForceZeroBuf(void);
    size_t WolfSslFake_LastForceZeroLen(void);

    /* AES-256-GCM (wc_AesGcm*) + the wolfCrypt RNG nonce source - drive the
     * at-rest AES-GCM SecurityPolicy without linking the real library. A
     * capture-and-canned-return double, NOT a cipher: wc_AesGcmEncrypt /
     * wc_AesGcmDecrypt capture their arguments, copy the body through unchanged,
     * and return canned results. Genuine AES-256-GCM correctness (round-trip,
     * tamper, wrong-key) is the WolfSslIntegration suite's job. */
    int WolfSslFake_AesInitCallCount(void);
    int WolfSslFake_AesFreeCallCount(void);
    int WolfSslFake_GcmSealCount(void); /* wc_AesGcmEncrypt calls (seals) */
    int WolfSslFake_GcmOpenCount(void); /* wc_AesGcmDecrypt calls (opens) */
    const uint8_t* WolfSslFake_LastGcmKey(void); /* from wc_AesGcmSetKey */
    size_t WolfSslFake_LastGcmKeyLen(void);
    const uint8_t* WolfSslFake_LastGcmNonce(void); /* 12 bytes */
    const uint8_t* WolfSslFake_LastGcmAad(void);
    size_t WolfSslFake_LastGcmAadLen(void);
    const uint8_t* WolfSslFake_LastGcmPlaintext(void); /* body bytes handed to encrypt */
    size_t WolfSslFake_LastGcmPlaintextLen(void);

    /* Step of the seal/open sequence to fail, so a test can pin the error path of
     * each fallible wolfCrypt call: init, setkey, encrypt (seal), decrypt (open,
     * genuine error - distinct from the tamper verdict below). */
    enum WolfSslFakeGcmStep
    {
        WOLFSSLFAKE_GCM_STEP_NONE = 0,
        WOLFSSLFAKE_GCM_STEP_INIT,
        WOLFSSLFAKE_GCM_STEP_SETKEY,
        WOLFSSLFAKE_GCM_STEP_ENCRYPT,
        WOLFSSLFAKE_GCM_STEP_DECRYPT
    };

    void WolfSslFake_SetGcmStepFails(enum WolfSslFakeGcmStep step);
    /* Makes wc_AesGcmDecrypt return AES_GCM_AUTH_E - the tamper / wrong-key
     * verdict the adapter must surface silently (no report). */
    void WolfSslFake_SetGcmAuthFails(bool fails);

    /* A WC_RNG a test can wire without including wolfSSL's random.h, which is
     * not clean under C++ -Wpedantic. The double only ever captures its address. */
    struct WC_RNG* WolfSslFake_Rng(void);

    /* wc_RNG_GenerateBlock - the policy's per-record nonce source. Fills
     * 0xA0, 0xA1, ... so a test can assert the bytes reached the trailer. */
    int WolfSslFake_RngGenerateBlockCallCount(void);
    const void* WolfSslFake_LastRngGenerateBlockContext(void);
    const void* WolfSslFake_LastRngGenerateBlockBuf(void);
    size_t WolfSslFake_LastRngGenerateBlockLen(void);
    void WolfSslFake_SetRngGenerateBlockFails(bool fails);

SOLIDSYSLOG_EXTERN_C_END

#endif /* WOLFSSLFAKE_H */
