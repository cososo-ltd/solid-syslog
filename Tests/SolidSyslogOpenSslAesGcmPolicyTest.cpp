#include <cstring>

#include "CppUTest/TestHarness.h"

extern "C"
{
#include "ConfigLockFake.h"
#include "ErrorHandlerFake.h"
#include "OpenSslFake.h"
#include "SolidSyslogNullSecurityPolicy.h"
#include "SolidSyslogOpenSslAesGcmPolicy.h"
#include "SolidSyslogOpenSslAesGcmPolicyErrors.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogSecurityPolicyDefinition.h"
#include "SolidSyslogTunables.h"
}

#include "TestUtils.h"

using namespace CososoTesting;

enum
{
    AES_256_KEY_SIZE = 32,
    GCM_NONCE_SIZE = 12,
    GCM_TAG_SIZE = 16,
    AES_GCM_TRAILER_SIZE = GCM_NONCE_SIZE + GCM_TAG_SIZE,
    TEST_HEADER_LEN = 4,
    TEST_BODY_LEN = 8,
    TEST_CONTENT_LEN = TEST_HEADER_LEN + TEST_BODY_LEN,
    TEST_KEY_BYTE = 0x2B
};

/* The buffer + capacity the policy handed to GetKey on the most recent fetch. */
static const uint8_t* lastGetKeyBuffer = nullptr;
static size_t lastGetKeyCapacity = 0;

/* Settable key accessor. `keyAvailable` false → GetKey fails; `keyByte` sets the
 * key contents (vary it to forge a wrong key); `keyLengthToReport` lets a test
 * report a non-32-byte key. */
static bool keyAvailable = true;
static uint8_t keyByte = TEST_KEY_BYTE;
static size_t keyLengthToReport = AES_256_KEY_SIZE;

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

// NOLINTBEGIN(cppcoreguidelines-macro-usage,cppcoreguidelines-avoid-do-while)
#define CHECK_REPORTED_ERROR(severity, code)                                            \
    do                                                                                  \
    {                                                                                   \
        CALLED_FAKE(ErrorHandlerFake_Handle, ONCE);                                     \
        LONGS_EQUAL((severity), ErrorHandlerFake_LastSeverity());                       \
        POINTERS_EQUAL(&OpenSslAesGcmPolicyErrorSource, ErrorHandlerFake_LastSource()); \
        UNSIGNED_LONGS_EQUAL((code), ErrorHandlerFake_LastCode());                      \
    } while (0)
// NOLINTEND(cppcoreguidelines-macro-usage,cppcoreguidelines-avoid-do-while)

#define CHECK_IS_NULL_FALLBACK(handle) POINTERS_EQUAL(SolidSyslogNullSecurityPolicy_Get(), (handle))

// clang-format off
TEST_BASE(OpenSslAesGcmPolicyTestBase)
{
    struct SolidSyslogOpenSslAesGcmPolicyConfig config = {};

    void armConfig()
    {
        OpenSslFake_Reset();
        lastGetKeyBuffer   = nullptr;
        lastGetKeyCapacity = 0;
        keyAvailable       = true;
        keyByte            = TEST_KEY_BYTE;
        keyLengthToReport  = AES_256_KEY_SIZE;
        config.GetKey      = TestGetKey;
        config.KeyContext  = nullptr;
    }
};

TEST_GROUP_BASE(SolidSyslogOpenSslAesGcmPolicy, OpenSslAesGcmPolicyTestBase)
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
                SolidSyslogOpenSslAesGcmPolicy_Destroy(handle);
            }
        }
        if (overflow != nullptr)
        {
            SolidSyslogOpenSslAesGcmPolicy_Destroy(overflow);
        }
        ConfigLockFake_Uninstall();
    }

    void FillPool()
    {
        for (auto*& slot : pooled)
        {
            slot = SolidSyslogOpenSslAesGcmPolicy_Create(&config);
        }
    }
};

TEST_GROUP_BASE(SolidSyslogOpenSslAesGcmPolicySeal, OpenSslAesGcmPolicyTestBase)
{
    struct SolidSyslogSecurityPolicy* policy = nullptr;
    uint8_t content[TEST_CONTENT_LEN] = {};
    uint8_t originalBody[TEST_BODY_LEN] = {};
    uint8_t trailer[AES_GCM_TRAILER_SIZE] = {};

    void setup() override
    {
        armConfig();
        policy = SolidSyslogOpenSslAesGcmPolicy_Create(&config);

        static const uint8_t header[TEST_HEADER_LEN] = {0xA5, 0x5A, 0x08, 0x00};
        static const uint8_t body[TEST_BODY_LEN] = {'p', 'l', 'a', 'i', 'n', 't', 'x', 't'};
        memcpy(content, header, TEST_HEADER_LEN);
        memcpy(&content[TEST_HEADER_LEN], body, TEST_BODY_LEN);
        memcpy(originalBody, body, TEST_BODY_LEN);
    }

    void teardown() override
    {
        SolidSyslogOpenSslAesGcmPolicy_Destroy(policy);
        ConfigLockFake_Uninstall();
    }

    bool seal()
    {
        return policy->SealRecord(policy, content, TEST_CONTENT_LEN, TEST_HEADER_LEN, trailer);
    }

    bool open()
    {
        return policy->OpenRecord(policy, content, TEST_CONTENT_LEN, TEST_HEADER_LEN, trailer);
    }

    const uint8_t* body() const
    {
        return &content[TEST_HEADER_LEN];
    }
};

// clang-format on

TEST(SolidSyslogOpenSslAesGcmPolicy, CreateReturnsHandleDistinctFromFallback)
{
    struct SolidSyslogSecurityPolicy* handle = SolidSyslogOpenSslAesGcmPolicy_Create(&config);

    CHECK_TEXT(handle != nullptr, "Create returned nullptr");
    CHECK_TEXT(handle != SolidSyslogNullSecurityPolicy_Get(), "Create returned the Null fallback");

    SolidSyslogOpenSslAesGcmPolicy_Destroy(handle);
}

TEST(SolidSyslogOpenSslAesGcmPolicy, TrailerSizeIsTwentyEight)
{
    struct SolidSyslogSecurityPolicy* handle = SolidSyslogOpenSslAesGcmPolicy_Create(&config);

    LONGS_EQUAL(AES_GCM_TRAILER_SIZE, handle->TrailerSize);

    SolidSyslogOpenSslAesGcmPolicy_Destroy(handle);
}

TEST(SolidSyslogOpenSslAesGcmPolicy, FillingPoolThenOverflowReturnsNullFallback)
{
    FillPool();

    overflow = SolidSyslogOpenSslAesGcmPolicy_Create(&config);

    CHECK_IS_NULL_FALLBACK(overflow);
}

TEST(SolidSyslogOpenSslAesGcmPolicy, ExhaustedCreateReportsError)
{
    ErrorHandlerFake_Install(nullptr);
    FillPool();

    overflow = SolidSyslogOpenSslAesGcmPolicy_Create(&config);

    CHECK_REPORTED_ERROR(SOLIDSYSLOG_SEVERITY_ERROR, OPENSSLAESGCMPOLICY_ERROR_POOL_EXHAUSTED);
}

TEST(SolidSyslogOpenSslAesGcmPolicy, NullConfigReturnsNullFallback)
{
    CHECK_IS_NULL_FALLBACK(SolidSyslogOpenSslAesGcmPolicy_Create(nullptr));
}

TEST(SolidSyslogOpenSslAesGcmPolicy, NullGetKeyReturnsNullFallback)
{
    config.GetKey = nullptr;

    CHECK_IS_NULL_FALLBACK(SolidSyslogOpenSslAesGcmPolicy_Create(&config));
}

TEST(SolidSyslogOpenSslAesGcmPolicy, BadConfigReportsError)
{
    ErrorHandlerFake_Install(nullptr);

    SolidSyslogOpenSslAesGcmPolicy_Create(nullptr);

    CHECK_REPORTED_ERROR(SOLIDSYSLOG_SEVERITY_ERROR, OPENSSLAESGCMPOLICY_ERROR_BAD_CONFIG);
}

TEST(SolidSyslogOpenSslAesGcmPolicy, CreateAcquiresAndReleasesConfigLockOnFirstFreeSlot)
{
    ConfigLockFake_Install();

    pooled[0] = SolidSyslogOpenSslAesGcmPolicy_Create(&config);

    CALLED_FAKE(ConfigLockFake_Lock, ONCE);
    CALLED_FAKE(ConfigLockFake_Unlock, ONCE);
}

TEST(SolidSyslogOpenSslAesGcmPolicy, DestroyOfPooledHandleLocksOnce)
{
    pooled[0] = SolidSyslogOpenSslAesGcmPolicy_Create(&config);
    ConfigLockFake_Install();

    SolidSyslogOpenSslAesGcmPolicy_Destroy(pooled[0]);
    pooled[0] = nullptr;

    CALLED_FAKE(ConfigLockFake_Lock, ONCE);
    CALLED_FAKE(ConfigLockFake_Unlock, ONCE);
}

TEST(SolidSyslogOpenSslAesGcmPolicy, DestroyOfUnknownHandleReportsWarning)
{
    ErrorHandlerFake_Install(nullptr);
    struct SolidSyslogSecurityPolicy stranger = {};

    SolidSyslogOpenSslAesGcmPolicy_Destroy(&stranger);

    CHECK_REPORTED_ERROR(SOLIDSYSLOG_SEVERITY_WARNING, OPENSSLAESGCMPOLICY_ERROR_UNKNOWN_DESTROY);
}

TEST(SolidSyslogOpenSslAesGcmPolicy, DestroyOfStaleHandleReportsWarning)
{
    pooled[0] = SolidSyslogOpenSslAesGcmPolicy_Create(&config);
    SolidSyslogOpenSslAesGcmPolicy_Destroy(pooled[0]);
    ErrorHandlerFake_Install(nullptr);

    SolidSyslogOpenSslAesGcmPolicy_Destroy(pooled[0]);
    pooled[0] = nullptr;

    CHECK_REPORTED_ERROR(SOLIDSYSLOG_SEVERITY_WARNING, OPENSSLAESGCMPOLICY_ERROR_UNKNOWN_DESTROY);
}

TEST(SolidSyslogOpenSslAesGcmPolicySeal, SealRecordGeneratesAFreshNonceIntoTheTrailer)
{
    CHECK_TRUE(seal());

    LONGS_EQUAL(1, OpenSslFake_RandBytesCallCount());
    LONGS_EQUAL(GCM_NONCE_SIZE, OpenSslFake_LastRandBytesLen());
    POINTERS_EQUAL(trailer, OpenSslFake_LastRandBytesBuf());
    /* The fake's RAND_bytes fills 0xA0, 0xA1, … — assert it reached the trailer. */
    for (int index = 0; index < GCM_NONCE_SIZE; index++)
    {
        BYTES_EQUAL(0xA0 + index, trailer[index]);
    }
}

TEST(SolidSyslogOpenSslAesGcmPolicySeal, SealRecordEncryptsTheBodyInPlace)
{
    CHECK_TRUE(seal());

    CHECK_TEXT(memcmp(body(), originalBody, TEST_BODY_LEN) != 0, "body was not encrypted");
    LONGS_EQUAL(TEST_BODY_LEN, OpenSslFake_LastGcmPlaintextLen());
    MEMCMP_EQUAL(originalBody, OpenSslFake_LastGcmPlaintext(), TEST_BODY_LEN);
}

TEST(SolidSyslogOpenSslAesGcmPolicySeal, SealRecordLeavesTheHeaderInClear)
{
    static const uint8_t header[TEST_HEADER_LEN] = {0xA5, 0x5A, 0x08, 0x00};

    CHECK_TRUE(seal());

    MEMCMP_EQUAL(header, content, TEST_HEADER_LEN);
}

TEST(SolidSyslogOpenSslAesGcmPolicySeal, SealRecordAuthenticatesTheHeaderAsAssociatedData)
{
    CHECK_TRUE(seal());

    LONGS_EQUAL(TEST_HEADER_LEN, OpenSslFake_LastGcmAadLen());
    MEMCMP_EQUAL(content, OpenSslFake_LastGcmAad(), TEST_HEADER_LEN);
}

TEST(SolidSyslogOpenSslAesGcmPolicySeal, SealRecordUsesTheFetchedKey)
{
    uint8_t expectedKey[AES_256_KEY_SIZE];
    memset(expectedKey, TEST_KEY_BYTE, sizeof expectedKey);

    CHECK_TRUE(seal());

    LONGS_EQUAL(1, OpenSslFake_GcmSealCount());
    MEMCMP_EQUAL(expectedKey, OpenSslFake_LastGcmKey(), AES_256_KEY_SIZE);
}

TEST(SolidSyslogOpenSslAesGcmPolicySeal, SealThenOpenRoundTripRestoresTheBody)
{
    CHECK_TRUE(seal());

    CHECK_TRUE(open());

    MEMCMP_EQUAL(originalBody, body(), TEST_BODY_LEN);
}

TEST(SolidSyslogOpenSslAesGcmPolicySeal, OpenRejectsAFlippedCiphertextByte)
{
    CHECK_TRUE(seal());
    content[TEST_HEADER_LEN] ^= 0xFFU;

    CHECK_FALSE(open());
}

TEST(SolidSyslogOpenSslAesGcmPolicySeal, OpenRejectsAFlippedTag)
{
    CHECK_TRUE(seal());
    trailer[GCM_NONCE_SIZE] ^= 0xFFU;

    CHECK_FALSE(open());
}

TEST(SolidSyslogOpenSslAesGcmPolicySeal, OpenRejectsAFlippedHeader)
{
    CHECK_TRUE(seal());
    content[0] ^= 0xFFU;

    CHECK_FALSE(open());
}

TEST(SolidSyslogOpenSslAesGcmPolicySeal, OpenRejectsAWrongKey)
{
    CHECK_TRUE(seal());
    keyByte = 0x99; /* GetKey now hands back a different key */

    CHECK_FALSE(open());
}

TEST(SolidSyslogOpenSslAesGcmPolicySeal, SealFailsClosedWhenKeyUnavailable)
{
    ErrorHandlerFake_Install(nullptr);
    keyAvailable = false;

    CHECK_FALSE(seal());
    CHECK_REPORTED_ERROR(SOLIDSYSLOG_SEVERITY_ERROR, OPENSSLAESGCMPOLICY_ERROR_KEY_UNAVAILABLE);
}

TEST(SolidSyslogOpenSslAesGcmPolicySeal, SealFailsClosedWhenKeyIsWrongLength)
{
    ErrorHandlerFake_Install(nullptr);
    keyLengthToReport = 16; /* AES-256 requires exactly 32 bytes */

    CHECK_FALSE(seal());
    CHECK_REPORTED_ERROR(SOLIDSYSLOG_SEVERITY_ERROR, OPENSSLAESGCMPOLICY_ERROR_KEY_UNAVAILABLE);
}

TEST(SolidSyslogOpenSslAesGcmPolicySeal, OpenFailsClosedWhenKeyUnavailable)
{
    CHECK_TRUE(seal());
    ErrorHandlerFake_Install(nullptr);
    keyAvailable = false;

    CHECK_FALSE(open());
    CHECK_REPORTED_ERROR(SOLIDSYSLOG_SEVERITY_ERROR, OPENSSLAESGCMPOLICY_ERROR_KEY_UNAVAILABLE);
}

TEST(SolidSyslogOpenSslAesGcmPolicySeal, SealReportsNonceFailure)
{
    ErrorHandlerFake_Install(nullptr);
    OpenSslFake_SetRandBytesFails(true);

    CHECK_FALSE(seal());
    CHECK_REPORTED_ERROR(SOLIDSYSLOG_SEVERITY_ERROR, OPENSSLAESGCMPOLICY_ERROR_NONCE_FAILED);
}

TEST(SolidSyslogOpenSslAesGcmPolicySeal, SealReportsEncryptFailure)
{
    ErrorHandlerFake_Install(nullptr);
    OpenSslFake_SetGcmEncryptFails(true);

    CHECK_FALSE(seal());
    CHECK_REPORTED_ERROR(SOLIDSYSLOG_SEVERITY_ERROR, OPENSSLAESGCMPOLICY_ERROR_ENCRYPT_FAILED);
}

TEST(SolidSyslogOpenSslAesGcmPolicySeal, OpenReportsDecryptFailure)
{
    CHECK_TRUE(seal());
    ErrorHandlerFake_Install(nullptr);
    OpenSslFake_SetGcmDecryptFails(true);

    CHECK_FALSE(open());
    CHECK_REPORTED_ERROR(SOLIDSYSLOG_SEVERITY_ERROR, OPENSSLAESGCMPOLICY_ERROR_DECRYPT_FAILED);
}

TEST(SolidSyslogOpenSslAesGcmPolicySeal, SealWipesTheKeyBufferAfterUse)
{
    CHECK_TRUE(seal());

    LONGS_EQUAL(1, OpenSslFake_CleanseCallCount());
    POINTERS_EQUAL(lastGetKeyBuffer, OpenSslFake_LastCleanseBuf());
    LONGS_EQUAL(lastGetKeyCapacity, OpenSslFake_LastCleanseLen());
}
