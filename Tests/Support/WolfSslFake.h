#ifndef WOLFSSLFAKE_H
#define WOLFSSLFAKE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "SolidSyslogExternC.h"

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

SOLIDSYSLOG_EXTERN_C_END

#endif /* WOLFSSLFAKE_H */
