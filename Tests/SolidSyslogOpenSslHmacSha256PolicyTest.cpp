#include <cstring>

#include "CppUTest/TestHarness.h"

extern "C"
{
#include "ConfigLockFake.h"
#include "ErrorHandlerFake.h"
#include "OpenSslFake.h"
#include "SolidSyslogOpenSslHmacSha256Policy.h"
#include "SolidSyslogOpenSslHmacSha256PolicyErrors.h"
#include "SolidSyslogNullSecurityPolicy.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogSecurityPolicyDefinition.h"
#include "SolidSyslogTunables.h"
}

#include <openssl/evp.h>

#include "TestUtils.h"

using namespace CososoTesting;

enum
{
    HMAC_SHA256_TAG_SIZE = 32,
    TEST_KEY_SIZE = 32,
    TEST_KEY_BYTE = 0x2B
};

/* Record bytes a seal/verify test runs through the policy. */
static const uint8_t TEST_RECORD[] = {0x10, 0x20, 0x30, 0x40};

/* The buffer + capacity the policy handed to GetKey on the most recent fetch —
 * lets a test assert the key buffer is wiped at exactly that pointer and size. */
static const uint8_t* lastGetKeyBuffer = nullptr;
static size_t lastGetKeyCapacity = 0;

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
    size_t written = (capacity < TEST_KEY_SIZE) ? capacity : (size_t) TEST_KEY_SIZE;
    memset(keyOut, TEST_KEY_BYTE, written);
    *keyLengthOut = written;
    return true;
}

/* Asserts exactly one error of (severity, code) was reported from this policy's source. */
// NOLINTBEGIN(cppcoreguidelines-macro-usage,cppcoreguidelines-avoid-do-while)
#define CHECK_REPORTED_ERROR(severity, code)                                                \
    do                                                                                      \
    {                                                                                       \
        CALLED_FAKE(ErrorHandlerFake_Handle, ONCE);                                         \
        LONGS_EQUAL((severity), ErrorHandlerFake_LastSeverity());                           \
        POINTERS_EQUAL(&OpenSslHmacSha256PolicyErrorSource, ErrorHandlerFake_LastSource()); \
        UNSIGNED_LONGS_EQUAL((code), ErrorHandlerFake_LastCode());                          \
    } while (0)
// NOLINTEND(cppcoreguidelines-macro-usage,cppcoreguidelines-avoid-do-while)

#define CHECK_IS_NULL_FALLBACK(handle) POINTERS_EQUAL(SolidSyslogNullSecurityPolicy_Get(), (handle))

// clang-format off
TEST_BASE(OpenSslHmacSha256PolicyTestBase)
{
    struct SolidSyslogOpenSslHmacSha256PolicyConfig config = {};
    bool keyAvailable = true;

    void armConfig()
    {
        OpenSslFake_Reset();
        lastGetKeyBuffer  = nullptr;
        lastGetKeyCapacity = 0;
        config.GetKey     = TestGetKey;
        config.KeyContext = &keyAvailable;
    }
};

TEST_GROUP_BASE(SolidSyslogOpenSslHmacSha256Policy, OpenSslHmacSha256PolicyTestBase)
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
                SolidSyslogOpenSslHmacSha256Policy_Destroy(handle);
            }
        }
        if (overflow != nullptr)
        {
            SolidSyslogOpenSslHmacSha256Policy_Destroy(overflow);
        }
        ConfigLockFake_Uninstall();
    }

    void FillPool()
    {
        for (auto*& slot : pooled)
        {
            slot = SolidSyslogOpenSslHmacSha256Policy_Create(&config);
        }
    }
};

TEST_GROUP_BASE(SolidSyslogOpenSslHmacSha256PolicySeal, OpenSslHmacSha256PolicyTestBase)
{
    struct SolidSyslogSecurityPolicy* policy = nullptr;

    void setup() override
    {
        armConfig();
        policy = SolidSyslogOpenSslHmacSha256Policy_Create(&config);
    }

    void teardown() override
    {
        SolidSyslogOpenSslHmacSha256Policy_Destroy(policy);
        ConfigLockFake_Uninstall();
    }

    bool seal(const uint8_t* record, size_t length, uint8_t* tag) const
    {
        return policy->ComputeIntegrity(policy, record, (uint16_t) length, tag);
    }

    bool verify(const uint8_t* record, size_t length, const uint8_t* tag) const
    {
        return policy->VerifyIntegrity(policy, record, (uint16_t) length, tag);
    }

    static void expectedTagFor(const uint8_t* record, size_t length, uint8_t* tagOut)
    {
        uint8_t key[TEST_KEY_SIZE];
        memset(key, TEST_KEY_BYTE, sizeof key);
        OpenSslFake_ComputeExpectedTag(key, TEST_KEY_SIZE, record, length, tagOut);
    }
};

// clang-format on

TEST(SolidSyslogOpenSslHmacSha256Policy, CreateReturnsHandleDistinctFromFallback)
{
    struct SolidSyslogSecurityPolicy* handle = SolidSyslogOpenSslHmacSha256Policy_Create(&config);

    CHECK_TEXT(handle != nullptr, "Create returned nullptr");
    CHECK_TEXT(handle != SolidSyslogNullSecurityPolicy_Get(), "Create returned the Null fallback");

    SolidSyslogOpenSslHmacSha256Policy_Destroy(handle);
}

TEST(SolidSyslogOpenSslHmacSha256Policy, IntegritySizeIsThirtyTwo)
{
    struct SolidSyslogSecurityPolicy* handle = SolidSyslogOpenSslHmacSha256Policy_Create(&config);

    LONGS_EQUAL(HMAC_SHA256_TAG_SIZE, handle->IntegritySize);

    SolidSyslogOpenSslHmacSha256Policy_Destroy(handle);
}

TEST(SolidSyslogOpenSslHmacSha256Policy, FillingPoolThenOverflowReturnsNullFallback)
{
    FillPool();

    overflow = SolidSyslogOpenSslHmacSha256Policy_Create(&config);

    CHECK_IS_NULL_FALLBACK(overflow);
}

TEST(SolidSyslogOpenSslHmacSha256Policy, ExhaustedCreateReportsError)
{
    ErrorHandlerFake_Install(nullptr);
    FillPool();

    overflow = SolidSyslogOpenSslHmacSha256Policy_Create(&config);

    CHECK_REPORTED_ERROR(SOLIDSYSLOG_SEVERITY_ERROR, OPENSSLHMACSHA256POLICY_ERROR_POOL_EXHAUSTED);
}

TEST(SolidSyslogOpenSslHmacSha256Policy, NullConfigReturnsNullFallback)
{
    CHECK_IS_NULL_FALLBACK(SolidSyslogOpenSslHmacSha256Policy_Create(nullptr));
}

TEST(SolidSyslogOpenSslHmacSha256Policy, NullGetKeyReturnsNullFallback)
{
    config.GetKey = nullptr;

    CHECK_IS_NULL_FALLBACK(SolidSyslogOpenSslHmacSha256Policy_Create(&config));
}

TEST(SolidSyslogOpenSslHmacSha256Policy, BadConfigReportsError)
{
    ErrorHandlerFake_Install(nullptr);

    SolidSyslogOpenSslHmacSha256Policy_Create(nullptr);

    CHECK_REPORTED_ERROR(SOLIDSYSLOG_SEVERITY_ERROR, OPENSSLHMACSHA256POLICY_ERROR_BAD_CONFIG);
}

TEST(SolidSyslogOpenSslHmacSha256Policy, CreateAcquiresAndReleasesConfigLockOnFirstFreeSlot)
{
    ConfigLockFake_Install();

    pooled[0] = SolidSyslogOpenSslHmacSha256Policy_Create(&config);

    CALLED_FAKE(ConfigLockFake_Lock, ONCE);
    CALLED_FAKE(ConfigLockFake_Unlock, ONCE);
}

TEST(SolidSyslogOpenSslHmacSha256Policy, DestroyOfPooledHandleLocksOnce)
{
    pooled[0] = SolidSyslogOpenSslHmacSha256Policy_Create(&config);
    ConfigLockFake_Install();

    SolidSyslogOpenSslHmacSha256Policy_Destroy(pooled[0]);
    pooled[0] = nullptr;

    CALLED_FAKE(ConfigLockFake_Lock, ONCE);
    CALLED_FAKE(ConfigLockFake_Unlock, ONCE);
}

TEST(SolidSyslogOpenSslHmacSha256Policy, DestroyOfUnknownHandleReportsWarning)
{
    ErrorHandlerFake_Install(nullptr);
    struct SolidSyslogSecurityPolicy stranger = {};

    SolidSyslogOpenSslHmacSha256Policy_Destroy(&stranger);

    CHECK_REPORTED_ERROR(SOLIDSYSLOG_SEVERITY_WARNING, OPENSSLHMACSHA256POLICY_ERROR_UNKNOWN_DESTROY);
}

TEST(SolidSyslogOpenSslHmacSha256Policy, DestroyOfStaleHandleReportsWarning)
{
    pooled[0] = SolidSyslogOpenSslHmacSha256Policy_Create(&config);
    SolidSyslogOpenSslHmacSha256Policy_Destroy(pooled[0]);
    ErrorHandlerFake_Install(nullptr);

    SolidSyslogOpenSslHmacSha256Policy_Destroy(pooled[0]);
    pooled[0] = nullptr;

    CHECK_REPORTED_ERROR(SOLIDSYSLOG_SEVERITY_WARNING, OPENSSLHMACSHA256POLICY_ERROR_UNKNOWN_DESTROY);
}

TEST(SolidSyslogOpenSslHmacSha256PolicySeal, ComputeIntegrityHmacsTheRecordWithTheFetchedKey)
{
    uint8_t expectedKey[TEST_KEY_SIZE];
    memset(expectedKey, TEST_KEY_BYTE, sizeof expectedKey);
    uint8_t tag[HMAC_SHA256_TAG_SIZE] = {};

    seal(TEST_RECORD, sizeof TEST_RECORD, tag);

    LONGS_EQUAL(1, OpenSslFake_HmacCallCount());
    POINTERS_EQUAL(EVP_sha256(), OpenSslFake_LastHmacMd());
    LONGS_EQUAL(TEST_KEY_SIZE, OpenSslFake_LastHmacKeyLen());
    MEMCMP_EQUAL(expectedKey, OpenSslFake_LastHmacKey(), TEST_KEY_SIZE);
    LONGS_EQUAL(sizeof TEST_RECORD, OpenSslFake_LastHmacInputLen());
    MEMCMP_EQUAL(TEST_RECORD, OpenSslFake_LastHmacInput(), sizeof TEST_RECORD);
}

TEST(SolidSyslogOpenSslHmacSha256PolicySeal, ComputeIntegrityWritesTheTagIntoTheIntegrityBuffer)
{
    uint8_t expected[HMAC_SHA256_TAG_SIZE];
    expectedTagFor(TEST_RECORD, sizeof TEST_RECORD, expected);
    uint8_t tag[HMAC_SHA256_TAG_SIZE] = {};

    seal(TEST_RECORD, sizeof TEST_RECORD, tag);

    MEMCMP_EQUAL(expected, tag, HMAC_SHA256_TAG_SIZE);
}

TEST(SolidSyslogOpenSslHmacSha256PolicySeal, ComputeIntegrityReportsHmacFailure)
{
    ErrorHandlerFake_Install(nullptr);
    OpenSslFake_SetHmacFails(true);
    uint8_t tag[HMAC_SHA256_TAG_SIZE] = {};

    bool sealed = seal(TEST_RECORD, sizeof TEST_RECORD, tag);

    CHECK_FALSE(sealed);
    CHECK_REPORTED_ERROR(SOLIDSYSLOG_SEVERITY_ERROR, OPENSSLHMACSHA256POLICY_ERROR_HMAC_FAILED);
}

TEST(SolidSyslogOpenSslHmacSha256PolicySeal, ComputeIntegrityReportsKeyUnavailable)
{
    ErrorHandlerFake_Install(nullptr);
    keyAvailable = false;
    uint8_t tag[HMAC_SHA256_TAG_SIZE] = {};

    bool sealed = seal(TEST_RECORD, sizeof TEST_RECORD, tag);

    CHECK_FALSE(sealed);
    CHECK_REPORTED_ERROR(SOLIDSYSLOG_SEVERITY_ERROR, OPENSSLHMACSHA256POLICY_ERROR_KEY_UNAVAILABLE);
}

TEST(SolidSyslogOpenSslHmacSha256PolicySeal, ComputeIntegrityWipesTheKeyBufferAfterUse)
{
    uint8_t tag[HMAC_SHA256_TAG_SIZE] = {};

    seal(TEST_RECORD, sizeof TEST_RECORD, tag);

    LONGS_EQUAL(1, OpenSslFake_CleanseCallCount());
    POINTERS_EQUAL(lastGetKeyBuffer, OpenSslFake_LastCleanseBuf());
    LONGS_EQUAL(lastGetKeyCapacity, OpenSslFake_LastCleanseLen());
}

TEST(SolidSyslogOpenSslHmacSha256PolicySeal, VerifyIntegrityAcceptsATagItProduced)
{
    uint8_t tag[HMAC_SHA256_TAG_SIZE] = {};
    seal(TEST_RECORD, sizeof TEST_RECORD, tag);

    CHECK_TRUE(verify(TEST_RECORD, sizeof TEST_RECORD, tag));
}

TEST(SolidSyslogOpenSslHmacSha256PolicySeal, VerifyIntegrityRejectsAModifiedTag)
{
    uint8_t tag[HMAC_SHA256_TAG_SIZE] = {};
    seal(TEST_RECORD, sizeof TEST_RECORD, tag);
    tag[0] ^= 0xFFU;

    CHECK_FALSE(verify(TEST_RECORD, sizeof TEST_RECORD, tag));
}

TEST(SolidSyslogOpenSslHmacSha256PolicySeal, VerifyIntegrityFailsClosedWhenKeyUnavailable)
{
    uint8_t tag[HMAC_SHA256_TAG_SIZE] = {};
    seal(TEST_RECORD, sizeof TEST_RECORD, tag);
    keyAvailable = false;

    CHECK_FALSE(verify(TEST_RECORD, sizeof TEST_RECORD, tag));
}
