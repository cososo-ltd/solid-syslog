#include "TestUtils.h"
#include "CppUTest/TestHarness.h"

using namespace CososoTesting;

#include <cstring>

#include "SolidSyslogTimestamp.h"
#include "SolidSyslogVxWorks64Clock.h"
#include "VxWorks64ClockFake.h"

// clang-format off
TEST_GROUP(SolidSyslogVxWorks64Clock)
{
    struct SolidSyslogTimestamp timestamp = {};

    void setup() override
    {
        VxWorks64ClockFake_Reset();
        (void) memset(&timestamp, 0xA5, sizeof(timestamp));
    }
};

// clang-format on

TEST(SolidSyslogVxWorks64Clock, TimestampIsZeroedWhenTheClockFails)
{
    VxWorks64ClockFake_FailClockGettime();

    SolidSyslogVxWorks64_GetTimestamp(&timestamp);

    const struct SolidSyslogTimestamp zeroed = {};
    MEMCMP_EQUAL(&zeroed, &timestamp, sizeof(timestamp));
}
