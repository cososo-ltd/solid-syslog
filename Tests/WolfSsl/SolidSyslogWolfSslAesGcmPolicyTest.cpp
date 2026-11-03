#include <cstring>

#include "CppUTest/TestHarness.h"

extern "C"
{
#include "ConfigLockFake.h"
#include "ErrorHandlerFake.h"
#include "WolfSslFake.h"
#include "SolidSyslogWolfSslAesGcmPolicy.h"
#include "SolidSyslogWolfSslAesGcmPolicyErrors.h"
#include "SolidSyslogNullSecurityPolicy.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogSecurityPolicyDefinition.h"
#include "SolidSyslogTunables.h"
}

#include "SolidSyslogErrorCategory.h"
#include "SolidSyslogSecurityPolicyCategories.h"
#include "SolidSyslogAesGcmPolicyErrors.h"
#include "SolidSyslogError.h"
#include "TestUtils.h"

using namespace CososoTesting;

/* The CHECK_* macros below call seal() and open() on the Seal fixture, so the
   tests that expand them are declared for include-what-you-use. */
class TEST_SolidSyslogWolfSslAesGcmPolicySeal_SealReportsErrorWhenInitialisingTheContextFails_Test;
class TEST_SolidSyslogWolfSslAesGcmPolicySeal_SealReportsErrorWhenSettingKeyFails_Test;
class TEST_SolidSyslogWolfSslAesGcmPolicySeal_SealReportsErrorWhenEncryptingFails_Test;
class TEST_SolidSyslogWolfSslAesGcmPolicySeal_OpenReportsErrorWhenInitialisingTheContextFails_Test;
class TEST_SolidSyslogWolfSslAesGcmPolicySeal_OpenReportsErrorWhenSettingKeyFails_Test;
class TEST_SolidSyslogWolfSslAesGcmPolicySeal_OpenReportsErrorWhenDecryptingFails_Test;

enum
{
    AES_GCM_POLICY_KEY_SIZE = 32,
    AES_GCM_POLICY_NONCE_SIZE = 12,
    AES_GCM_POLICY_TAG_SIZE = 16,
    AES_GCM_POLICY_TRAILER_SIZE = AES_GCM_POLICY_NONCE_SIZE + AES_GCM_POLICY_TAG_SIZE,
    TEST_HEADER_LEN = 4,
    TEST_BODY_LEN = 8,
    TEST_CONTENT_LEN = TEST_HEADER_LEN + TEST_BODY_LEN,
    TEST_KEY_BYTE = 0x2B
};

/* The buffer + capacity the policy handed to GetKey on the most recent fetch. */
static const uint8_t* lastGetKeyBuffer = nullptr;
static size_t lastGetKeyCapacity = 0U;

/* Settable key accessor. `keyAvailable` false -> GetKey fails; `keyByte` sets the
 * key contents (vary it to forge a wrong key); `keyLengthToReport` lets a test
 * report a non-32-byte key. */
static bool keyAvailable = true;
static uint8_t keyByte = TEST_KEY_BYTE;
static size_t keyLengthToReport = AES_GCM_POLICY_KEY_SIZE;

static bool TestGetKey(void* context, uint8_t* keyOut, size_t capacity, size_t* keyLengthOut)
{
    (void) context;
    lastGetKeyBuffer = keyOut;
    lastGetKeyCapacity = capacity;
    if (!keyAvailable)
    {
        return false;
    }
    size_t written = (keyLengthToReport < capacity) ? keyLengthToReport : capacity;
    memset(keyOut, keyByte, written);
    *keyLengthOut = keyLengthToReport;
    return true;
}

#define CHECK_REPORTED_ERROR(severity, expectedCategory, code) \
    CHECK_ERROR_REPORTED_ONCE((severity), &SolidSyslogWolfSslAesGcmPolicyErrorSource, (expectedCategory), (code))

#define CHECK_IS_NULL_FALLBACK(handle) POINTERS_EQUAL(SolidSyslogNullSecurityPolicy_Get(), (handle))

/* One macro per direction so each fallible wolfCrypt GCM call's failure path reads
 * as a one-line test: seal/open must fail closed and report once. Used only
 * inside the Seal fixture (they reference its seal()/open() helpers). */
#define CHECK_SEAL_REPORTS_ENCRYPT_FAILURE_AT(step)         \
    {                                                       \
        ErrorHandlerFake_Install(nullptr);                  \
        WolfSslFake_SetGcmStepFails(step);                  \
        CHECK_FALSE(seal());                                \
        CHECK_REPORTED_ERROR(                               \
            SOLIDSYSLOG_SEVERITY_ERROR,                     \
            SOLIDSYSLOG_CAT_SECURITY_POLICY_SEAL_FAILED,    \
            SOLIDSYSLOG_AES_GCM_POLICY_ERROR_ENCRYPT_FAILED \
        );                                                  \
    }

#define CHECK_OPEN_REPORTS_DECRYPT_FAILURE_AT(step)                          \
    {                                                                        \
        ErrorHandlerFake_Install(nullptr);                                   \
        WolfSslFake_SetGcmStepFails(step);                                   \
        LONGS_EQUAL(SOLIDSYSLOG_SECURITY_POLICY_OPEN_COULD_NOT_RUN, open()); \
        CHECK_REPORTED_ERROR(                                                \
            SOLIDSYSLOG_SEVERITY_ERROR,                                      \
            SOLIDSYSLOG_CAT_SECURITY_POLICY_OPEN_FAILED,                     \
            SOLIDSYSLOG_AES_GCM_POLICY_ERROR_DECRYPT_FAILED                  \
        );                                                                   \
    }

// clang-format off
TEST_BASE(WolfSslAesGcmPolicyTestBase)
{
    struct SolidSyslogWolfSslAesGcmPolicyConfig config = {};

    void armConfig()
    {
        WolfSslFake_Reset();
        lastGetKeyBuffer   = nullptr;
        lastGetKeyCapacity = 0;
        keyAvailable       = true;
        keyByte            = TEST_KEY_BYTE;
        keyLengthToReport  = AES_GCM_POLICY_KEY_SIZE;
        config.GetKey      = TestGetKey;
        config.KeyContext  = nullptr;
        config.Rng         = WolfSslFake_Rng();
    }
};

TEST_GROUP_BASE(SolidSyslogWolfSslAesGcmPolicy, WolfSslAesGcmPolicyTestBase)
{
    struct SolidSyslogSecurityPolicy* pooled[SOLIDSYSLOG_AES_GCM_POLICY_POOL_SIZE] = {};
    struct SolidSyslogSecurityPolicy* overflow = nullptr;

    void setup() override
    {
        armConfig();
    }

    void teardown() override
    {
        for (auto* handle : pooled)
        {
            if (handle != nullptr)
            {
                SolidSyslogWolfSslAesGcmPolicy_Destroy(handle);
            }
        }
        if (overflow != nullptr)
        {
            SolidSyslogWolfSslAesGcmPolicy_Destroy(overflow);
        }
        ConfigLockFake_Uninstall();
    }

    void FillPool()
    {
        for (auto*& slot : pooled)
        {
            slot = SolidSyslogWolfSslAesGcmPolicy_Create(&config);
        }
    }
};

TEST_GROUP_BASE(SolidSyslogWolfSslAesGcmPolicySeal, WolfSslAesGcmPolicyTestBase)
{
    struct SolidSyslogSecurityPolicy* policy = nullptr;
    uint8_t content[TEST_CONTENT_LEN] = {};
    uint8_t originalBody[TEST_BODY_LEN] = {};
    uint8_t trailer[AES_GCM_POLICY_TRAILER_SIZE] = {};

    void setup() override
    {
        armConfig();
        policy = SolidSyslogWolfSslAesGcmPolicy_Create(&config);

        static const uint8_t header[TEST_HEADER_LEN] = {0xA5, 0x5A, 0x08, 0x00};
        static const uint8_t body[TEST_BODY_LEN] = {'p', 'l', 'a', 'i', 'n', 't', 'x', 't'};
        memcpy(content, header, TEST_HEADER_LEN);
        memcpy(&content[TEST_HEADER_LEN], body, TEST_BODY_LEN);
        memcpy(originalBody, body, TEST_BODY_LEN);
    }

    void teardown() override
    {
        SolidSyslogWolfSslAesGcmPolicy_Destroy(policy);
        ConfigLockFake_Uninstall();
    }

    bool seal()
    {
        struct SolidSyslogSecurityRecord rec = {content, TEST_CONTENT_LEN, TEST_HEADER_LEN, trailer};
        return policy->SealRecord(policy, &rec);
    }

    enum SolidSyslogSecurityPolicyOpenResult open()
    {
        struct SolidSyslogSecurityRecord rec = {content, TEST_CONTENT_LEN, TEST_HEADER_LEN, trailer};
        return policy->OpenRecord(policy, &rec);
    }
};

// clang-format on

TEST(SolidSyslogWolfSslAesGcmPolicy, CreateReturnsHandleDistinctFromFallback)
{
    struct SolidSyslogSecurityPolicy* handle = SolidSyslogWolfSslAesGcmPolicy_Create(&config);

    CHECK_TEXT(handle != nullptr, "Create returned nullptr");
    CHECK_TEXT(handle != SolidSyslogNullSecurityPolicy_Get(), "Create returned the Null fallback");

    SolidSyslogWolfSslAesGcmPolicy_Destroy(handle);
}

TEST(SolidSyslogWolfSslAesGcmPolicy, TrailerSizeIsTwentyEight)
{
    struct SolidSyslogSecurityPolicy* handle = SolidSyslogWolfSslAesGcmPolicy_Create(&config);

    LONGS_EQUAL(AES_GCM_POLICY_TRAILER_SIZE, handle->TrailerSize);

    SolidSyslogWolfSslAesGcmPolicy_Destroy(handle);
}

TEST(SolidSyslogWolfSslAesGcmPolicy, FillingPoolThenOverflowReturnsNullFallback)
{
    FillPool();

    overflow = SolidSyslogWolfSslAesGcmPolicy_Create(&config);

    CHECK_IS_NULL_FALLBACK(overflow);
}

TEST(SolidSyslogWolfSslAesGcmPolicy, ExhaustedCreateReportsError)
{
    ErrorHandlerFake_Install(nullptr);
    FillPool();

    overflow = SolidSyslogWolfSslAesGcmPolicy_Create(&config);

    CHECK_REPORTED_ERROR(
        SOLIDSYSLOG_SEVERITY_CRITICAL,
        SOLIDSYSLOG_CAT_POOL_EXHAUSTED,
        SOLIDSYSLOG_AES_GCM_POLICY_ERROR_POOL_EXHAUSTED
    );
}

TEST(SolidSyslogWolfSslAesGcmPolicy, NullConfigReturnsNullFallback)
{
    CHECK_IS_NULL_FALLBACK(SolidSyslogWolfSslAesGcmPolicy_Create(nullptr));
}

TEST(SolidSyslogWolfSslAesGcmPolicy, NullGetKeyReturnsNullFallback)
{
    config.GetKey = nullptr;

    CHECK_IS_NULL_FALLBACK(SolidSyslogWolfSslAesGcmPolicy_Create(&config));
}

TEST(SolidSyslogWolfSslAesGcmPolicy, NullRngReturnsNullFallback)
{
    config.Rng = nullptr;

    CHECK_IS_NULL_FALLBACK(SolidSyslogWolfSslAesGcmPolicy_Create(&config));
}

TEST(SolidSyslogWolfSslAesGcmPolicy, BadConfigReportsError)
{
    ErrorHandlerFake_Install(nullptr);

    SolidSyslogWolfSslAesGcmPolicy_Create(nullptr);

    CHECK_REPORTED_ERROR(
        SOLIDSYSLOG_SEVERITY_CRITICAL,
        SOLIDSYSLOG_CAT_BAD_CONFIG,
        SOLIDSYSLOG_AES_GCM_POLICY_ERROR_BAD_CONFIG
    );
}

TEST(SolidSyslogWolfSslAesGcmPolicy, CreateAcquiresAndReleasesConfigLockOnFirstFreeSlot)
{
    ConfigLockFake_Install();

    pooled[0] = SolidSyslogWolfSslAesGcmPolicy_Create(&config);

    CALLED_FAKE(ConfigLockFake_Lock, ONCE);
    CALLED_FAKE(ConfigLockFake_Unlock, ONCE);
}

TEST(SolidSyslogWolfSslAesGcmPolicy, DestroyOfPooledHandleLocksOnce)
{
    pooled[0] = SolidSyslogWolfSslAesGcmPolicy_Create(&config);
    ConfigLockFake_Install();

    SolidSyslogWolfSslAesGcmPolicy_Destroy(pooled[0]);
    pooled[0] = nullptr;

    CALLED_FAKE(ConfigLockFake_Lock, ONCE);
    CALLED_FAKE(ConfigLockFake_Unlock, ONCE);
}

TEST(SolidSyslogWolfSslAesGcmPolicy, DestroyOfUnknownHandleReportsWarning)
{
    ErrorHandlerFake_Install(nullptr);
    struct SolidSyslogSecurityPolicy stranger = {};

    SolidSyslogWolfSslAesGcmPolicy_Destroy(&stranger);

    CHECK_REPORTED_ERROR(
        SOLIDSYSLOG_SEVERITY_WARNING,
        SOLIDSYSLOG_CAT_UNKNOWN_DESTROY,
        SOLIDSYSLOG_AES_GCM_POLICY_ERROR_UNKNOWN_DESTROY
    );
}

TEST(SolidSyslogWolfSslAesGcmPolicy, DestroyOfStaleHandleReportsWarning)
{
    pooled[0] = SolidSyslogWolfSslAesGcmPolicy_Create(&config);
    SolidSyslogWolfSslAesGcmPolicy_Destroy(pooled[0]);
    ErrorHandlerFake_Install(nullptr);

    SolidSyslogWolfSslAesGcmPolicy_Destroy(pooled[0]);
    pooled[0] = nullptr;

    CHECK_REPORTED_ERROR(
        SOLIDSYSLOG_SEVERITY_WARNING,
        SOLIDSYSLOG_CAT_UNKNOWN_DESTROY,
        SOLIDSYSLOG_AES_GCM_POLICY_ERROR_UNKNOWN_DESTROY
    );
}

TEST(SolidSyslogWolfSslAesGcmPolicySeal, SealRecordGeneratesAFreshNonceIntoTheTrailer)
{
    CHECK_TRUE(seal());

    /* The nonce is drawn from the injected RNG straight into the trailer. */
    LONGS_EQUAL(1, WolfSslFake_RngGenerateBlockCallCount());
    LONGS_EQUAL(AES_GCM_POLICY_NONCE_SIZE, WolfSslFake_LastRngGenerateBlockLen());
    POINTERS_EQUAL(trailer, WolfSslFake_LastRngGenerateBlockBuf());
    POINTERS_EQUAL(WolfSslFake_Rng(), WolfSslFake_LastRngGenerateBlockContext());
    /* The fake's RNG fills 0xA0, 0xA1, ... - assert it reached the trailer. */
    static const uint8_t expectedNonce[AES_GCM_POLICY_NONCE_SIZE] =
        {0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xAB};
    MEMCMP_EQUAL(expectedNonce, trailer, AES_GCM_POLICY_NONCE_SIZE);
}

TEST(SolidSyslogWolfSslAesGcmPolicySeal, SealRecordPassesTheBodyAsPlaintextToEncrypt)
{
    CHECK_TRUE(seal());

    /* Production hands wolfCrypt the body region (Content past HeaderLength), not
     * the header - that the body region is what gets encrypted is the wiring
     * under test. Whether the ciphertext genuinely differs is the integration
     * suite's concern. */
    LONGS_EQUAL(TEST_BODY_LEN, WolfSslFake_LastGcmPlaintextLen());
    MEMCMP_EQUAL(originalBody, WolfSslFake_LastGcmPlaintext(), TEST_BODY_LEN);
}

TEST(SolidSyslogWolfSslAesGcmPolicySeal, SealRecordAuthenticatesTheHeaderAsAssociatedData)
{
    CHECK_TRUE(seal());

    LONGS_EQUAL(TEST_HEADER_LEN, WolfSslFake_LastGcmAadLen());
    MEMCMP_EQUAL(content, WolfSslFake_LastGcmAad(), TEST_HEADER_LEN);
}

TEST(SolidSyslogWolfSslAesGcmPolicySeal, SealRecordUsesTheFetchedKeyAsAes256)
{
    uint8_t expectedKey[AES_GCM_POLICY_KEY_SIZE];
    memset(expectedKey, TEST_KEY_BYTE, sizeof expectedKey);

    CHECK_TRUE(seal());

    LONGS_EQUAL(1, WolfSslFake_GcmSealCount());
    MEMCMP_EQUAL(expectedKey, WolfSslFake_LastGcmKey(), AES_GCM_POLICY_KEY_SIZE);
    LONGS_EQUAL(AES_GCM_POLICY_KEY_SIZE, WolfSslFake_LastGcmKeyLen());
}

TEST(SolidSyslogWolfSslAesGcmPolicySeal, OpenReadsTheNonceFromTheTrailer)
{
    CHECK_TRUE(seal());

    LONGS_EQUAL(SOLIDSYSLOG_SECURITY_POLICY_OPEN_OPENED, open());

    /* Open must decrypt with the nonce the seal wrote into the trailer. */
    MEMCMP_EQUAL(trailer, WolfSslFake_LastGcmNonce(), AES_GCM_POLICY_NONCE_SIZE);
}

TEST(SolidSyslogWolfSslAesGcmPolicySeal, OpenOpensWhenDecryptionSucceeds)
{
    CHECK_TRUE(seal());

    LONGS_EQUAL(SOLIDSYSLOG_SECURITY_POLICY_OPEN_OPENED, open());

    LONGS_EQUAL(1, WolfSslFake_GcmOpenCount());
}

/* A tag mismatch (tamper or wrong key) surfaces as wc_AesGcmDecrypt returning
 * AES_GCM_AUTH_E. Production must reject the record but
 * stay silent - that is the expected outcome, not a library error. Real tamper /
 * wrong-key rejection lives in the integration suite; here we only prove the
 * adapter's verdict-propagation and silence. */
TEST(SolidSyslogWolfSslAesGcmPolicySeal, OpenRejectsWithoutReportingWhenAuthenticationFails)
{
    ErrorHandlerFake_Install(nullptr);
    WolfSslFake_SetGcmAuthFails(true);

    LONGS_EQUAL(SOLIDSYSLOG_SECURITY_POLICY_OPEN_REJECTED, open());
    CALLED_FAKE(ErrorHandlerFake_Handle, NEVER);
}

TEST(SolidSyslogWolfSslAesGcmPolicySeal, SealFailsClosedWhenKeyUnavailable)
{
    ErrorHandlerFake_Install(nullptr);
    keyAvailable = false;

    CHECK_FALSE(seal());
    CHECK_REPORTED_ERROR(
        SOLIDSYSLOG_SEVERITY_ERROR,
        SOLIDSYSLOG_CAT_SECURITY_POLICY_KEY_UNAVAILABLE,
        SOLIDSYSLOG_AES_GCM_POLICY_ERROR_KEY_UNAVAILABLE
    );
}

TEST(SolidSyslogWolfSslAesGcmPolicySeal, SealFailsClosedWhenKeyIsWrongLength)
{
    ErrorHandlerFake_Install(nullptr);
    keyLengthToReport = 16; /* AES-256 requires exactly 32 bytes */

    CHECK_FALSE(seal());
    CHECK_REPORTED_ERROR(
        SOLIDSYSLOG_SEVERITY_ERROR,
        SOLIDSYSLOG_CAT_SECURITY_POLICY_KEY_UNAVAILABLE,
        SOLIDSYSLOG_AES_GCM_POLICY_ERROR_KEY_UNAVAILABLE
    );
}

TEST(SolidSyslogWolfSslAesGcmPolicySeal, OpenCouldNotRunWhenKeyUnavailable)
{
    CHECK_TRUE(seal());
    keyAvailable = false;

    LONGS_EQUAL(SOLIDSYSLOG_SECURITY_POLICY_OPEN_COULD_NOT_RUN, open());
}

TEST(SolidSyslogWolfSslAesGcmPolicySeal, OpenReportsWhenKeyUnavailable)
{
    CHECK_TRUE(seal());
    ErrorHandlerFake_Install(nullptr);
    keyAvailable = false;

    open();
    CHECK_REPORTED_ERROR(
        SOLIDSYSLOG_SEVERITY_ERROR,
        SOLIDSYSLOG_CAT_SECURITY_POLICY_KEY_UNAVAILABLE,
        SOLIDSYSLOG_AES_GCM_POLICY_ERROR_KEY_UNAVAILABLE
    );
}

TEST(SolidSyslogWolfSslAesGcmPolicySeal, SealReportsNonceFailure)
{
    ErrorHandlerFake_Install(nullptr);
    WolfSslFake_SetRngGenerateBlockFails(true);

    CHECK_FALSE(seal());
    CHECK_REPORTED_ERROR(
        SOLIDSYSLOG_SEVERITY_ERROR,
        SOLIDSYSLOG_CAT_SECURITY_POLICY_SEAL_FAILED,
        SOLIDSYSLOG_AES_GCM_POLICY_ERROR_NONCE_FAILED
    );
}

/* Seal threads init, setkey then encrypt; a non-zero from either must fail
 * closed and report ENCRYPT_FAILED. One test per call pins each step. */
TEST(SolidSyslogWolfSslAesGcmPolicySeal, SealReportsErrorWhenInitialisingTheContextFails)
{
    CHECK_SEAL_REPORTS_ENCRYPT_FAILURE_AT(WOLFSSLFAKE_GCM_STEP_INIT);
}

TEST(SolidSyslogWolfSslAesGcmPolicySeal, SealReportsErrorWhenSettingKeyFails)
{
    CHECK_SEAL_REPORTS_ENCRYPT_FAILURE_AT(WOLFSSLFAKE_GCM_STEP_SETKEY);
}

TEST(SolidSyslogWolfSslAesGcmPolicySeal, SealReportsErrorWhenEncryptingFails)
{
    CHECK_SEAL_REPORTS_ENCRYPT_FAILURE_AT(WOLFSSLFAKE_GCM_STEP_ENCRYPT);
}

/* Open's init or setkey failure and a genuine (non-auth) decrypt error all report
 * DECRYPT_FAILED, and reach no verdict. The auth-mismatch verdict is separate -
 * that rejected-but-silent path is OpenRejectsWithoutReporting... above. */
TEST(SolidSyslogWolfSslAesGcmPolicySeal, OpenReportsErrorWhenInitialisingTheContextFails)
{
    CHECK_OPEN_REPORTS_DECRYPT_FAILURE_AT(WOLFSSLFAKE_GCM_STEP_INIT);
}

TEST(SolidSyslogWolfSslAesGcmPolicySeal, AnInitialisedContextIsFreedWhenSealFailsAtALaterStep)
{
    WolfSslFake_SetGcmStepFails(WOLFSSLFAKE_GCM_STEP_SETKEY);

    seal();

    LONGS_EQUAL(1, WolfSslFake_AesInitCallCount());
    LONGS_EQUAL(1, WolfSslFake_AesFreeCallCount());
}

TEST(SolidSyslogWolfSslAesGcmPolicySeal, AContextThatFailedToInitialiseIsNotFreed)
{
    WolfSslFake_SetGcmStepFails(WOLFSSLFAKE_GCM_STEP_INIT);

    seal();
    open();

    LONGS_EQUAL(0, WolfSslFake_AesFreeCallCount());
}

TEST(SolidSyslogWolfSslAesGcmPolicySeal, SealAndOpenEachFreeTheContextTheyInitialised)
{
    CHECK_TRUE(seal());
    open();

    LONGS_EQUAL(2, WolfSslFake_AesInitCallCount());
    LONGS_EQUAL(2, WolfSslFake_AesFreeCallCount());
}

TEST(SolidSyslogWolfSslAesGcmPolicySeal, OpenReportsErrorWhenSettingKeyFails)
{
    CHECK_OPEN_REPORTS_DECRYPT_FAILURE_AT(WOLFSSLFAKE_GCM_STEP_SETKEY);
}

TEST(SolidSyslogWolfSslAesGcmPolicySeal, OpenReportsErrorWhenDecryptingFails)
{
    CHECK_OPEN_REPORTS_DECRYPT_FAILURE_AT(WOLFSSLFAKE_GCM_STEP_DECRYPT);
}

TEST(SolidSyslogWolfSslAesGcmPolicySeal, SealWipesTheKeyBufferAfterUse)
{
    CHECK_TRUE(seal());

    LONGS_EQUAL(1, WolfSslFake_ForceZeroCallCount());
    POINTERS_EQUAL(lastGetKeyBuffer, WolfSslFake_LastForceZeroBuf());
    LONGS_EQUAL(lastGetKeyCapacity, WolfSslFake_LastForceZeroLen());
}

TEST(SolidSyslogWolfSslAesGcmPolicySeal, AKeyThatStaysUnavailableIsReportedOnce)
{
    CHECK_TRUE(seal());
    ErrorHandlerFake_Install(nullptr);
    keyAvailable = false;

    open();
    open();

    CALLED_FAKE(ErrorHandlerFake_Handle, ONCE);
}

TEST(SolidSyslogWolfSslAesGcmPolicySeal, AnOpenedRecordEndsTheRunSoTheNextFaultIsReportedAgain)
{
    CHECK_TRUE(seal());
    ErrorHandlerFake_Install(nullptr);
    keyAvailable = false;
    open();
    keyAvailable = true;
    open();

    keyAvailable = false;
    open();

    CALLED_FAKE(ErrorHandlerFake_Handle, TWICE);
}

TEST(SolidSyslogWolfSslAesGcmPolicySeal, ARejectedRecordEndsTheRunSoTheNextFaultIsReportedAgain)
{
    CHECK_TRUE(seal());
    ErrorHandlerFake_Install(nullptr);
    keyAvailable = false;
    open();
    keyAvailable = true;
    WolfSslFake_SetGcmAuthFails(true);
    open();
    WolfSslFake_SetGcmAuthFails(false);

    keyAvailable = false;
    open();

    CALLED_FAKE(ErrorHandlerFake_Handle, TWICE);
}

TEST(SolidSyslogWolfSslAesGcmPolicySeal, ASealedRecordEndsTheRunSoTheNextFaultIsReportedAgain)
{
    ErrorHandlerFake_Install(nullptr);
    keyAvailable = false;
    seal();
    keyAvailable = true;
    seal();

    keyAvailable = false;
    seal();

    CALLED_FAKE(ErrorHandlerFake_Handle, TWICE);
}

TEST(SolidSyslogWolfSslAesGcmPolicySeal, SealAndOpenShareOneRunOfFaults)
{
    ErrorHandlerFake_Install(nullptr);
    keyAvailable = false;

    seal();
    open();

    CALLED_FAKE(ErrorHandlerFake_Handle, ONCE);
}

TEST(SolidSyslogWolfSslAesGcmPolicySeal, AKeyFaultAndABackendFaultShareOneRun)
{
    CHECK_TRUE(seal());
    ErrorHandlerFake_Install(nullptr);
    keyAvailable = false;
    open();
    keyAvailable = true;
    WolfSslFake_SetGcmStepFails(WOLFSSLFAKE_GCM_STEP_SETKEY);

    open();

    CALLED_FAKE(ErrorHandlerFake_Handle, ONCE);
}

TEST(SolidSyslogWolfSslAesGcmPolicySeal, AKeyFaultAndASealBackendFaultShareOneRun)
{
    ErrorHandlerFake_Install(nullptr);
    keyAvailable = false;
    seal();
    keyAvailable = true;
    WolfSslFake_SetRngGenerateBlockFails(true);

    seal();

    CALLED_FAKE(ErrorHandlerFake_Handle, ONCE);
}

TEST(SolidSyslogWolfSslAesGcmPolicySeal, ANewPolicyReportsItsFirstFaultEvenAfterAnEarlierOneInTheSameSlotWasLatched)
{
    ErrorHandlerFake_Install(nullptr);
    keyAvailable = false;
    open();
    SolidSyslogWolfSslAesGcmPolicy_Destroy(policy);
    policy = SolidSyslogWolfSslAesGcmPolicy_Create(&config);

    open();

    CALLED_FAKE(ErrorHandlerFake_Handle, TWICE);
}

TEST(SolidSyslogWolfSslAesGcmPolicy, TheErrorSourceNamesItsDetailsThroughAesGcmPolicyErrors)
{
    CHECK(SolidSyslogAesGcmPolicyErrors_Name == SolidSyslogWolfSslAesGcmPolicyErrorSource.DetailName);
}
