#include "CppUTest/TestHarness.h"
#include "SolidSyslogNullSender.h"
#include "SolidSyslogSender.h"

// clang-format off
TEST_GROUP(SolidSyslogNullSender)
{
    struct SolidSyslogSender* sender = nullptr;

    void setup() override
    {
        sender = SolidSyslogNullSender_Get();
    }
};

// clang-format on

TEST(SolidSyslogNullSender, SendReturnsTrueToDropOnTheFloor)
{
    CHECK_TRUE(SolidSyslogSender_Send(sender, "hello", 5));
}

TEST(SolidSyslogNullSender, DisconnectDoesNotCrash)
{
    SolidSyslogSender_Disconnect(sender);
}
