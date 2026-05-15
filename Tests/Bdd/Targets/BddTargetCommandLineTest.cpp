#include <unistd.h>

#include "BddTargetCommandLine.h"
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
    LONGS_EQUAL(10, options.MaxBlocks);
}

TEST(BddTargetCommandLine, DefaultMaxBlockSize)
{
    char arg0[] = "test";
    char* argv[] = {arg0, nullptr};
    Parse(1, argv);
    LONGS_EQUAL(65536, options.MaxBlockSize);
}

TEST(BddTargetCommandLine, DefaultDiscardPolicy)
{
    char arg0[] = "test";
    char* argv[] = {arg0, nullptr};
    Parse(1, argv);
    STRCMP_EQUAL("oldest", options.DiscardPolicy);
}

TEST(BddTargetCommandLine, DefaultNoSd)
{
    char arg0[] = "test";
    char* argv[] = {arg0, nullptr};
    Parse(1, argv);
    CHECK_FALSE(options.NoSd);
}

TEST(BddTargetCommandLine, MaxBlocksFlag)
{
    char arg0[] = "test";
    char arg1[] = "--max-blocks";
    char arg2[] = "5";
    char* argv[] = {arg0, arg1, arg2, nullptr};
    LONGS_EQUAL(0, Parse(3, argv));
    LONGS_EQUAL(5, options.MaxBlocks);
}

TEST(BddTargetCommandLine, MaxBlockSizeFlag)
{
    char arg0[] = "test";
    char arg1[] = "--max-block-size";
    char arg2[] = "1024";
    char* argv[] = {arg0, arg1, arg2, nullptr};
    LONGS_EQUAL(0, Parse(3, argv));
    LONGS_EQUAL(1024, options.MaxBlockSize);
}

TEST(BddTargetCommandLine, DiscardPolicyOldest)
{
    char arg0[] = "test";
    char arg1[] = "--discard-policy";
    char arg2[] = "oldest";
    char* argv[] = {arg0, arg1, arg2, nullptr};
    LONGS_EQUAL(0, Parse(3, argv));
    STRCMP_EQUAL("oldest", options.DiscardPolicy);
}

TEST(BddTargetCommandLine, DiscardPolicyNewest)
{
    char arg0[] = "test";
    char arg1[] = "--discard-policy";
    char arg2[] = "newest";
    char* argv[] = {arg0, arg1, arg2, nullptr};
    LONGS_EQUAL(0, Parse(3, argv));
    STRCMP_EQUAL("newest", options.DiscardPolicy);
}

TEST(BddTargetCommandLine, DiscardPolicyHalt)
{
    char arg0[] = "test";
    char arg1[] = "--discard-policy";
    char arg2[] = "halt";
    char* argv[] = {arg0, arg1, arg2, nullptr};
    LONGS_EQUAL(0, Parse(3, argv));
    STRCMP_EQUAL("halt", options.DiscardPolicy);
}

TEST(BddTargetCommandLine, InvalidDiscardPolicyReturnsOne)
{
    char arg0[] = "test";
    char arg1[] = "--discard-policy";
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
    CHECK_TRUE(options.NoSd);
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
