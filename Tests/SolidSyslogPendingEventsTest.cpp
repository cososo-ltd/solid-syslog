#include <stddef.h>

#include "CppUTest/TestHarness.h"

#include "SolidSyslogBufferCategories.h"
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

TEST(SolidSyslogPendingEvents, ARaiseOfAReportedKindFillsItsSlot)
{
    static const struct SolidSyslogErrorSource source = {"test", nullptr};
    SolidSyslog_Error(SOLIDSYSLOG_SEVERITY_WARNING, &source, SOLIDSYSLOG_CAT_BUFFER_RECORDS_LOST, 7);

    CHECK(SolidSyslogPendingEvents_Take(SolidSyslogPendingEvents_IndexOf(SOLIDSYSLOG_CAT_BUFFER_RECORDS_LOST), &taken));
    LONGS_EQUAL(SOLIDSYSLOG_SEVERITY_WARNING, taken.Severity);
    POINTERS_EQUAL(&source, taken.Source);
    LONGS_EQUAL(SOLIDSYSLOG_CAT_BUFFER_RECORDS_LOST, taken.Category);
    LONGS_EQUAL(7, taken.Detail);
    CHECK_FALSE(taken.HasNativeCode);
}

TEST(SolidSyslogPendingEvents, ARaiseWithANativeCodeKeepsItInTheSlot)
{
    static const struct SolidSyslogErrorSource source = {"test", nullptr};
    SolidSyslog_ErrorWithNativeCode(SOLIDSYSLOG_SEVERITY_ERROR, &source, SOLIDSYSLOG_CAT_BUFFER_RECORDS_LOST, 7, -5);

    CHECK(SolidSyslogPendingEvents_Take(SolidSyslogPendingEvents_IndexOf(SOLIDSYSLOG_CAT_BUFFER_RECORDS_LOST), &taken));
    CHECK(taken.HasNativeCode);
    LONGS_EQUAL(-5, taken.NativeCode);
}
