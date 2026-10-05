#include "BddTargetStoreSettings.h"
#include "CppUTest/TestHarness.h"

// clang-format off
TEST_GROUP(BddTargetStoreSettings)
{
    void setup() override
    {
        BddTargetStoreSettings_Reset();
    }
};

// clang-format on

TEST(BddTargetStoreSettings, MaxBlocksIsTenUntilSet)
{
    LONGS_EQUAL(10, BddTargetStoreSettings_MaxBlocks());
}

TEST(BddTargetStoreSettings, SetMaxBlocksChangesIt)
{
    CHECK_TRUE(BddTargetStoreSettings_SetByName("max-blocks", "2"));

    LONGS_EQUAL(2, BddTargetStoreSettings_MaxBlocks());
}

TEST(BddTargetStoreSettings, MaxBlocksThatIsNotANumberIsRefused)
{
    CHECK_FALSE(BddTargetStoreSettings_SetByName("max-blocks", "two"));

    LONGS_EQUAL(10, BddTargetStoreSettings_MaxBlocks());
}

TEST(BddTargetStoreSettings, ANameItDoesNotHoldIsNotTaken)
{
    CHECK_FALSE(BddTargetStoreSettings_SetByName("msgid", "abc"));
}

TEST(BddTargetStoreSettings, MaxBlockSizeIs65536UntilSet)
{
    LONGS_EQUAL(65536, BddTargetStoreSettings_MaxBlockSize());
}

TEST(BddTargetStoreSettings, SetMaxBlockSizeChangesIt)
{
    CHECK_TRUE(BddTargetStoreSettings_SetByName("max-block-size", "520"));

    LONGS_EQUAL(520, BddTargetStoreSettings_MaxBlockSize());
}

TEST(BddTargetStoreSettings, MaxBlockSizeThatIsNotANumberIsRefused)
{
    CHECK_FALSE(BddTargetStoreSettings_SetByName("max-block-size", "big"));

    LONGS_EQUAL(65536, BddTargetStoreSettings_MaxBlockSize());
}

TEST(BddTargetStoreSettings, CapacityThresholdIsZeroUntilSet)
{
    LONGS_EQUAL(0, BddTargetStoreSettings_GetCapacityThreshold(nullptr));
}

TEST(BddTargetStoreSettings, SetCapacityThresholdChangesIt)
{
    CHECK_TRUE(BddTargetStoreSettings_SetByName("capacity-threshold", "200"));

    LONGS_EQUAL(200, BddTargetStoreSettings_GetCapacityThreshold(nullptr));
}

TEST(BddTargetStoreSettings, CapacityThresholdThatIsNotANumberIsRefused)
{
    CHECK_FALSE(BddTargetStoreSettings_SetByName("capacity-threshold", "-1"));

    LONGS_EQUAL(0, BddTargetStoreSettings_GetCapacityThreshold(nullptr));
}

TEST(BddTargetStoreSettings, DiscardPolicyIsOldestUntilSet)
{
    LONGS_EQUAL(SOLIDSYSLOG_DISCARD_POLICY_OLDEST, BddTargetStoreSettings_DiscardPolicy());
}

TEST(BddTargetStoreSettings, SetDiscardPolicyNewestChangesIt)
{
    CHECK_TRUE(BddTargetStoreSettings_SetByName("discard-policy", "newest"));

    LONGS_EQUAL(SOLIDSYSLOG_DISCARD_POLICY_NEWEST, BddTargetStoreSettings_DiscardPolicy());
}

TEST(BddTargetStoreSettings, SetDiscardPolicyHaltChangesIt)
{
    CHECK_TRUE(BddTargetStoreSettings_SetByName("discard-policy", "halt"));

    LONGS_EQUAL(SOLIDSYSLOG_DISCARD_POLICY_HALT, BddTargetStoreSettings_DiscardPolicy());
}

TEST(BddTargetStoreSettings, SetDiscardPolicyOldestChangesItBack)
{
    BddTargetStoreSettings_SetByName("discard-policy", "halt");

    CHECK_TRUE(BddTargetStoreSettings_SetByName("discard-policy", "oldest"));

    LONGS_EQUAL(SOLIDSYSLOG_DISCARD_POLICY_OLDEST, BddTargetStoreSettings_DiscardPolicy());
}

TEST(BddTargetStoreSettings, ADiscardPolicyItDoesNotKnowIsRefusedAndKeepsThePrevious)
{
    BddTargetStoreSettings_SetByName("discard-policy", "newest");

    CHECK_FALSE(BddTargetStoreSettings_SetByName("discard-policy", "random"));

    LONGS_EQUAL(SOLIDSYSLOG_DISCARD_POLICY_NEWEST, BddTargetStoreSettings_DiscardPolicy());
}

TEST(BddTargetStoreSettings, SecurityPolicyIsCrc16UntilSet)
{
    LONGS_EQUAL(BDD_TARGET_SECURITY_POLICY_CRC16, BddTargetStoreSettings_SecurityPolicy());
}

TEST(BddTargetStoreSettings, SetSecurityPolicyNullChangesIt)
{
    CHECK_TRUE(BddTargetStoreSettings_SetByName("security-policy", "null"));

    LONGS_EQUAL(BDD_TARGET_SECURITY_POLICY_NULL, BddTargetStoreSettings_SecurityPolicy());
}

TEST(BddTargetStoreSettings, SetSecurityPolicyHmacSha256ChangesIt)
{
    CHECK_TRUE(BddTargetStoreSettings_SetByName("security-policy", "hmac-sha256"));

    LONGS_EQUAL(BDD_TARGET_SECURITY_POLICY_HMAC_SHA256, BddTargetStoreSettings_SecurityPolicy());
}

TEST(BddTargetStoreSettings, SetSecurityPolicyAes256GcmChangesIt)
{
    CHECK_TRUE(BddTargetStoreSettings_SetByName("security-policy", "aes-256-gcm"));

    LONGS_EQUAL(BDD_TARGET_SECURITY_POLICY_AES_256_GCM, BddTargetStoreSettings_SecurityPolicy());
}

TEST(BddTargetStoreSettings, SetSecurityPolicyCrc16ChangesItBack)
{
    BddTargetStoreSettings_SetByName("security-policy", "null");

    CHECK_TRUE(BddTargetStoreSettings_SetByName("security-policy", "crc16"));

    LONGS_EQUAL(BDD_TARGET_SECURITY_POLICY_CRC16, BddTargetStoreSettings_SecurityPolicy());
}

TEST(BddTargetStoreSettings, ASecurityPolicyItDoesNotKnowIsRefusedAndKeepsThePrevious)
{
    BddTargetStoreSettings_SetByName("security-policy", "null");

    CHECK_FALSE(BddTargetStoreSettings_SetByName("security-policy", "rot13"));

    LONGS_EQUAL(BDD_TARGET_SECURITY_POLICY_NULL, BddTargetStoreSettings_SecurityPolicy());
}

TEST(BddTargetStoreSettings, HaltExitIsOffUntilSet)
{
    CHECK_FALSE(BddTargetStoreSettings_HaltExit());
}

TEST(BddTargetStoreSettings, SetHaltExitToANonZeroNumberTurnsItOn)
{
    CHECK_TRUE(BddTargetStoreSettings_SetByName("halt-exit", "1"));

    CHECK_TRUE(BddTargetStoreSettings_HaltExit());
}

TEST(BddTargetStoreSettings, SetHaltExitToZeroTurnsItOff)
{
    BddTargetStoreSettings_SetByName("halt-exit", "1");

    CHECK_TRUE(BddTargetStoreSettings_SetByName("halt-exit", "0"));

    CHECK_FALSE(BddTargetStoreSettings_HaltExit());
}

TEST(BddTargetStoreSettings, HaltExitThatIsNotANumberIsRefused)
{
    CHECK_FALSE(BddTargetStoreSettings_SetByName("halt-exit", "yes"));

    CHECK_FALSE(BddTargetStoreSettings_HaltExit());
}

TEST(BddTargetStoreSettings, NoSdIsOffUntilSet)
{
    CHECK_FALSE(BddTargetStoreSettings_NoSd());
}

TEST(BddTargetStoreSettings, SetNoSdToANonZeroNumberTurnsItOn)
{
    CHECK_TRUE(BddTargetStoreSettings_SetByName("no-sd", "1"));

    CHECK_TRUE(BddTargetStoreSettings_NoSd());
}

TEST(BddTargetStoreSettings, NoSdThatIsNotANumberIsRefused)
{
    CHECK_FALSE(BddTargetStoreSettings_SetByName("no-sd", "yes"));

    CHECK_FALSE(BddTargetStoreSettings_NoSd());
}
