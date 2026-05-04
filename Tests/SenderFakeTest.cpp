#include "SenderFake.h"
#include "SolidSyslogSender.h"
#include "CppUTest/TestHarness.h"

// clang-format off
TEST_GROUP(SenderFake)
{
    struct SolidSyslogSender* sender = nullptr;

    void setup() override
    {
        // cppcheck-suppress unreadVariable -- used across TEST_GROUP methods; cppcheck does not model CppUTest macros
        sender = SenderFake_Create();
    }

    void teardown() override
    {
        SenderFake_Destroy(sender);
    }
};

// clang-format on

TEST(SenderFake, SendCountIsZeroAfterCreate)
{
    LONGS_EQUAL(0, SenderFake_SendCount(sender));
}

TEST(SenderFake, DisconnectCountIsZeroAfterCreate)
{
    LONGS_EQUAL(0, SenderFake_DisconnectCount(sender));
}

TEST(SenderFake, SendCountIncrementsOnSend)
{
    SolidSyslogSender_Send(sender, "a", 1);
    LONGS_EQUAL(1, SenderFake_SendCount(sender));
}

TEST(SenderFake, SendCountIncrementsTwiceOnTwoSends)
{
    SolidSyslogSender_Send(sender, "a", 1);
    SolidSyslogSender_Send(sender, "b", 1);
    LONGS_EQUAL(2, SenderFake_SendCount(sender));
}

TEST(SenderFake, DisconnectCountIncrementsOnDisconnect)
{
    SolidSyslogSender_Disconnect(sender);
    LONGS_EQUAL(1, SenderFake_DisconnectCount(sender));
}

TEST(SenderFake, DisconnectCountIncrementsTwiceOnTwoDisconnects)
{
    SolidSyslogSender_Disconnect(sender);
    SolidSyslogSender_Disconnect(sender);
    LONGS_EQUAL(2, SenderFake_DisconnectCount(sender));
}

TEST(SenderFake, LastBufferCapturesMessage)
{
    SolidSyslogSender_Send(sender, "hello", 5);
    STRCMP_EQUAL("hello", SenderFake_LastBufferAsString(sender));
}

TEST(SenderFake, LastBufferIsNullTerminated)
{
    SolidSyslogSender_Send(sender, "test", 4);
    LONGS_EQUAL(0, SenderFake_LastBufferAsString(sender)[4]);
}

TEST(SenderFake, LastBufferCapturesLastSend)
{
    SolidSyslogSender_Send(sender, "first", 5);
    SolidSyslogSender_Send(sender, "second", 6);
    STRCMP_EQUAL("second", SenderFake_LastBufferAsString(sender));
}

TEST(SenderFake, SendReturnsTrue)
{
    CHECK_TRUE(SolidSyslogSender_Send(sender, "a", 1));
}

TEST(SenderFake, ResetClearsLastBuffer)
{
    SolidSyslogSender_Send(sender, "hello", 5);
    SenderFake_Reset(sender);
    STRCMP_EQUAL("", SenderFake_LastBufferAsString(sender));
}

TEST(SenderFake, ResetClearsSendCount)
{
    SolidSyslogSender_Send(sender, "a", 1);
    SenderFake_Reset(sender);
    LONGS_EQUAL(0, SenderFake_SendCount(sender));
}

TEST(SenderFake, ResetClearsDisconnectCount)
{
    SolidSyslogSender_Disconnect(sender);
    SenderFake_Reset(sender);
    LONGS_EQUAL(0, SenderFake_DisconnectCount(sender));
}

TEST(SenderFake, FailNextSendReturnsFalse)
{
    SenderFake_FailNextSend(sender);
    CHECK_FALSE(SolidSyslogSender_Send(sender, "a", 1));
}

TEST(SenderFake, FailNextSendStillCapturesBuffer)
{
    SenderFake_FailNextSend(sender);
    SolidSyslogSender_Send(sender, "hello", 5);
    STRCMP_EQUAL("hello", SenderFake_LastBufferAsString(sender));
}

TEST(SenderFake, FailNextSendOnlyAffectsOneSend)
{
    SenderFake_FailNextSend(sender);
    CHECK_FALSE(SolidSyslogSender_Send(sender, "a", 1));
    CHECK_TRUE(SolidSyslogSender_Send(sender, "b", 1));
}

// clang-format off
TEST_GROUP(SenderFakeInstances)
{
    struct SolidSyslogSender* a = nullptr;
    struct SolidSyslogSender* b = nullptr;

    void setup() override
    {
        // cppcheck-suppress unreadVariable -- used across TEST_GROUP methods; cppcheck does not model CppUTest macros
        a = SenderFake_Create();
        // cppcheck-suppress unreadVariable -- used across TEST_GROUP methods; cppcheck does not model CppUTest macros
        b = SenderFake_Create();
    }

    void teardown() override
    {
        SenderFake_Destroy(b);
        SenderFake_Destroy(a);
    }
};

// clang-format on

TEST(SenderFakeInstances, TwoInstancesHaveDistinctHandles)
{
    CHECK(a != b);
}

TEST(SenderFakeInstances, SendCountsAreIndependent)
{
    SolidSyslogSender_Send(a, "x", 1);
    LONGS_EQUAL(1, SenderFake_SendCount(a));
    LONGS_EQUAL(0, SenderFake_SendCount(b));
}

TEST(SenderFakeInstances, DisconnectCountsAreIndependent)
{
    SolidSyslogSender_Disconnect(a);
    LONGS_EQUAL(1, SenderFake_DisconnectCount(a));
    LONGS_EQUAL(0, SenderFake_DisconnectCount(b));
}

TEST(SenderFakeInstances, LastBuffersAreIndependent)
{
    SolidSyslogSender_Send(a, "alpha", 5);
    SolidSyslogSender_Send(b, "beta", 4);
    STRCMP_EQUAL("alpha", SenderFake_LastBufferAsString(a));
    STRCMP_EQUAL("beta", SenderFake_LastBufferAsString(b));
}

TEST(SenderFakeInstances, FailNextSendIsIndependent)
{
    SenderFake_FailNextSend(a);
    CHECK_FALSE(SolidSyslogSender_Send(a, "a", 1));
    CHECK_TRUE(SolidSyslogSender_Send(b, "b", 1));
}
