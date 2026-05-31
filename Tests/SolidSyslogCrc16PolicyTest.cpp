#include <stdint.h>

#include "CppUTest/TestHarness.h"
#include "SolidSyslogCrc16Policy.h"
#include "SolidSyslogSecurityPolicyDefinition.h"

// clang-format off
TEST_GROUP(SolidSyslogCrc16Policy)
{
    struct SolidSyslogSecurityPolicy* policy = nullptr;

    void setup() override
    {
        policy = SolidSyslogCrc16Policy_Create();
    }

    void teardown() override
    {
        SolidSyslogCrc16Policy_Destroy();
    }
};

// clang-format on

TEST(SolidSyslogCrc16Policy, CreateReturnsNonNull)
{
    CHECK_TRUE(policy != nullptr);
}

TEST(SolidSyslogCrc16Policy, IntegritySizeIsTwo)
{
    LONGS_EQUAL(2, policy->IntegritySize);
}

TEST(SolidSyslogCrc16Policy, ComputeIntegrityReturnsCrc16)
{
    const uint8_t data[] = "123456789";
    uint8_t integrity[2] = {};
    policy->ComputeIntegrity(policy, data, 9, integrity);
    /* CRC-16/CCITT-FALSE of "123456789" is 0x29B1 */
    BYTES_EQUAL(0x29, integrity[0]);
    BYTES_EQUAL(0xB1, integrity[1]);
}

TEST(SolidSyslogCrc16Policy, ComputeThenVerifyRoundTrip)
{
    const uint8_t data[] = "hello";
    uint8_t integrity[2] = {};
    policy->ComputeIntegrity(policy, data, 5, integrity);
    CHECK_TRUE(policy->VerifyIntegrity(policy, data, 5, integrity));
}

TEST(SolidSyslogCrc16Policy, VerifyDetectsSingleBitFlip)
{
    const uint8_t data[] = "hello";
    uint8_t integrity[2] = {};
    policy->ComputeIntegrity(policy, data, 5, integrity);
    integrity[0] ^= 0x01;
    CHECK_FALSE(policy->VerifyIntegrity(policy, data, 5, integrity));
}

TEST(SolidSyslogCrc16Policy, VerifyDetectsDataCorruption)
{
    uint8_t data[] = "hello";
    uint8_t integrity[2] = {};
    policy->ComputeIntegrity(policy, data, 5, integrity);
    data[0] ^= 0x01;
    CHECK_FALSE(policy->VerifyIntegrity(policy, data, 5, integrity));
}

TEST(SolidSyslogCrc16Policy, DestroyDoesNotCrash)
{
    SolidSyslogCrc16Policy_Destroy();
}
