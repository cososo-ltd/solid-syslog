#include <cstring>

#include "CppUTest/TestHarness.h"

extern "C"
{
#include "ConfigLockFake.h"
#include "ErrorHandlerFake.h"
#include "WolfSslFake.h"
#include "SolidSyslogWolfSslHmacSha256Policy.h"
#include "SolidSyslogWolfSslHmacSha256PolicyErrors.h"
#include "SolidSyslogNullSecurityPolicy.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogSecurityPolicyDefinition.h"
#include "SolidSyslogTunables.h"
}

#ifndef WOLFSSL_USER_SETTINGS
#include <wolfssl/options.h>
#endif
#include <wolfssl/wolfcrypt/hmac.h>
#include <wolfssl/wolfcrypt/settings.h>

#include "SolidSyslogErrorCategory.h"
#include "SolidSyslogSecurityPolicyCategories.h"
#include "SolidSyslogHmacSha256PolicyErrors.h"
#include "SolidSyslogError.h"
#include "TestUtils.h"

using namespace CososoTesting;

enum
{
    HMAC_SHA256_TAG_SIZE = 32,
    TEST_KEY_SIZE = 32,
    TEST_KEY_BYTE = 0x2B
};

/* Record bytes a seal/verify test runs through the policy. Non-const because the
 * vtable's content parameter is non-const (an AEAD policy mutates it); HMAC does
 * not. */
static uint8_t TEST_RECORD[] = {0x10, 0x20, 0x30, 0x40};

/* The buffer + capacity the policy handed to GetKey on the most recent fetch -
 * lets a test assert the key buffer is wiped at exactly that pointer and size. */
static const uint8_t* lastGetKeyBuffer = nullptr;
static size_t lastGetKeyCapacity = 0U;

/* The key length GetKey reports on the next fetch. Defaults to a full-strength
 * TEST_KEY_SIZE key; a test lowers it to drive the sub-minimum-key path. */
static size_t testKeyLength = TEST_KEY_SIZE;

/* A key accessor: writes a fixed TEST_KEY_BYTE key. The policy fetches this on
 * every seal and verify; expectedTagFor() mirrors it so tests can predict the
 * fake's deterministic tag. `context` points at a bool the test can clear to
 * simulate the key being unavailable (GetKey returns false). */
static bool TestGetKey(void* context, uint8_t* keyOut, size_t capacity, size_t* keyLengthOut)
{
    lastGetKeyBuffer = keyOut;
    lastGetKeyCapacity = capacity;
    const bool* available = static_cast<const bool*>(context);
    if ((available != nullptr) && !*available)
    {
        return false;
    }
    size_t written = (capacity < testKeyLength) ? capacity : testKeyLength;
    memset(keyOut, TEST_KEY_BYTE, written);
    *keyLengthOut = testKeyLength;
    return true;
}

/* Asserts exactly one error of (severity, code) was reported from this policy's source. */
#define CHECK_REPORTED_ERROR(severity, expectedCategory, code) \
    CHECK_ERROR_REPORTED_ONCE((severity), &SolidSyslogWolfSslHmacSha256PolicyErrorSource, (expectedCategory), (code))

#define CHECK_IS_NULL_FALLBACK(handle) POINTERS_EQUAL(SolidSyslogNullSecurityPolicy_Get(), (handle))

// clang-format off
TEST_BASE(WolfSslHmacSha256PolicyTestBase)
{
    struct SolidSyslogWolfSslHmacSha256PolicyConfig config = {};
    bool keyAvailable = true;

    void armConfig()
    {
        WolfSslFake_Reset();
        lastGetKeyBuffer  = nullptr;
        lastGetKeyCapacity = 0;
        testKeyLength     = TEST_KEY_SIZE;
        config.GetKey     = TestGetKey;
        config.KeyContext = &keyAvailable;
    }
};

TEST_GROUP_BASE(SolidSyslogWolfSslHmacSha256Policy, WolfSslHmacSha256PolicyTestBase)
{
    struct SolidSyslogSecurityPolicy* pooled[SOLIDSYSLOG_HMAC_SHA256_POLICY_POOL_SIZE] = {};
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
                SolidSyslogWolfSslHmacSha256Policy_Destroy(handle);
            }
        }
        if (overflow != nullptr)
        {
            SolidSyslogWolfSslHmacSha256Policy_Destroy(overflow);
        }
        ConfigLockFake_Uninstall();
    }

    void FillPool()
    {
        for (auto*& slot : pooled)
        {
            slot = SolidSyslogWolfSslHmacSha256Policy_Create(&config);
        }
    }
};

TEST_GROUP_BASE(SolidSyslogWolfSslHmacSha256PolicySeal, WolfSslHmacSha256PolicyTestBase)
{
    struct SolidSyslogSecurityPolicy* policy = nullptr;

    void setup() override
    {
        armConfig();
        policy = SolidSyslogWolfSslHmacSha256Policy_Create(&config);
    }

    void teardown() override
    {
        SolidSyslogWolfSslHmacSha256Policy_Destroy(policy);
        ConfigLockFake_Uninstall();
    }

    bool seal(uint8_t* content, size_t length, uint8_t* tag) const
    {
        struct SolidSyslogSecurityRecord rec = {content, (uint16_t) length, 0, tag};
        return policy->SealRecord(policy, &rec);
    }

    enum SolidSyslogSecurityPolicyOpenResult verify(uint8_t* content, size_t length, uint8_t* tag) const
    {
        struct SolidSyslogSecurityRecord rec = {content, (uint16_t) length, 0, tag};
        return policy->OpenRecord(policy, &rec);
    }

    static void expectedTagFor(const uint8_t* record, size_t length, uint8_t* tagOut)
    {
        uint8_t key[TEST_KEY_SIZE];
        memset(key, TEST_KEY_BYTE, sizeof key);
        WolfSslFake_ComputeExpectedTag(key, TEST_KEY_SIZE, record, length, tagOut);
    }
};

// clang-format on

TEST(SolidSyslogWolfSslHmacSha256Policy, CreateReturnsHandleDistinctFromFallback)
{
    struct SolidSyslogSecurityPolicy* handle = SolidSyslogWolfSslHmacSha256Policy_Create(&config);

    CHECK_TEXT(handle != nullptr, "Create returned nullptr");
    CHECK_TEXT(handle != SolidSyslogNullSecurityPolicy_Get(), "Create returned the Null fallback");

    SolidSyslogWolfSslHmacSha256Policy_Destroy(handle);
}

TEST(SolidSyslogWolfSslHmacSha256Policy, TrailerSizeIsThirtyTwo)
{
    struct SolidSyslogSecurityPolicy* handle = SolidSyslogWolfSslHmacSha256Policy_Create(&config);

    LONGS_EQUAL(HMAC_SHA256_TAG_SIZE, handle->TrailerSize);

    SolidSyslogWolfSslHmacSha256Policy_Destroy(handle);
}

TEST(SolidSyslogWolfSslHmacSha256Policy, FillingPoolThenOverflowReturnsNullFallback)
{
    FillPool();

    overflow = SolidSyslogWolfSslHmacSha256Policy_Create(&config);

    CHECK_IS_NULL_FALLBACK(overflow);
}

TEST(SolidSyslogWolfSslHmacSha256Policy, ExhaustedCreateReportsError)
{
    ErrorHandlerFake_Install(nullptr);
    FillPool();

    overflow = SolidSyslogWolfSslHmacSha256Policy_Create(&config);

    CHECK_REPORTED_ERROR(
        SOLIDSYSLOG_SEVERITY_CRITICAL,
        SOLIDSYSLOG_CAT_POOL_EXHAUSTED,
        SOLIDSYSLOG_HMAC_SHA256_POLICY_ERROR_POOL_EXHAUSTED
    );
}

TEST(SolidSyslogWolfSslHmacSha256Policy, NullConfigReturnsNullFallback)
{
    CHECK_IS_NULL_FALLBACK(SolidSyslogWolfSslHmacSha256Policy_Create(nullptr));
}

TEST(SolidSyslogWolfSslHmacSha256Policy, NullGetKeyReturnsNullFallback)
{
    config.GetKey = nullptr;

    CHECK_IS_NULL_FALLBACK(SolidSyslogWolfSslHmacSha256Policy_Create(&config));
}

TEST(SolidSyslogWolfSslHmacSha256Policy, BadConfigReportsError)
{
    ErrorHandlerFake_Install(nullptr);

    SolidSyslogWolfSslHmacSha256Policy_Create(nullptr);

    CHECK_REPORTED_ERROR(
        SOLIDSYSLOG_SEVERITY_CRITICAL,
        SOLIDSYSLOG_CAT_BAD_CONFIG,
        SOLIDSYSLOG_HMAC_SHA256_POLICY_ERROR_BAD_CONFIG
    );
}

TEST(SolidSyslogWolfSslHmacSha256Policy, CreateAcquiresAndReleasesConfigLockOnFirstFreeSlot)
{
    ConfigLockFake_Install();

    pooled[0] = SolidSyslogWolfSslHmacSha256Policy_Create(&config);

    CALLED_FAKE(ConfigLockFake_Lock, ONCE);
    CALLED_FAKE(ConfigLockFake_Unlock, ONCE);
}

TEST(SolidSyslogWolfSslHmacSha256Policy, DestroyOfPooledHandleLocksOnce)
{
    pooled[0] = SolidSyslogWolfSslHmacSha256Policy_Create(&config);
    ConfigLockFake_Install();

    SolidSyslogWolfSslHmacSha256Policy_Destroy(pooled[0]);
    pooled[0] = nullptr;

    CALLED_FAKE(ConfigLockFake_Lock, ONCE);
    CALLED_FAKE(ConfigLockFake_Unlock, ONCE);
}

TEST(SolidSyslogWolfSslHmacSha256Policy, DestroyOfUnknownHandleReportsWarning)
{
    ErrorHandlerFake_Install(nullptr);
    struct SolidSyslogSecurityPolicy stranger = {};

    SolidSyslogWolfSslHmacSha256Policy_Destroy(&stranger);

    CHECK_REPORTED_ERROR(
        SOLIDSYSLOG_SEVERITY_WARNING,
        SOLIDSYSLOG_CAT_UNKNOWN_DESTROY,
        SOLIDSYSLOG_HMAC_SHA256_POLICY_ERROR_UNKNOWN_DESTROY
    );
}

TEST(SolidSyslogWolfSslHmacSha256Policy, DestroyOfStaleHandleReportsWarning)
{
    pooled[0] = SolidSyslogWolfSslHmacSha256Policy_Create(&config);
    SolidSyslogWolfSslHmacSha256Policy_Destroy(pooled[0]);
    ErrorHandlerFake_Install(nullptr);

    SolidSyslogWolfSslHmacSha256Policy_Destroy(pooled[0]);
    pooled[0] = nullptr;

    CHECK_REPORTED_ERROR(
        SOLIDSYSLOG_SEVERITY_WARNING,
        SOLIDSYSLOG_CAT_UNKNOWN_DESTROY,
        SOLIDSYSLOG_HMAC_SHA256_POLICY_ERROR_UNKNOWN_DESTROY
    );
}

TEST(SolidSyslogWolfSslHmacSha256PolicySeal, SealRecordHmacsTheRecordWithTheFetchedKey)
{
    uint8_t expectedKey[TEST_KEY_SIZE];
    memset(expectedKey, TEST_KEY_BYTE, sizeof expectedKey);
    uint8_t tag[HMAC_SHA256_TAG_SIZE] = {};

    seal(TEST_RECORD, sizeof TEST_RECORD, tag);

    LONGS_EQUAL(1, WolfSslFake_HmacSetKeyCallCount());
    LONGS_EQUAL(WC_SHA256, WolfSslFake_LastHmacType());
    LONGS_EQUAL(TEST_KEY_SIZE, WolfSslFake_LastHmacKeyLen());
    MEMCMP_EQUAL(expectedKey, WolfSslFake_LastHmacKey(), TEST_KEY_SIZE);
    LONGS_EQUAL(sizeof TEST_RECORD, WolfSslFake_LastHmacInputLen());
    MEMCMP_EQUAL(TEST_RECORD, WolfSslFake_LastHmacInput(), sizeof TEST_RECORD);
}

TEST(SolidSyslogWolfSslHmacSha256PolicySeal, SealRecordWritesTheTagIntoTheTrailer)
{
    uint8_t expected[HMAC_SHA256_TAG_SIZE];
    expectedTagFor(TEST_RECORD, sizeof TEST_RECORD, expected);
    uint8_t tag[HMAC_SHA256_TAG_SIZE] = {};

    seal(TEST_RECORD, sizeof TEST_RECORD, tag);

    MEMCMP_EQUAL(expected, tag, HMAC_SHA256_TAG_SIZE);
}

TEST(SolidSyslogWolfSslHmacSha256PolicySeal, SealRecordReportsHmacFailure)
{
    ErrorHandlerFake_Install(nullptr);
    WolfSslFake_SetHmacStepFails(WOLFSSLFAKE_HMAC_STEP_FINAL);
    uint8_t tag[HMAC_SHA256_TAG_SIZE] = {};

    bool sealed = seal(TEST_RECORD, sizeof TEST_RECORD, tag);

    CHECK_FALSE(sealed);
    CHECK_REPORTED_ERROR(
        SOLIDSYSLOG_SEVERITY_ERROR,
        SOLIDSYSLOG_CAT_SECURITY_POLICY_SEAL_FAILED,
        SOLIDSYSLOG_HMAC_SHA256_POLICY_ERROR_HMAC_FAILED
    );
}

TEST(SolidSyslogWolfSslHmacSha256PolicySeal, SealRecordReportsKeyUnavailable)
{
    ErrorHandlerFake_Install(nullptr);
    keyAvailable = false;
    uint8_t tag[HMAC_SHA256_TAG_SIZE] = {};

    bool sealed = seal(TEST_RECORD, sizeof TEST_RECORD, tag);

    CHECK_FALSE(sealed);
    CHECK_REPORTED_ERROR(
        SOLIDSYSLOG_SEVERITY_ERROR,
        SOLIDSYSLOG_CAT_SECURITY_POLICY_KEY_UNAVAILABLE,
        SOLIDSYSLOG_HMAC_SHA256_POLICY_ERROR_KEY_UNAVAILABLE
    );
}

TEST(SolidSyslogWolfSslHmacSha256PolicySeal, SealRecordFailsClosedWhenKeyBelowMinimum)
{
    testKeyLength = 16;
    uint8_t tag[HMAC_SHA256_TAG_SIZE] = {};

    bool sealed = seal(TEST_RECORD, sizeof TEST_RECORD, tag);

    CHECK_FALSE(sealed);
}

TEST(SolidSyslogWolfSslHmacSha256PolicySeal, SealRecordReportsKeyTooShort)
{
    ErrorHandlerFake_Install(nullptr);
    testKeyLength = 16;
    uint8_t tag[HMAC_SHA256_TAG_SIZE] = {};

    bool sealed = seal(TEST_RECORD, sizeof TEST_RECORD, tag);

    CHECK_FALSE(sealed);
    CHECK_REPORTED_ERROR(
        SOLIDSYSLOG_SEVERITY_ERROR,
        SOLIDSYSLOG_CAT_SECURITY_POLICY_KEY_UNAVAILABLE,
        SOLIDSYSLOG_HMAC_SHA256_POLICY_ERROR_KEY_TOO_SHORT
    );
}

// A key longer than the buffer GetKey was handed is a contract violation; reject
// it too rather than passing an out-of-range length to wolfssl_md_hmac.
TEST(SolidSyslogWolfSslHmacSha256PolicySeal, SealRecordFailsClosedWhenKeyExceedsCapacity)
{
    testKeyLength = SOLIDSYSLOG_MAX_HMAC_KEY_SIZE + 1U;
    uint8_t tag[HMAC_SHA256_TAG_SIZE] = {};

    CHECK_FALSE(seal(TEST_RECORD, sizeof TEST_RECORD, tag));
}

TEST(SolidSyslogWolfSslHmacSha256PolicySeal, OpenRecordCouldNotRunWhenKeyBelowMinimum)
{
    uint8_t tag[HMAC_SHA256_TAG_SIZE] = {};
    seal(TEST_RECORD, sizeof TEST_RECORD, tag);
    testKeyLength = 16;

    LONGS_EQUAL(SOLIDSYSLOG_SECURITY_POLICY_OPEN_COULD_NOT_RUN, verify(TEST_RECORD, sizeof TEST_RECORD, tag));
}

TEST(SolidSyslogWolfSslHmacSha256PolicySeal, SealRecordWipesTheKeyBufferAfterUse)
{
    uint8_t tag[HMAC_SHA256_TAG_SIZE] = {};

    seal(TEST_RECORD, sizeof TEST_RECORD, tag);

    LONGS_EQUAL(1, WolfSslFake_ForceZeroCallCount());
    POINTERS_EQUAL(lastGetKeyBuffer, WolfSslFake_LastForceZeroBuf());
    LONGS_EQUAL(lastGetKeyCapacity, WolfSslFake_LastForceZeroLen());
}

TEST(SolidSyslogWolfSslHmacSha256PolicySeal, OpenRecordAcceptsATagItProduced)
{
    uint8_t tag[HMAC_SHA256_TAG_SIZE] = {};
    seal(TEST_RECORD, sizeof TEST_RECORD, tag);

    LONGS_EQUAL(SOLIDSYSLOG_SECURITY_POLICY_OPEN_OPENED, verify(TEST_RECORD, sizeof TEST_RECORD, tag));
}

TEST(SolidSyslogWolfSslHmacSha256PolicySeal, OpenRecordRejectsAModifiedTag)
{
    uint8_t tag[HMAC_SHA256_TAG_SIZE] = {};
    seal(TEST_RECORD, sizeof TEST_RECORD, tag);
    tag[0] ^= 0xFFU;

    LONGS_EQUAL(SOLIDSYSLOG_SECURITY_POLICY_OPEN_REJECTED, verify(TEST_RECORD, sizeof TEST_RECORD, tag));
}

TEST(SolidSyslogWolfSslHmacSha256PolicySeal, OpenRecordCouldNotRunWhenKeyUnavailable)
{
    uint8_t tag[HMAC_SHA256_TAG_SIZE] = {};
    seal(TEST_RECORD, sizeof TEST_RECORD, tag);
    keyAvailable = false;

    LONGS_EQUAL(SOLIDSYSLOG_SECURITY_POLICY_OPEN_COULD_NOT_RUN, verify(TEST_RECORD, sizeof TEST_RECORD, tag));
}

TEST(SolidSyslogWolfSslHmacSha256PolicySeal, OpenRecordCouldNotRunWhenHmacFails)
{
    uint8_t tag[HMAC_SHA256_TAG_SIZE] = {};
    seal(TEST_RECORD, sizeof TEST_RECORD, tag);
    WolfSslFake_SetHmacStepFails(WOLFSSLFAKE_HMAC_STEP_FINAL);

    LONGS_EQUAL(SOLIDSYSLOG_SECURITY_POLICY_OPEN_COULD_NOT_RUN, verify(TEST_RECORD, sizeof TEST_RECORD, tag));
}

// An HMAC computation failure during verify is an OPEN failure, not a SEAL
// failure - the same ComputeTag helper serves both paths, so the category must
// be chosen by the caller.
TEST(SolidSyslogWolfSslHmacSha256PolicySeal, OpenRecordReportsHmacFailureAsOpenFailed)
{
    uint8_t tag[HMAC_SHA256_TAG_SIZE] = {};
    seal(TEST_RECORD, sizeof TEST_RECORD, tag);
    ErrorHandlerFake_Install(nullptr);
    WolfSslFake_SetHmacStepFails(WOLFSSLFAKE_HMAC_STEP_FINAL);

    verify(TEST_RECORD, sizeof TEST_RECORD, tag);

    CHECK_REPORTED_ERROR(
        SOLIDSYSLOG_SEVERITY_ERROR,
        SOLIDSYSLOG_CAT_SECURITY_POLICY_OPEN_FAILED,
        SOLIDSYSLOG_HMAC_SHA256_POLICY_ERROR_HMAC_FAILED
    );
}

TEST(SolidSyslogWolfSslHmacSha256PolicySeal, AKeyThatStaysUnavailableIsReportedOnce)
{
    uint8_t tag[HMAC_SHA256_TAG_SIZE] = {};
    seal(TEST_RECORD, sizeof TEST_RECORD, tag);
    ErrorHandlerFake_Install(nullptr);
    keyAvailable = false;

    verify(TEST_RECORD, sizeof TEST_RECORD, tag);
    verify(TEST_RECORD, sizeof TEST_RECORD, tag);

    CALLED_FAKE(ErrorHandlerFake_Handle, ONCE);
}

TEST(SolidSyslogWolfSslHmacSha256PolicySeal, AnOpenedRecordEndsTheRunSoTheNextFaultIsReportedAgain)
{
    uint8_t tag[HMAC_SHA256_TAG_SIZE] = {};
    seal(TEST_RECORD, sizeof TEST_RECORD, tag);
    ErrorHandlerFake_Install(nullptr);
    keyAvailable = false;
    verify(TEST_RECORD, sizeof TEST_RECORD, tag);
    keyAvailable = true;
    verify(TEST_RECORD, sizeof TEST_RECORD, tag);

    keyAvailable = false;
    verify(TEST_RECORD, sizeof TEST_RECORD, tag);

    CALLED_FAKE(ErrorHandlerFake_Handle, TWICE);
}

TEST(SolidSyslogWolfSslHmacSha256PolicySeal, ARejectedRecordEndsTheRunSoTheNextFaultIsReportedAgain)
{
    uint8_t tag[HMAC_SHA256_TAG_SIZE] = {};
    seal(TEST_RECORD, sizeof TEST_RECORD, tag);
    tag[0] ^= 0xFFU;
    ErrorHandlerFake_Install(nullptr);
    keyAvailable = false;
    verify(TEST_RECORD, sizeof TEST_RECORD, tag);
    keyAvailable = true;
    verify(TEST_RECORD, sizeof TEST_RECORD, tag);

    keyAvailable = false;
    verify(TEST_RECORD, sizeof TEST_RECORD, tag);

    CALLED_FAKE(ErrorHandlerFake_Handle, TWICE);
}

TEST(SolidSyslogWolfSslHmacSha256PolicySeal, ASealedRecordEndsTheRunSoTheNextFaultIsReportedAgain)
{
    uint8_t tag[HMAC_SHA256_TAG_SIZE] = {};
    ErrorHandlerFake_Install(nullptr);
    keyAvailable = false;
    seal(TEST_RECORD, sizeof TEST_RECORD, tag);
    keyAvailable = true;
    seal(TEST_RECORD, sizeof TEST_RECORD, tag);

    keyAvailable = false;
    seal(TEST_RECORD, sizeof TEST_RECORD, tag);

    CALLED_FAKE(ErrorHandlerFake_Handle, TWICE);
}

TEST(SolidSyslogWolfSslHmacSha256PolicySeal, SealAndOpenShareOneRunOfFaults)
{
    uint8_t tag[HMAC_SHA256_TAG_SIZE] = {};
    ErrorHandlerFake_Install(nullptr);
    keyAvailable = false;

    seal(TEST_RECORD, sizeof TEST_RECORD, tag);
    verify(TEST_RECORD, sizeof TEST_RECORD, tag);

    CALLED_FAKE(ErrorHandlerFake_Handle, ONCE);
}

TEST(SolidSyslogWolfSslHmacSha256PolicySeal, AKeyFaultAndABackendFaultShareOneRun)
{
    uint8_t tag[HMAC_SHA256_TAG_SIZE] = {};
    ErrorHandlerFake_Install(nullptr);
    keyAvailable = false;
    verify(TEST_RECORD, sizeof TEST_RECORD, tag);
    keyAvailable = true;
    WolfSslFake_SetHmacStepFails(WOLFSSLFAKE_HMAC_STEP_FINAL);

    verify(TEST_RECORD, sizeof TEST_RECORD, tag);

    CALLED_FAKE(ErrorHandlerFake_Handle, ONCE);
}

TEST(SolidSyslogWolfSslHmacSha256PolicySeal, ANewPolicyReportsItsFirstFaultEvenAfterAnEarlierOneInTheSameSlotWasLatched)
{
    uint8_t tag[HMAC_SHA256_TAG_SIZE] = {};
    ErrorHandlerFake_Install(nullptr);
    keyAvailable = false;
    verify(TEST_RECORD, sizeof TEST_RECORD, tag);
    SolidSyslogWolfSslHmacSha256Policy_Destroy(policy);
    policy = SolidSyslogWolfSslHmacSha256Policy_Create(&config);

    verify(TEST_RECORD, sizeof TEST_RECORD, tag);

    CALLED_FAKE(ErrorHandlerFake_Handle, TWICE);
}


/* Each fallible wolfCrypt call in the HMAC sequence fails the seal closed and
 * is reported as one HMAC fault. */
#define CHECK_SEAL_REPORTS_HMAC_FAILURE_AT(step)              \
    {                                                         \
        ErrorHandlerFake_Install(nullptr);                    \
        WolfSslFake_SetHmacStepFails(step);                   \
        uint8_t tag[HMAC_SHA256_TAG_SIZE] = {};               \
        CHECK_FALSE(seal(TEST_RECORD, sizeof TEST_RECORD, tag)); \
        CHECK_REPORTED_ERROR(                                 \
            SOLIDSYSLOG_SEVERITY_ERROR,                       \
            SOLIDSYSLOG_CAT_SECURITY_POLICY_SEAL_FAILED,      \
            SOLIDSYSLOG_HMAC_SHA256_POLICY_ERROR_HMAC_FAILED  \
        );                                                    \
    }

TEST(SolidSyslogWolfSslHmacSha256PolicySeal, SealRecordFailsClosedWhenHmacInitFails)
{
    CHECK_SEAL_REPORTS_HMAC_FAILURE_AT(WOLFSSLFAKE_HMAC_STEP_INIT);
}

TEST(SolidSyslogWolfSslHmacSha256PolicySeal, SealRecordFailsClosedWhenHmacSetKeyFails)
{
    CHECK_SEAL_REPORTS_HMAC_FAILURE_AT(WOLFSSLFAKE_HMAC_STEP_SETKEY);
}

TEST(SolidSyslogWolfSslHmacSha256PolicySeal, SealRecordFailsClosedWhenHmacUpdateFails)
{
    CHECK_SEAL_REPORTS_HMAC_FAILURE_AT(WOLFSSLFAKE_HMAC_STEP_UPDATE);
}

TEST(SolidSyslogWolfSslHmacSha256PolicySeal, AnInitialisedHmacContextIsFreedWhenAStepFails)
{
    WolfSslFake_SetHmacStepFails(WOLFSSLFAKE_HMAC_STEP_SETKEY);
    uint8_t tag[HMAC_SHA256_TAG_SIZE] = {};

    seal(TEST_RECORD, sizeof TEST_RECORD, tag);

    LONGS_EQUAL(1, WolfSslFake_HmacInitCallCount());
    LONGS_EQUAL(1, WolfSslFake_HmacFreeCallCount());
}

TEST(SolidSyslogWolfSslHmacSha256PolicySeal, AContextThatFailedToInitialiseIsNotFreed)
{
    WolfSslFake_SetHmacStepFails(WOLFSSLFAKE_HMAC_STEP_INIT);
    uint8_t tag[HMAC_SHA256_TAG_SIZE] = {};

    seal(TEST_RECORD, sizeof TEST_RECORD, tag);

    LONGS_EQUAL(0, WolfSslFake_HmacFreeCallCount());
}

TEST(SolidSyslogWolfSslHmacSha256PolicySeal, EverySealFreesTheContextItInitialised)
{
    uint8_t tag[HMAC_SHA256_TAG_SIZE] = {};

    seal(TEST_RECORD, sizeof TEST_RECORD, tag);

    LONGS_EQUAL(1, WolfSslFake_HmacInitCallCount());
    LONGS_EQUAL(1, WolfSslFake_HmacFreeCallCount());
}

TEST(SolidSyslogWolfSslHmacSha256Policy, TheErrorSourceNamesItsDetailsThroughHmacSha256PolicyErrors)
{
    CHECK(SolidSyslogHmacSha256PolicyErrors_Name == SolidSyslogWolfSslHmacSha256PolicyErrorSource.DetailName);
}
