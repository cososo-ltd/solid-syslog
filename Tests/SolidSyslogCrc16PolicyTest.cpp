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

TEST(SolidSyslogCrc16Policy, TrailerSizeIsTwo)
{
    LONGS_EQUAL(2, policy->TrailerSize);
}

TEST(SolidSyslogCrc16Policy, SealRecordWritesCrc16ToTrailer)
{
    uint8_t content[] = "123456789";
    uint8_t trailer[2] = {};
    policy->SealRecord(policy, content, 9, 0, trailer);
    /* CRC-16/CCITT-FALSE of "123456789" is 0x29B1 */
    BYTES_EQUAL(0x29, trailer[0]);
    BYTES_EQUAL(0xB1, trailer[1]);
}

TEST(SolidSyslogCrc16Policy, SealThenOpenRoundTrip)
{
    uint8_t content[] = "hello";
    uint8_t trailer[2] = {};
    policy->SealRecord(policy, content, 5, 0, trailer);
    CHECK_TRUE(policy->OpenRecord(policy, content, 5, 0, trailer));
}

TEST(SolidSyslogCrc16Policy, OpenDetectsTrailerBitFlip)
{
    uint8_t content[] = "hello";
    uint8_t trailer[2] = {};
    policy->SealRecord(policy, content, 5, 0, trailer);
    trailer[0] ^= 0x01;
    CHECK_FALSE(policy->OpenRecord(policy, content, 5, 0, trailer));
}

TEST(SolidSyslogCrc16Policy, OpenDetectsContentCorruption)
{
    uint8_t content[] = "hello";
    uint8_t trailer[2] = {};
    policy->SealRecord(policy, content, 5, 0, trailer);
    content[0] ^= 0x01;
    CHECK_FALSE(policy->OpenRecord(policy, content, 5, 0, trailer));
}

TEST(SolidSyslogCrc16Policy, DestroyDoesNotCrash)
{
    SolidSyslogCrc16Policy_Destroy();
}
