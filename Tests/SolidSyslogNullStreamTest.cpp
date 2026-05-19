#include "CppUTest/TestHarness.h"
#include "SolidSyslogNullStream.h"
#include "SolidSyslogStream.h"

// clang-format off
TEST_GROUP(SolidSyslogNullStream)
{
    struct SolidSyslogStream* stream = nullptr;

    void setup() override
    {
        // cppcheck-suppress unreadVariable -- used across TEST_GROUP methods; cppcheck does not model CppUTest macros
        stream = SolidSyslogNullStream_Get();
    }
};

// clang-format on

TEST(SolidSyslogNullStream, SendReturnsTrueToDropOnTheFloor)
{
    CHECK_TRUE(SolidSyslogStream_Send(stream, "x", 1));
}

TEST(SolidSyslogNullStream, OpenReturnsTrue)
{
    CHECK_TRUE(SolidSyslogStream_Open(stream, nullptr));
}

TEST(SolidSyslogNullStream, ReadReturnsZeroForWouldBlock)
{
    char buffer[1] = {0};
    CHECK_EQUAL(0, SolidSyslogStream_Read(stream, buffer, sizeof(buffer)));
}

TEST(SolidSyslogNullStream, CloseDoesNotCrash)
{
    SolidSyslogStream_Close(stream);
}
