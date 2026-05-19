#include <stdint.h>
#include <cstring>

#include "CppUTest/TestHarness.h"
#include "ErrorHandlerFake.h"
#include "SolidSyslogFormatter.h"
#include "SolidSyslogStructuredData.h"
#include "SolidSyslogTimeQuality.h"
#include "SolidSyslogTimeQualitySd.h"
#include "SolidSyslogTunables.h"
#include "TestUtils.h"

using namespace CososoTesting; // NOLINT(google-build-using-namespace) -- test-file scope only; brings ONCE/NEVER into scope for CALLED_FAKE

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
        SolidSyslogTimeQualitySd_Destroy(sd);
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

TEST(SolidSyslogTimeQualitySd, UseAfterDestroyIsCrashSafeViaNullSdVtable)
{
    /* After Destroy the slot's abstract-base vtable is the shared NullSd's, so
     * Format through the stale handle is a safe no-op rather than a NULL-fn-pointer
     * crash. NullSd.Format writes nothing into the Formatter. */
    SolidSyslogTimeQualitySd_Destroy(sd);
    SolidSyslogStructuredData_Format(sd, formatter);
    LONGS_EQUAL(0, SolidSyslogFormatter_Length(formatter));
    sd = SolidSyslogTimeQualitySd_Create(StubGetTimeQuality); // for teardown
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

// Pool tests — prove SOLIDSYSLOG_TIME_QUALITY_SD_POOL_SIZE caps live
// instances and overflow falls back to the shared SolidSyslogNullSd.

// clang-format off
TEST_GROUP(SolidSyslogTimeQualitySdPool)
{
    // cppcheck-suppress constVariable -- assigned in TEST() bodies; cppcheck does not model CppUTest macros
    struct SolidSyslogStructuredData* pooled[SOLIDSYSLOG_TIME_QUALITY_SD_POOL_SIZE] = {};
    struct SolidSyslogStructuredData* overflow                                       = nullptr;

    void teardown() override
    {
        for (auto* handle : pooled)
        {
            if (handle != nullptr)
            {
                SolidSyslogTimeQualitySd_Destroy(handle);
            }
        }
        // cppcheck-suppress knownConditionTrueFalse -- assigned in TEST() bodies; cppcheck does not model CppUTest macros
        if (overflow != nullptr)
        {
            SolidSyslogTimeQualitySd_Destroy(overflow);
        }
    }

    static struct SolidSyslogStructuredData* MakeSd()
    {
        return SolidSyslogTimeQualitySd_Create(StubGetTimeQuality);
    }

    void FillPool()
    {
        for (auto*& slot : pooled)
        {
            slot = MakeSd();
        }
    }
};

// clang-format on

TEST(SolidSyslogTimeQualitySdPool, FillingPoolThenOverflowReturnsDistinctFallback)
{
    FillPool();

    overflow = MakeSd();

    CHECK_TEXT(overflow != nullptr, "Fallback handle was nullptr");
    for (auto* slot : pooled)
    {
        CHECK_TEXT(slot != nullptr, "pool slot was nullptr (FillPool failed?)");
        CHECK_TEXT(overflow != slot, "Fallback handle collided with a pool slot");
    }
}

// Bad-setup test — _Create rejects NULL callback and routes to NullSd.

// clang-format off
TEST_GROUP(SolidSyslogTimeQualitySdBadSetup)
{
    int sentinel = 0;

    void setup() override
    {
        ErrorHandlerFake_Install(&sentinel);
    }

    void teardown() override
    {
        ErrorHandlerFake_Uninstall();
    }
};

// clang-format on

TEST(SolidSyslogTimeQualitySdBadSetup, CreateWithNullCallbackReportsError)
{
    SolidSyslogTimeQualitySd_Create(nullptr);
    CHECK_REPORTED_ERROR("SolidSyslogTimeQualitySd_Create called with NULL getTimeQuality");
}
