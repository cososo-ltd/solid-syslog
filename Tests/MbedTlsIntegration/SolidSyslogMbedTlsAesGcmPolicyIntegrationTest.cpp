/* Exercises SolidSyslogMbedTlsAesGcmPolicy against real libmbedcrypto (the unit
 * tests run against the deterministic MbedTlsFake capture double). Proves the
 * genuine AES-256-GCM round-trip, that the body is actually encrypted, that each
 * seal draws a fresh nonce from the injected CTR-DRBG, and that tampering or a
 * wrong key is rejected. */
#include <cstring>

#include <mbedtls/ctr_drbg.h>
#include <mbedtls/entropy.h>

#include "CppUTest/TestHarness.h"

extern "C"
{
#include "SolidSyslogMbedTlsAesGcmPolicy.h"
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
} // namespace

// clang-format off
TEST_GROUP(SolidSyslogMbedTlsAesGcmPolicyIntegration)
{
    mbedtls_entropy_context  entropy = {};
    mbedtls_ctr_drbg_context rng     = {};
    struct SolidSyslogSecurityPolicy* policy = nullptr;
    uint8_t content[CONTENT_LEN] = {};
    uint8_t originalBody[BODY_LEN] = {};
    uint8_t trailer[TRAILER_SIZE] = {};

    void setup() override
    {
        SetKey(0x42);
        mbedtls_entropy_init(&entropy);
        mbedtls_ctr_drbg_init(&rng);
        static const unsigned char pers[] = "solidsyslog-aesgcm-itest";
        LONGS_EQUAL(0, mbedtls_ctr_drbg_seed(&rng, mbedtls_entropy_func, &entropy, pers, sizeof(pers) - 1U));

        struct SolidSyslogMbedTlsAesGcmPolicyConfig config = {ProvideKey, nullptr, &rng};
        policy = SolidSyslogMbedTlsAesGcmPolicy_Create(&config);

        static const uint8_t header[HEADER_LEN] = {0xA5, 0x5A, 0x0B, 0x00};
        static const uint8_t body[BODY_LEN] = {'t', 'o', 'p', '-', 's', 'e', 'c', 'r', 'e', 't', '!'};
        memcpy(content, header, HEADER_LEN);
        memcpy(&content[HEADER_LEN], body, BODY_LEN);
        memcpy(originalBody, body, BODY_LEN);
    }

    void teardown() override
    {
        SolidSyslogMbedTlsAesGcmPolicy_Destroy(policy);
        mbedtls_ctr_drbg_free(&rng);
        mbedtls_entropy_free(&entropy);
    }

    bool seal()
    {
        struct SolidSyslogSecurityRecord rec = {content, CONTENT_LEN, HEADER_LEN, trailer};
        return policy->SealRecord(policy, &rec);
    }

    bool open()
    {
        struct SolidSyslogSecurityRecord rec = {content, CONTENT_LEN, HEADER_LEN, trailer};
        return policy->OpenRecord(policy, &rec);
    }

    uint8_t* body()
    {
        return &content[HEADER_LEN];
    }
};

// clang-format on

TEST(SolidSyslogMbedTlsAesGcmPolicyIntegration, SealEncryptsBodyAndOpenRestoresIt)
{
    CHECK_TRUE(seal());
    CHECK_TEXT(memcmp(body(), originalBody, BODY_LEN) != 0, "body was not encrypted");

    CHECK_TRUE(open());
    MEMCMP_EQUAL(originalBody, body(), BODY_LEN);
}

TEST(SolidSyslogMbedTlsAesGcmPolicyIntegration, HeaderStaysInClear)
{
    static const uint8_t header[HEADER_LEN] = {0xA5, 0x5A, 0x0B, 0x00};

    CHECK_TRUE(seal());

    MEMCMP_EQUAL(header, content, HEADER_LEN);
}

TEST(SolidSyslogMbedTlsAesGcmPolicyIntegration, EachSealDrawsAFreshNonce)
{
    uint8_t firstNonce[NONCE_SIZE];
    CHECK_TRUE(seal());
    memcpy(firstNonce, trailer, NONCE_SIZE);

    /* Reset the body and seal again; the random nonce must differ. */
    memcpy(body(), originalBody, BODY_LEN);
    CHECK_TRUE(seal());

    CHECK_TEXT(memcmp(firstNonce, trailer, NONCE_SIZE) != 0, "nonce was reused across seals");
}

TEST(SolidSyslogMbedTlsAesGcmPolicyIntegration, TamperedCiphertextIsRejected)
{
    CHECK_TRUE(seal());
    content[HEADER_LEN] ^= 0xFFU;

    CHECK_FALSE(open());
}

TEST(SolidSyslogMbedTlsAesGcmPolicyIntegration, TamperedTagIsRejected)
{
    CHECK_TRUE(seal());
    trailer[NONCE_SIZE] ^= 0xFFU;

    CHECK_FALSE(open());
}

TEST(SolidSyslogMbedTlsAesGcmPolicyIntegration, TamperedHeaderIsRejected)
{
    CHECK_TRUE(seal());
    content[0] ^= 0xFFU;

    CHECK_FALSE(open());
}

TEST(SolidSyslogMbedTlsAesGcmPolicyIntegration, WrongKeyIsRejected)
{
    CHECK_TRUE(seal());
    SetKey(0x99);

    CHECK_FALSE(open());
}
