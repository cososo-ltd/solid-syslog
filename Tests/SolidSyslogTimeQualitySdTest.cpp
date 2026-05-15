#include <stdint.h>
#include <cstring>

#include "SolidSyslogFormatter.h"
#include "SolidSyslogTimeQualitySd.h"
#include "SolidSyslogStructuredData.h"
#include "SolidSyslogTimeQuality.h"
#include "CppUTest/TestHarness.h"

struct SolidSyslogFormatter;
struct SolidSyslogStructuredData;

enum
{
    TEST_BUFFER_SIZE = 256
};

static struct SolidSyslogTimeQuality stubTimeQuality;

static void StubGetTimeQuality(struct SolidSyslogTimeQuality* timeQuality)
{
    *timeQuality = stubTimeQuality;
}

// clang-format off
TEST_GROUP(SolidSyslogTimeQualitySd)
{
    // cppcheck-suppress variableScope -- member of TEST_GROUP; scope managed by CppUTest macro
    SolidSyslogStructuredData* sd;
    SolidSyslogFormatterStorage storage[SOLIDSYSLOG_FORMATTER_STORAGE_SIZE(TEST_BUFFER_SIZE)];
    // cppcheck-suppress variableScope -- member of TEST_GROUP; scope managed by CppUTest macro
    SolidSyslogFormatter* formatter;

    void setup() override
    {
        formatter = SolidSyslogFormatter_Create(storage, TEST_BUFFER_SIZE);
        stubTimeQuality = {true, true, SOLIDSYSLOG_SYNC_ACCURACY_OMIT};
        sd = SolidSyslogTimeQualitySd_Create(StubGetTimeQuality);
    }

    void teardown() override
    {
        SolidSyslogTimeQualitySd_Destroy();
    }

    void format() const
    {
        SolidSyslogStructuredData_Format(sd, formatter);
    }

    void resetFormatter()
    {
        formatter = SolidSyslogFormatter_Create(storage, TEST_BUFFER_SIZE);
    }
};

// clang-format on

TEST(SolidSyslogTimeQualitySd, CreateReturnsNonNull)
{
    CHECK(sd != nullptr);
}

TEST(SolidSyslogTimeQualitySd, FormatProducesTzKnownAndIsSynced)
{
    format();
    STRCMP_EQUAL("[timeQuality tzKnown=\"1\" isSynced=\"1\"]", SolidSyslogFormatter_AsFormattedBuffer(formatter));
}

TEST(SolidSyslogTimeQualitySd, FormatWithFalseValues)
{
    stubTimeQuality.TzKnown = false;
    stubTimeQuality.IsSynced = false;
    format();
    STRCMP_EQUAL("[timeQuality tzKnown=\"0\" isSynced=\"0\"]", SolidSyslogFormatter_AsFormattedBuffer(formatter));
}

TEST(SolidSyslogTimeQualitySd, FormatIncludesSyncAccuracyWhenNonZero)
{
    stubTimeQuality.SyncAccuracyMicroseconds = 50;
    format();
    STRCMP_EQUAL(
        "[timeQuality tzKnown=\"1\" isSynced=\"1\" syncAccuracy=\"50\"]",
        SolidSyslogFormatter_AsFormattedBuffer(formatter)
    );
}

TEST(SolidSyslogTimeQualitySd, SyncAccuracyOfOneIsSmallestNonOmitValue)
{
    stubTimeQuality.SyncAccuracyMicroseconds = 1;
    format();
    STRCMP_EQUAL(
        "[timeQuality tzKnown=\"1\" isSynced=\"1\" syncAccuracy=\"1\"]",
        SolidSyslogFormatter_AsFormattedBuffer(formatter)
    );
}

TEST(SolidSyslogTimeQualitySd, SyncAccuracyAtMaxUint32)
{
    stubTimeQuality.SyncAccuracyMicroseconds = UINT32_MAX;
    format();
    STRCMP_EQUAL(
        "[timeQuality tzKnown=\"1\" isSynced=\"1\" syncAccuracy=\"4294967295\"]",
        SolidSyslogFormatter_AsFormattedBuffer(formatter)
    );
}

TEST(SolidSyslogTimeQualitySd, OmitSyncAccuracyUsesDefinedConstant)
{
    stubTimeQuality.SyncAccuracyMicroseconds = SOLIDSYSLOG_SYNC_ACCURACY_OMIT;
    format();
    STRCMP_EQUAL("[timeQuality tzKnown=\"1\" isSynced=\"1\"]", SolidSyslogFormatter_AsFormattedBuffer(formatter));
}

TEST(SolidSyslogTimeQualitySd, CallbackIsInvokedOnEachFormat)
{
    format();
    STRCMP_EQUAL("[timeQuality tzKnown=\"1\" isSynced=\"1\"]", SolidSyslogFormatter_AsFormattedBuffer(formatter));

    stubTimeQuality.IsSynced = false;
    resetFormatter();
    format();
    STRCMP_EQUAL("[timeQuality tzKnown=\"1\" isSynced=\"0\"]", SolidSyslogFormatter_AsFormattedBuffer(formatter));
}

TEST(SolidSyslogTimeQualitySd, FormatAdvancesFormatterLength)
{
    LONGS_EQUAL(0, SolidSyslogFormatter_Length(formatter));
    format();
    CHECK(SolidSyslogFormatter_Length(formatter) > 0);
    LONGS_EQUAL(strlen(SolidSyslogFormatter_AsFormattedBuffer(formatter)), SolidSyslogFormatter_Length(formatter));
}

TEST(SolidSyslogTimeQualitySd, FormatAdvancesLengthWithSyncAccuracy)
{
    stubTimeQuality.SyncAccuracyMicroseconds = 50;
    format();
    LONGS_EQUAL(strlen(SolidSyslogFormatter_AsFormattedBuffer(formatter)), SolidSyslogFormatter_Length(formatter));
}

TEST(SolidSyslogTimeQualitySd, DestroyDoesNotCrash)
{
    // Covered by teardown — this test documents the intent
}
