#include <unistd.h>

#include "ExampleCommandLine.h"
#include "CppUTest/TestHarness.h"

// clang-format off
TEST_GROUP(ExampleCommandLine)
{
    struct ExampleOptions options = {};

    void setup() override
    {
        optind = 1;
    }

    int Parse(int argc, char* argv[])
    {
        return ExampleCommandLine_Parse(argc, argv, &options);
    }
};

// clang-format on

TEST(ExampleCommandLine, DefaultMaxBlocks)
{
    char  arg0[] = "test";
    char* argv[] = {arg0, nullptr};
    Parse(1, argv);
    LONGS_EQUAL(10, options.maxBlocks);
}

TEST(ExampleCommandLine, DefaultMaxBlockSize)
{
    char  arg0[] = "test";
    char* argv[] = {arg0, nullptr};
    Parse(1, argv);
    LONGS_EQUAL(65536, options.maxBlockSize);
}

TEST(ExampleCommandLine, DefaultDiscardPolicy)
{
    char  arg0[] = "test";
    char* argv[] = {arg0, nullptr};
    Parse(1, argv);
    STRCMP_EQUAL("oldest", options.discardPolicy);
}

TEST(ExampleCommandLine, DefaultNoSd)
{
    char  arg0[] = "test";
    char* argv[] = {arg0, nullptr};
    Parse(1, argv);
    CHECK_FALSE(options.noSd);
}

TEST(ExampleCommandLine, MaxBlocksFlag)
{
    char  arg0[] = "test";
    char  arg1[] = "--max-blocks";
    char  arg2[] = "5";
    char* argv[] = {arg0, arg1, arg2, nullptr};
    LONGS_EQUAL(0, Parse(3, argv));
    LONGS_EQUAL(5, options.maxBlocks);
}

TEST(ExampleCommandLine, MaxBlockSizeFlag)
{
    char  arg0[] = "test";
    char  arg1[] = "--max-block-size";
    char  arg2[] = "1024";
    char* argv[] = {arg0, arg1, arg2, nullptr};
    LONGS_EQUAL(0, Parse(3, argv));
    LONGS_EQUAL(1024, options.maxBlockSize);
}

TEST(ExampleCommandLine, DiscardPolicyOldest)
{
    char  arg0[] = "test";
    char  arg1[] = "--discard-policy";
    char  arg2[] = "oldest";
    char* argv[] = {arg0, arg1, arg2, nullptr};
    LONGS_EQUAL(0, Parse(3, argv));
    STRCMP_EQUAL("oldest", options.discardPolicy);
}

TEST(ExampleCommandLine, DiscardPolicyNewest)
{
    char  arg0[] = "test";
    char  arg1[] = "--discard-policy";
    char  arg2[] = "newest";
    char* argv[] = {arg0, arg1, arg2, nullptr};
    LONGS_EQUAL(0, Parse(3, argv));
    STRCMP_EQUAL("newest", options.discardPolicy);
}

TEST(ExampleCommandLine, DiscardPolicyHalt)
{
    char  arg0[] = "test";
    char  arg1[] = "--discard-policy";
    char  arg2[] = "halt";
    char* argv[] = {arg0, arg1, arg2, nullptr};
    LONGS_EQUAL(0, Parse(3, argv));
    STRCMP_EQUAL("halt", options.discardPolicy);
}

TEST(ExampleCommandLine, InvalidDiscardPolicyReturnsOne)
{
    char  arg0[] = "test";
    char  arg1[] = "--discard-policy";
    char  arg2[] = "invalid";
    char* argv[] = {arg0, arg1, arg2, nullptr};
    LONGS_EQUAL(1, Parse(3, argv));
}

TEST(ExampleCommandLine, NegativeMaxBlocksReturnsOne)
{
    char  arg0[] = "test";
    char  arg1[] = "--max-blocks";
    char  arg2[] = "-1";
    char* argv[] = {arg0, arg1, arg2, nullptr};
    LONGS_EQUAL(1, Parse(3, argv));
}

TEST(ExampleCommandLine, NonNumericMaxBlocksReturnsOne)
{
    char  arg0[] = "test";
    char  arg1[] = "--max-blocks";
    char  arg2[] = "abc";
    char* argv[] = {arg0, arg1, arg2, nullptr};
    LONGS_EQUAL(1, Parse(3, argv));
}

TEST(ExampleCommandLine, TrailingTextMaxBlocksReturnsOne)
{
    char  arg0[] = "test";
    char  arg1[] = "--max-blocks";
    char  arg2[] = "5abc";
    char* argv[] = {arg0, arg1, arg2, nullptr};
    LONGS_EQUAL(1, Parse(3, argv));
}

TEST(ExampleCommandLine, NegativeMaxBlockSizeReturnsOne)
{
    char  arg0[] = "test";
    char  arg1[] = "--max-block-size";
    char  arg2[] = "-1";
    char* argv[] = {arg0, arg1, arg2, nullptr};
    LONGS_EQUAL(1, Parse(3, argv));
}

TEST(ExampleCommandLine, NonNumericMaxBlockSizeReturnsOne)
{
    char  arg0[] = "test";
    char  arg1[] = "--max-block-size";
    char  arg2[] = "abc";
    char* argv[] = {arg0, arg1, arg2, nullptr};
    LONGS_EQUAL(1, Parse(3, argv));
}

TEST(ExampleCommandLine, TrailingTextMaxBlockSizeReturnsOne)
{
    char  arg0[] = "test";
    char  arg1[] = "--max-block-size";
    char  arg2[] = "1024abc";
    char* argv[] = {arg0, arg1, arg2, nullptr};
    LONGS_EQUAL(1, Parse(3, argv));
}

TEST(ExampleCommandLine, TransportTlsAccepted)
{
    char  arg0[]    = "test";
    char  argFlag[] = "--transport";
    char  argVal[]  = "tls";
    char* argv[]    = {arg0, argFlag, argVal, nullptr};
    LONGS_EQUAL(0, Parse(3, argv));
    STRCMP_EQUAL("tls", options.transport);
}

TEST(ExampleCommandLine, NoSdFlag)
{
    char  arg0[] = "test";
    char  arg1[] = "--no-sd";
    char* argv[] = {arg0, arg1, nullptr};
    LONGS_EQUAL(0, Parse(2, argv));
    CHECK_TRUE(options.noSd);
}
