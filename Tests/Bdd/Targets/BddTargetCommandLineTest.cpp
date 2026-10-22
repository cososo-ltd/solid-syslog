#include <unistd.h>

#include "BddTargetCommandLine.h"
#include "BddTargetStoreSettings.h"
#include "CppUTest/TestHarness.h"

// clang-format off
TEST_GROUP(BddTargetCommandLine)
{
    struct BddTargetOptions options = {};

    void setup() override
    {
        optind = 1;
    }

    int Parse(int argc, char* argv[])
    {
        return BddTargetCommandLine_Parse(argc, argv, &options);
    }
};

// clang-format on

TEST(BddTargetCommandLine, DefaultMaxBlocks)
{
    char arg0[] = "test";
    char* argv[] = {arg0, nullptr};
    Parse(1, argv);
    LONGS_EQUAL(10, BddTargetStoreSettings_MaxBlocks());
}

TEST(BddTargetCommandLine, DefaultMaxBlockSize)
{
    char arg0[] = "test";
    char* argv[] = {arg0, nullptr};
    Parse(1, argv);
    LONGS_EQUAL(65536, BddTargetStoreSettings_MaxBlockSize());
}

TEST(BddTargetCommandLine, DefaultDiscardPolicy)
{
    char arg0[] = "test";
    char* argv[] = {arg0, nullptr};
    Parse(1, argv);
    LONGS_EQUAL(SOLIDSYSLOG_DISCARD_POLICY_OLDEST, BddTargetStoreSettings_GetDiscardPolicy(nullptr));
}

TEST(BddTargetCommandLine, DefaultNoSd)
{
    char arg0[] = "test";
    char* argv[] = {arg0, nullptr};
    Parse(1, argv);
    CHECK_FALSE(BddTargetStoreSettings_NoSd());
}

TEST(BddTargetCommandLine, MaxBlocksFlag)
{
    char arg0[] = "test";
    char arg1[] = "--max-blocks";
    char arg2[] = "5";
    char* argv[] = {arg0, arg1, arg2, nullptr};
    LONGS_EQUAL(0, Parse(3, argv));
    LONGS_EQUAL(5, BddTargetStoreSettings_MaxBlocks());
}

TEST(BddTargetCommandLine, MaxBlockSizeFlag)
{
    char arg0[] = "test";
    char arg1[] = "--max-block-size";
    char arg2[] = "1024";
    char* argv[] = {arg0, arg1, arg2, nullptr};
    LONGS_EQUAL(0, Parse(3, argv));
    LONGS_EQUAL(1024, BddTargetStoreSettings_MaxBlockSize());
}

TEST(BddTargetCommandLine, DiscardPolicyOldest)
{
    char arg0[] = "test";
    char arg1[] = "--discard-policy";
    char arg2[] = "oldest";
    char* argv[] = {arg0, arg1, arg2, nullptr};
    LONGS_EQUAL(0, Parse(3, argv));
    LONGS_EQUAL(SOLIDSYSLOG_DISCARD_POLICY_OLDEST, BddTargetStoreSettings_GetDiscardPolicy(nullptr));
}

TEST(BddTargetCommandLine, DiscardPolicyNewest)
{
    char arg0[] = "test";
    char arg1[] = "--discard-policy";
    char arg2[] = "newest";
    char* argv[] = {arg0, arg1, arg2, nullptr};
    LONGS_EQUAL(0, Parse(3, argv));
    LONGS_EQUAL(SOLIDSYSLOG_DISCARD_POLICY_NEWEST, BddTargetStoreSettings_GetDiscardPolicy(nullptr));
}

TEST(BddTargetCommandLine, DiscardPolicyHalt)
{
    char arg0[] = "test";
    char arg1[] = "--discard-policy";
    char arg2[] = "halt";
    char* argv[] = {arg0, arg1, arg2, nullptr};
    LONGS_EQUAL(0, Parse(3, argv));
    LONGS_EQUAL(SOLIDSYSLOG_DISCARD_POLICY_HALT, BddTargetStoreSettings_GetDiscardPolicy(nullptr));
}

TEST(BddTargetCommandLine, InvalidDiscardPolicyReturnsOne)
{
    char arg0[] = "test";
    char arg1[] = "--discard-policy";
    char arg2[] = "invalid";
    char* argv[] = {arg0, arg1, arg2, nullptr};
    LONGS_EQUAL(1, Parse(3, argv));
}

TEST(BddTargetCommandLine, DefaultSecurityPolicy)
{
    char arg0[] = "test";
    char* argv[] = {arg0, nullptr};
    Parse(1, argv);
    LONGS_EQUAL(BDD_TARGET_SECURITY_POLICY_CRC16, BddTargetStoreSettings_SecurityPolicy());
}

TEST(BddTargetCommandLine, SecurityPolicyCrc16)
{
    char arg0[] = "test";
    char arg1[] = "--security-policy";
    char arg2[] = "crc16";
    char* argv[] = {arg0, arg1, arg2, nullptr};
    LONGS_EQUAL(0, Parse(3, argv));
    LONGS_EQUAL(BDD_TARGET_SECURITY_POLICY_CRC16, BddTargetStoreSettings_SecurityPolicy());
}

TEST(BddTargetCommandLine, SecurityPolicyHmacSha256)
{
    char arg0[] = "test";
    char arg1[] = "--security-policy";
    char arg2[] = "hmac-sha256";
    char* argv[] = {arg0, arg1, arg2, nullptr};
    LONGS_EQUAL(0, Parse(3, argv));
    LONGS_EQUAL(BDD_TARGET_SECURITY_POLICY_HMAC_SHA256, BddTargetStoreSettings_SecurityPolicy());
}

TEST(BddTargetCommandLine, SecurityPolicyAes256Gcm)
{
    char arg0[] = "test";
    char arg1[] = "--security-policy";
    char arg2[] = "aes-256-gcm";
    char* argv[] = {arg0, arg1, arg2, nullptr};
    LONGS_EQUAL(0, Parse(3, argv));
    LONGS_EQUAL(BDD_TARGET_SECURITY_POLICY_AES_256_GCM, BddTargetStoreSettings_SecurityPolicy());
}

TEST(BddTargetCommandLine, SecurityPolicyNull)
{
    char arg0[] = "test";
    char arg1[] = "--security-policy";
    char arg2[] = "null";
    char* argv[] = {arg0, arg1, arg2, nullptr};
    LONGS_EQUAL(0, Parse(3, argv));
    LONGS_EQUAL(BDD_TARGET_SECURITY_POLICY_NULL, BddTargetStoreSettings_SecurityPolicy());
}

TEST(BddTargetCommandLine, InvalidSecurityPolicyReturnsOne)
{
    char arg0[] = "test";
    char arg1[] = "--security-policy";
    char arg2[] = "invalid";
    char* argv[] = {arg0, arg1, arg2, nullptr};
    LONGS_EQUAL(1, Parse(3, argv));
}

TEST(BddTargetCommandLine, NegativeMaxBlocksReturnsOne)
{
    char arg0[] = "test";
    char arg1[] = "--max-blocks";
    char arg2[] = "-1";
    char* argv[] = {arg0, arg1, arg2, nullptr};
    LONGS_EQUAL(1, Parse(3, argv));
}

TEST(BddTargetCommandLine, NonNumericMaxBlocksReturnsOne)
{
    char arg0[] = "test";
    char arg1[] = "--max-blocks";
    char arg2[] = "abc";
    char* argv[] = {arg0, arg1, arg2, nullptr};
    LONGS_EQUAL(1, Parse(3, argv));
}

TEST(BddTargetCommandLine, TrailingTextMaxBlocksReturnsOne)
{
    char arg0[] = "test";
    char arg1[] = "--max-blocks";
    char arg2[] = "5abc";
    char* argv[] = {arg0, arg1, arg2, nullptr};
    LONGS_EQUAL(1, Parse(3, argv));
}

TEST(BddTargetCommandLine, NegativeMaxBlockSizeReturnsOne)
{
    char arg0[] = "test";
    char arg1[] = "--max-block-size";
    char arg2[] = "-1";
    char* argv[] = {arg0, arg1, arg2, nullptr};
    LONGS_EQUAL(1, Parse(3, argv));
}

TEST(BddTargetCommandLine, NonNumericMaxBlockSizeReturnsOne)
{
    char arg0[] = "test";
    char arg1[] = "--max-block-size";
    char arg2[] = "abc";
    char* argv[] = {arg0, arg1, arg2, nullptr};
    LONGS_EQUAL(1, Parse(3, argv));
}

TEST(BddTargetCommandLine, TrailingTextMaxBlockSizeReturnsOne)
{
    char arg0[] = "test";
    char arg1[] = "--max-block-size";
    char arg2[] = "1024abc";
    char* argv[] = {arg0, arg1, arg2, nullptr};
    LONGS_EQUAL(1, Parse(3, argv));
}

TEST(BddTargetCommandLine, TransportTlsAccepted)
{
    char arg0[] = "test";
    char argFlag[] = "--transport";
    char argVal[] = "tls";
    char* argv[] = {arg0, argFlag, argVal, nullptr};
    LONGS_EQUAL(0, Parse(3, argv));
    STRCMP_EQUAL("tls", options.Transport);
}

TEST(BddTargetCommandLine, NoSdFlag)
{
    char arg0[] = "test";
    char arg1[] = "--no-sd";
    char* argv[] = {arg0, arg1, nullptr};
    LONGS_EQUAL(0, Parse(2, argv));
    CHECK_TRUE(BddTargetStoreSettings_NoSd());
}

TEST(BddTargetCommandLine, DefaultHaltExitIsOff)
{
    char arg0[] = "test";
    char* argv[] = {arg0, nullptr};
    LONGS_EQUAL(0, Parse(1, argv));
    CHECK_FALSE(BddTargetStoreSettings_HaltExit());
}

TEST(BddTargetCommandLine, HaltExitFlag)
{
    char arg0[] = "test";
    char arg1[] = "--halt-exit";
    char* argv[] = {arg0, arg1, nullptr};
    LONGS_EQUAL(0, Parse(2, argv));
    CHECK_TRUE(BddTargetStoreSettings_HaltExit());
}

TEST(BddTargetCommandLine, CapacityThresholdFlag)
{
    char arg0[] = "test";
    char arg1[] = "--capacity-threshold";
    char arg2[] = "200";
    char* argv[] = {arg0, arg1, arg2, nullptr};
    LONGS_EQUAL(0, Parse(3, argv));
    LONGS_EQUAL(200, BddTargetStoreSettings_GetCapacityThreshold(nullptr));
}

TEST(BddTargetCommandLine, DefaultAppNameIsNull)
{
    char arg0[] = "test";
    char* argv[] = {arg0, nullptr};
    LONGS_EQUAL(0, Parse(1, argv));
    POINTERS_EQUAL(nullptr, options.AppName);
}

TEST(BddTargetCommandLine, AppNameFlagSetsAppName)
{
    char arg0[] = "test";
    char arg1[] = "--app-name";
    char arg2[] = "SolidSyslogThreadedExample";
    char* argv[] = {arg0, arg1, arg2, nullptr};
    LONGS_EQUAL(0, Parse(3, argv));
    STRCMP_EQUAL("SolidSyslogThreadedExample", options.AppName);
}

TEST(BddTargetCommandLine, DefaultErrorsFatalIsOn)
{
    char arg0[] = "test";
    char* argv[] = {arg0, nullptr};
    LONGS_EQUAL(0, Parse(1, argv));
    STRCMP_EQUAL("1", options.ErrorsFatal);
}

TEST(BddTargetCommandLine, ErrorsFatalFlagSetsErrorsFatal)
{
    char arg0[] = "test";
    char arg1[] = "--errors-fatal";
    char arg2[] = "0";
    char* argv[] = {arg0, arg1, arg2, nullptr};
    LONGS_EQUAL(0, Parse(3, argv));
    STRCMP_EQUAL("0", options.ErrorsFatal);
}

TEST(BddTargetCommandLine, InvalidErrorsFatalReturnsOne)
{
    char arg0[] = "test";
    char arg1[] = "--errors-fatal";
    char arg2[] = "maybe";
    char* argv[] = {arg0, arg1, arg2, nullptr};
    LONGS_EQUAL(1, Parse(3, argv));
}
