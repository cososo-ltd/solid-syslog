#include <stddef.h>

#include "CppUTest/TestHarness.h"

#include "SolidSyslogError.h"
#include "SolidSyslogPendingEvents.h"

// clang-format off
TEST_GROUP(SolidSyslogPendingEvents)
{
    struct SolidSyslogErrorEvent taken;

    void setup() override
    {
        SolidSyslogPendingEvents_Reset();
    }
};

// clang-format on

TEST(SolidSyslogPendingEvents, NothingIsPendingAfterReset)
{
    for (size_t index = 0U; index < SOLIDSYSLOG_PENDING_EVENT_KIND_COUNT; index++)
    {
        CHECK_FALSE(SolidSyslogPendingEvents_Take(index, &taken));
    }
}
