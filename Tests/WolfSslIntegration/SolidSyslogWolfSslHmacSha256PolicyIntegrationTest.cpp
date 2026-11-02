/* Exercises SolidSyslogWolfSslHmacSha256Policy against real libwolfssl (the unit
 * tests run against the deterministic WolfSslFake double). Proves the genuine
 * HMAC-SHA256 round-trip: a tag the policy sealed opens, and tampering with the
 * record, the tag or the key is rejected. */
#include <cstring>

#include "CppUTest/TestHarness.h"

extern "C"
{
#include "SolidSyslogSecurityPolicyDefinition.h"
#include "SolidSyslogWolfSslHmacSha256Policy.h"
}

namespace
{
enum
{
    KEY_SIZE = 32,
    TAG_SIZE = 32,
    CONTENT_LEN = 15
};

/* A real 32-byte key, swappable so a test can open with the wrong key. */
uint8_t activeKey[KEY_SIZE];

bool ProvideKey(void* context, uint8_t* keyOut, size_t capacity, size_t* keyLengthOut)
{
    (void) context;
    if (capacity < KEY_SIZE)
    {
        return false;
    }
    memcpy(keyOut, activeKey, KEY_SIZE);
    *keyLengthOut = KEY_SIZE;
    return true;
}

void SetKey(uint8_t fill)
{
    memset(activeKey, fill, sizeof activeKey);
}
} // namespace

// clang-format off
TEST_GROUP(SolidSyslogWolfSslHmacSha256PolicyIntegration)
{
    struct SolidSyslogSecurityPolicy* policy = nullptr;
    uint8_t content[CONTENT_LEN] = {};
    uint8_t trailer[TAG_SIZE] = {};

    void setup() override
    {
        SetKey(0x42);
        struct SolidSyslogWolfSslHmacSha256PolicyConfig config = {ProvideKey, nullptr};
        policy = SolidSyslogWolfSslHmacSha256Policy_Create(&config);
        static const uint8_t record[CONTENT_LEN] =
            {0xA5, 0x5A, 0x0B, 0x00, 't', 'o', 'p', '-', 's', 'e', 'c', 'r', 'e', 't', '!'};
        memcpy(content, record, CONTENT_LEN);
    }

    void teardown() override
    {
        SolidSyslogWolfSslHmacSha256Policy_Destroy(policy);
    }

    bool seal()
    {
        struct SolidSyslogSecurityRecord rec = {content, CONTENT_LEN, 0, trailer};
        return policy->SealRecord(policy, &rec);
    }

    enum SolidSyslogSecurityPolicyOpenResult open()
    {
        struct SolidSyslogSecurityRecord rec = {content, CONTENT_LEN, 0, trailer};
        return policy->OpenRecord(policy, &rec);
    }
};

// clang-format on

TEST(SolidSyslogWolfSslHmacSha256PolicyIntegration, ASealedRecordOpens)
{
    CHECK_TRUE(seal());

    LONGS_EQUAL(SOLIDSYSLOG_SECURITY_POLICY_OPEN_OPENED, open());
}

TEST(SolidSyslogWolfSslHmacSha256PolicyIntegration, TheTagIsNotAllZero)
{
    static const uint8_t zero[TAG_SIZE] = {};

    CHECK_TRUE(seal());

    CHECK_TEXT(memcmp(trailer, zero, TAG_SIZE) != 0, "tag was not written");
}

TEST(SolidSyslogWolfSslHmacSha256PolicyIntegration, TamperedContentIsRejected)
{
    CHECK_TRUE(seal());
    content[4] ^= 0xFFU;

    LONGS_EQUAL(SOLIDSYSLOG_SECURITY_POLICY_OPEN_REJECTED, open());
}

TEST(SolidSyslogWolfSslHmacSha256PolicyIntegration, TamperedTagIsRejected)
{
    CHECK_TRUE(seal());
    trailer[TAG_SIZE - 1] ^= 0x01U;

    LONGS_EQUAL(SOLIDSYSLOG_SECURITY_POLICY_OPEN_REJECTED, open());
}

TEST(SolidSyslogWolfSslHmacSha256PolicyIntegration, WrongKeyIsRejected)
{
    CHECK_TRUE(seal());
    SetKey(0x99);

    LONGS_EQUAL(SOLIDSYSLOG_SECURITY_POLICY_OPEN_REJECTED, open());
}
