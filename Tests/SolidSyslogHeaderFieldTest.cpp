#include <stdint.h>

#include "CppUTest/TestHarness.h"
#include "SolidSyslogFormatter.h"
#include "SolidSyslogHeaderField.h"
#include "SolidSyslogHeaderFieldPrivate.h"

enum
{
    TEST_BUFFER_SIZE = 64
};

#define CHECK_FIELD(expected) STRCMP_EQUAL(expected, SolidSyslogFormatter_AsFormattedBuffer(formatter))

// clang-format off
TEST_GROUP(SolidSyslogHeaderField)
{
    SolidSyslogFormatterStorage storage[SOLIDSYSLOG_FORMATTER_STORAGE_SIZE(TEST_BUFFER_SIZE)];
    struct SolidSyslogFormatter* formatter = nullptr;
    struct SolidSyslogHeaderField field{};

    void setup() override
    {
        formatter = SolidSyslogFormatter_Create(storage, TEST_BUFFER_SIZE);
    }

    void fromFormatter(size_t maxLength)
    {
        SolidSyslogHeaderField_FromFormatter(&field, formatter, maxLength);
    }
};

// clang-format on

TEST(SolidSyslogHeaderField, PrintUsAsciiOfEmptyStringWritesNothing)
{
    fromFormatter(32);
    SolidSyslogHeaderField_PrintUsAscii(&field, "", 32);
    CHECK_FIELD("");
}

TEST(SolidSyslogHeaderField, PrintUsAsciiAppendsPrintableAscii)
{
    fromFormatter(32);
    SolidSyslogHeaderField_PrintUsAscii(&field, "host01", 32);
    CHECK_FIELD("host01");
}

TEST(SolidSyslogHeaderField, PrintUsAsciiSubstitutesControlCharacter)
{
    fromFormatter(32);
    SolidSyslogHeaderField_PrintUsAscii(&field, "a\tb", 32);
    CHECK_FIELD("a?b");
}

TEST(SolidSyslogHeaderField, PrintUsAsciiSubstitutesSpace)
{
    fromFormatter(32);
    SolidSyslogHeaderField_PrintUsAscii(&field, "a b", 32);
    CHECK_FIELD("a?b");
}

TEST(SolidSyslogHeaderField, PrintUsAsciiStopsAtNul)
{
    fromFormatter(32);
    SolidSyslogHeaderField_PrintUsAscii(&field, "ab\0cd", 5);
    CHECK_FIELD("ab");
}

TEST(SolidSyslogHeaderField, PrintUsAsciiTruncatesAtMaxLength)
{
    fromFormatter(32);
    SolidSyslogHeaderField_PrintUsAscii(&field, "abcdef", 3);
    CHECK_FIELD("abc");
}

TEST(SolidSyslogHeaderField, PrintUsAsciiTruncatesAtFieldWidth)
{
    fromFormatter(3);
    SolidSyslogHeaderField_PrintUsAscii(&field, "abcdef", 32);
    CHECK_FIELD("abc");
}

TEST(SolidSyslogHeaderField, PrintUsAsciiBudgetIsSharedAcrossCalls)
{
    fromFormatter(4);
    SolidSyslogHeaderField_PrintUsAscii(&field, "ab", 32);
    SolidSyslogHeaderField_PrintUsAscii(&field, "cdef", 32);
    CHECK_FIELD("abcd");
}

TEST(SolidSyslogHeaderField, Uint32AppendsDigits)
{
    fromFormatter(32);
    SolidSyslogHeaderField_Uint32(&field, 1234);
    CHECK_FIELD("1234");
}

TEST(SolidSyslogHeaderField, Uint32OfZeroAppendsZero)
{
    fromFormatter(32);
    SolidSyslogHeaderField_Uint32(&field, 0);
    CHECK_FIELD("0");
}
