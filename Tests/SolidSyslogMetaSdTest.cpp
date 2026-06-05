#include <cstdint>
#include <cstring>

#include "CppUTest/TestHarness.h"
#include "ErrorHandlerFake.h"
#include "SolidSyslogAtomicCounterTestHelper.h"
#include "SolidSyslogErrorCategory.h"
#include "SolidSyslogFormatter.h"
#include "SolidSyslogMetaSd.h"
#include "SolidSyslogMetaSdErrors.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogStructuredData.h"
#include "SolidSyslogTunables.h"
#include "TestUtils.h"

using namespace CososoTesting;

class TEST_SolidSyslogMetaSd_FirstFormatProducesSequenceId1_Test;
class TEST_SolidSyslogMetaSd_FormatEscapesBackslashInLanguage_Test;
class TEST_SolidSyslogMetaSd_FormatEscapesBracketInLanguage_Test;
class TEST_SolidSyslogMetaSd_FormatEscapesQuoteInLanguage_Test;
class TEST_SolidSyslogMetaSd_FormatIncludesDifferentLanguageFromCallback_Test;
class TEST_SolidSyslogMetaSd_FormatIncludesDifferentSysUpTimeFromCallback_Test;
class TEST_SolidSyslogMetaSd_FormatIncludesLanguageFromCallback_Test;
class TEST_SolidSyslogMetaSd_FormatIncludesSysUpTimeAtMaxUint32_Test;
class TEST_SolidSyslogMetaSd_FormatIncludesSysUpTimeAtZero_Test;
class TEST_SolidSyslogMetaSd_FormatIncludesSysUpTimeFromCallback_Test;
class TEST_SolidSyslogMetaSd_SecondFormatProducesSequenceId2_Test;
class TEST_SolidSyslogMetaSd_ThirdFormatProducesSequenceId3_Test;
struct SolidSyslogAtomicCounter;
struct SolidSyslogFormatter;
struct SolidSyslogStructuredData;

enum
{
    TEST_BUFFER_SIZE = 256
};

static uint32_t fakeSysUpTimeValue;

static uint32_t FakeSysUpTime_Get()
{
    return fakeSysUpTimeValue;
}

static const char* fakeLanguageContent;
static size_t fakeLanguageMaxLength;

static void FakeLanguage_Get(struct SolidSyslogFormatter* formatter)
{
    SolidSyslogFormatter_EscapedString(formatter, fakeLanguageContent, fakeLanguageMaxLength);
}

#define CHECK_SEQUENCEID(expected) \
    STRCMP_EQUAL("[meta sequenceId=\"" expected "\"]", SolidSyslogFormatter_AsFormattedBuffer(formatter))
#define CHECK_SYSUPTIME(expected)                             \
    STRCMP_EQUAL(                                             \
        "[meta sequenceId=\"1\" sysUpTime=\"" expected "\"]", \
        SolidSyslogFormatter_AsFormattedBuffer(formatter)     \
    )
#define CHECK_LANGUAGE(expected) \
    STRCMP_EQUAL("[meta sequenceId=\"1\" language=\"" expected "\"]", SolidSyslogFormatter_AsFormattedBuffer(formatter))

// clang-format off
TEST_GROUP(SolidSyslogMetaSd)
{
    SolidSyslogAtomicCounter* counter;
    SolidSyslogStructuredData* sd;
    SolidSyslogMetaSdConfig config;
    SolidSyslogFormatterStorage storage[SOLIDSYSLOG_FORMATTER_STORAGE_SIZE(TEST_BUFFER_SIZE)];
    SolidSyslogFormatter* formatter;
    int sentinel = 0;

    void setup() override
    {
        ErrorHandlerFake_Install(&sentinel);
        formatter = SolidSyslogFormatter_Create(storage, TEST_BUFFER_SIZE);
        counter = TestAtomicCounter_Create();
        fakeSysUpTimeValue = 0;
        fakeLanguageContent = nullptr;
        fakeLanguageMaxLength = 0;
        config = {};
        config.Counter = counter;
        sd = SolidSyslogMetaSd_Create(&config);
    }

    void teardown() override
    {
        SolidSyslogMetaSd_Destroy(sd);
        TestAtomicCounter_Destroy(counter);
    }

    void recreateWith(const SolidSyslogMetaSdConfig* configPtr)
    {
        SolidSyslogMetaSd_Destroy(sd);
        sd = SolidSyslogMetaSd_Create(configPtr);
    }

    void recreate()
    {
        recreateWith(&config);
    }

    void useSysUpTime(uint32_t value)
    {
        fakeSysUpTimeValue = value;
        config.GetSysUpTime = FakeSysUpTime_Get;
        recreate();
    }

    void useLanguage(const char* tag)
    {
        fakeLanguageContent = tag;
        fakeLanguageMaxLength = strlen(tag);
        config.GetLanguage = FakeLanguage_Get;
        recreate();
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

TEST(SolidSyslogMetaSd, CreateReturnsNonNull)
{
    CHECK(sd != nullptr);
}

TEST(SolidSyslogMetaSd, FirstFormatProducesSequenceId1)
{
    format();
    CHECK_SEQUENCEID("1");
}

TEST(SolidSyslogMetaSd, SecondFormatProducesSequenceId2)
{
    format();
    resetFormatter();
    format();
    CHECK_SEQUENCEID("2");
}

TEST(SolidSyslogMetaSd, ThirdFormatProducesSequenceId3)
{
    format();
    format();
    resetFormatter();
    format();
    CHECK_SEQUENCEID("3");
}

TEST(SolidSyslogMetaSd, FormatAdvancesFormatterLength)
{
    LONGS_EQUAL(0, SolidSyslogFormatter_Length(formatter));
    format();
    CHECK(SolidSyslogFormatter_Length(formatter) > 0);
    LONGS_EQUAL(strlen(SolidSyslogFormatter_AsFormattedBuffer(formatter)), SolidSyslogFormatter_Length(formatter));
}

TEST(SolidSyslogMetaSd, DestroyDoesNotCrash)
{
    // Covered by teardown — this test documents the intent
}

TEST(SolidSyslogMetaSd, UseAfterDestroyIsCrashSafeViaNullSdVtable)
{
    /* After Destroy the slot's abstract-base vtable is the shared NullSd's, so
     * Format through the stale handle is a safe no-op rather than a NULL-fn-pointer
     * crash. NullSd.Format writes nothing into the Formatter. */
    SolidSyslogMetaSd_Destroy(sd);
    SolidSyslogStructuredData_Format(sd, formatter);
    LONGS_EQUAL(0, SolidSyslogFormatter_Length(formatter));
    sd = SolidSyslogMetaSd_Create(&config); // for teardown
}

TEST(SolidSyslogMetaSd, FormatIncludesSysUpTimeFromCallback)
{
    useSysUpTime(12345);
    format();
    CHECK_SYSUPTIME("12345");
}

TEST(SolidSyslogMetaSd, FormatIncludesDifferentSysUpTimeFromCallback)
{
    useSysUpTime(99999);
    format();
    CHECK_SYSUPTIME("99999");
}

TEST(SolidSyslogMetaSd, FormatIncludesSysUpTimeAtZero)
{
    useSysUpTime(0);
    format();
    CHECK_SYSUPTIME("0");
}

TEST(SolidSyslogMetaSd, FormatIncludesSysUpTimeAtMaxUint32)
{
    useSysUpTime(UINT32_MAX);
    format();
    CHECK_SYSUPTIME("4294967295");
}

TEST(SolidSyslogMetaSd, FormatIncludesLanguageFromCallback)
{
    useLanguage("en-GB");
    format();
    CHECK_LANGUAGE("en-GB");
}

TEST(SolidSyslogMetaSd, FormatIncludesDifferentLanguageFromCallback)
{
    useLanguage("fr");
    format();
    CHECK_LANGUAGE("fr");
}

TEST(SolidSyslogMetaSd, FormatEscapesQuoteInLanguage)
{
    useLanguage("a\"b");
    format();
    CHECK_LANGUAGE("a\\\"b");
}

TEST(SolidSyslogMetaSd, FormatEscapesBackslashInLanguage)
{
    useLanguage("a\\b");
    format();
    CHECK_LANGUAGE("a\\\\b");
}

TEST(SolidSyslogMetaSd, FormatEscapesBracketInLanguage)
{
    useLanguage("a]b");
    format();
    CHECK_LANGUAGE("a\\]b");
}

TEST(SolidSyslogMetaSd, FormatWithAllThreeParamsEmitsAllThree)
{
    useSysUpTime(12345);
    useLanguage("en-GB");
    format();
    STRCMP_EQUAL(
        "[meta sequenceId=\"1\" sysUpTime=\"12345\" language=\"en-GB\"]",
        SolidSyslogFormatter_AsFormattedBuffer(formatter)
    );
}

TEST(SolidSyslogMetaSd, FormatEmitsNothingWhenConfigCounterIsNullEvenIfOtherFieldsPresent)
{
    config.Counter = nullptr;
    useSysUpTime(12345);
    useLanguage("en-GB");
    format();
    STRCMP_EQUAL("", SolidSyslogFormatter_AsFormattedBuffer(formatter));
}

TEST(SolidSyslogMetaSd, FormatEmitsNothingWhenCreatedWithNullConfig)
{
    recreateWith(nullptr);
    format();
    STRCMP_EQUAL("", SolidSyslogFormatter_AsFormattedBuffer(formatter));
}

TEST(SolidSyslogMetaSd, CreateWithNullConfigReportsWarning)
{
    recreateWith(nullptr);

    CALLED_FAKE(ErrorHandlerFake_Handle, ONCE);
    LONGS_EQUAL(SOLIDSYSLOG_SEVERITY_WARNING, ErrorHandlerFake_LastSeverity());
    POINTERS_EQUAL(&MetaSdErrorSource, ErrorHandlerFake_LastSource());
    UNSIGNED_LONGS_EQUAL(SOLIDSYSLOG_CAT_BAD_CONFIG, ErrorHandlerFake_LastCategory());
    UNSIGNED_LONGS_EQUAL(METASD_ERROR_NULL_CONFIG, ErrorHandlerFake_LastDetail());
}

TEST(SolidSyslogMetaSd, CreateWithNullCounterReportsWarning)
{
    config.Counter = nullptr;
    recreate();

    CALLED_FAKE(ErrorHandlerFake_Handle, ONCE);
    LONGS_EQUAL(SOLIDSYSLOG_SEVERITY_WARNING, ErrorHandlerFake_LastSeverity());
    POINTERS_EQUAL(&MetaSdErrorSource, ErrorHandlerFake_LastSource());
    UNSIGNED_LONGS_EQUAL(SOLIDSYSLOG_CAT_BAD_CONFIG, ErrorHandlerFake_LastCategory());
    UNSIGNED_LONGS_EQUAL(METASD_ERROR_NULL_COUNTER, ErrorHandlerFake_LastDetail());
}

// Pool tests — prove SOLIDSYSLOG_META_SD_POOL_SIZE caps live instances
// and overflow falls back to the shared SolidSyslogNullSd.

// clang-format off
TEST_GROUP(SolidSyslogMetaSdPool)
{
    SolidSyslogAtomicCounter* counter                                   = nullptr;
    SolidSyslogMetaSdConfig config{};
    struct SolidSyslogStructuredData* pooled[SOLIDSYSLOG_META_SD_POOL_SIZE] = {};
    struct SolidSyslogStructuredData* overflow                          = nullptr;

    void setup() override
    {
        counter = TestAtomicCounter_Create();
        config.Counter = counter;
    }

    void teardown() override
    {
        for (auto* handle : pooled)
        {
            if (handle != nullptr)
            {
                SolidSyslogMetaSd_Destroy(handle);
            }
        }
        if (overflow != nullptr)
        {
            SolidSyslogMetaSd_Destroy(overflow);
        }
        TestAtomicCounter_Destroy(counter);
    }

    struct SolidSyslogStructuredData* MakeSd()
    {
        return SolidSyslogMetaSd_Create(&config);
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

TEST(SolidSyslogMetaSdPool, OverflowReportsPoolExhausted)
{
    FillPool();
    ErrorHandlerFake_Install(nullptr);

    overflow = MakeSd();

    CALLED_FAKE(ErrorHandlerFake_Handle, ONCE);
    LONGS_EQUAL(SOLIDSYSLOG_SEVERITY_CRITICAL, ErrorHandlerFake_LastSeverity());
    POINTERS_EQUAL(&MetaSdErrorSource, ErrorHandlerFake_LastSource());
    UNSIGNED_LONGS_EQUAL(SOLIDSYSLOG_CAT_POOL_EXHAUSTED, ErrorHandlerFake_LastCategory());
    UNSIGNED_LONGS_EQUAL(METASD_ERROR_POOL_EXHAUSTED, ErrorHandlerFake_LastDetail());
}

TEST(SolidSyslogMetaSdPool, FillingPoolThenOverflowReturnsDistinctFallback)
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
