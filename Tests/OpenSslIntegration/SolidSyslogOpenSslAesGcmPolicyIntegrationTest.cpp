/* Exercises SolidSyslogOpenSslAesGcmPolicy against the real OpenSSL libcrypto
 * (the unit tests run against the deterministic OpenSslFake). Proves the
 * genuine AES-256-GCM round-trip, that the body is actually encrypted, that
 * each seal draws a fresh nonce, and that tampering or a wrong key is rejected. */
#include <cstring>

#include "CppUTest/TestHarness.h"

extern "C"
{
#include "SolidSyslogOpenSslAesGcmPolicy.h"
#include "SolidSyslogSecurityPolicyDefinition.h"
}

namespace
{
enum
{
    KEY_SIZE = 32,
    NONCE_SIZE = 12,
    TAG_SIZE = 16,
    TRAILER_SIZE = NONCE_SIZE + TAG_SIZE,
    HEADER_LEN = 4,
    BODY_LEN = 11,
    CONTENT_LEN = HEADER_LEN + BODY_LEN
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
}

// clang-format off
TEST_GROUP(SolidSyslogOpenSslAesGcmPolicyIntegration)
{
    struct SolidSyslogSecurityPolicy* policy = nullptr;
    uint8_t content[CONTENT_LEN] = {};
    uint8_t originalBody[BODY_LEN] = {};
    uint8_t trailer[TRAILER_SIZE] = {};

    void setup() override
    {
        SetKey(0x42);
        struct SolidSyslogOpenSslAesGcmPolicyConfig config = {ProvideKey, nullptr};
        policy = SolidSyslogOpenSslAesGcmPolicy_Create(&config);

        static const uint8_t header[HEADER_LEN] = {0xA5, 0x5A, 0x0B, 0x00};
        static const uint8_t body[BODY_LEN] = {'t', 'o', 'p', '-', 's', 'e', 'c', 'r', 'e', 't', '!'};
        memcpy(content, header, HEADER_LEN);
        memcpy(&content[HEADER_LEN], body, BODY_LEN);
        memcpy(originalBody, body, BODY_LEN);
    }

    void teardown() override
    {
        SolidSyslogOpenSslAesGcmPolicy_Destroy(policy);
    }

    bool seal()
    {
        return policy->SealRecord(policy, content, CONTENT_LEN, HEADER_LEN, trailer);
    }

    bool open()
    {
        return policy->OpenRecord(policy, content, CONTENT_LEN, HEADER_LEN, trailer);
    }

    uint8_t* body()
    {
        return &content[HEADER_LEN];
    }
};

// clang-format on

TEST(SolidSyslogOpenSslAesGcmPolicyIntegration, SealEncryptsBodyAndOpenRestoresIt)
{
    CHECK_TRUE(seal());
    CHECK_TEXT(memcmp(body(), originalBody, BODY_LEN) != 0, "body was not encrypted");

    CHECK_TRUE(open());
    MEMCMP_EQUAL(originalBody, body(), BODY_LEN);
}

TEST(SolidSyslogOpenSslAesGcmPolicyIntegration, HeaderStaysInClear)
{
    static const uint8_t header[HEADER_LEN] = {0xA5, 0x5A, 0x0B, 0x00};

    CHECK_TRUE(seal());

    MEMCMP_EQUAL(header, content, HEADER_LEN);
}

TEST(SolidSyslogOpenSslAesGcmPolicyIntegration, EachSealDrawsAFreshNonce)
{
    uint8_t firstNonce[NONCE_SIZE];
    CHECK_TRUE(seal());
    memcpy(firstNonce, trailer, NONCE_SIZE);

    /* Reset the body and seal again; the random nonce must differ. */
    memcpy(body(), originalBody, BODY_LEN);
    CHECK_TRUE(seal());

    CHECK_TEXT(memcmp(firstNonce, trailer, NONCE_SIZE) != 0, "nonce was reused across seals");
}

TEST(SolidSyslogOpenSslAesGcmPolicyIntegration, TamperedCiphertextIsRejected)
{
    CHECK_TRUE(seal());
    content[HEADER_LEN] ^= 0xFFU;

    CHECK_FALSE(open());
}

TEST(SolidSyslogOpenSslAesGcmPolicyIntegration, TamperedTagIsRejected)
{
    CHECK_TRUE(seal());
    trailer[NONCE_SIZE] ^= 0xFFU;

    CHECK_FALSE(open());
}

TEST(SolidSyslogOpenSslAesGcmPolicyIntegration, TamperedHeaderIsRejected)
{
    CHECK_TRUE(seal());
    content[0] ^= 0xFFU;

    CHECK_FALSE(open());
}

TEST(SolidSyslogOpenSslAesGcmPolicyIntegration, WrongKeyIsRejected)
{
    CHECK_TRUE(seal());
    SetKey(0x99);

    CHECK_FALSE(open());
}
