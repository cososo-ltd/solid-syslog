#include <cstring>
#include <stdint.h>

#include "CppUTest/TestHarness.h"
#include "SolidSyslogFormatter.h"
#include "SolidSyslogSdValue.h"
#include "SolidSyslogSdValuePrivate.h"

enum
{
    TEST_BUFFER_SIZE = 64
};

#define CHECK_VALUE(expected) STRCMP_EQUAL(expected, SolidSyslogFormatter_AsFormattedBuffer(formatter))

// clang-format off
TEST_GROUP(SolidSyslogSdValue)
{
    SolidSyslogFormatterStorage storage[SOLIDSYSLOG_FORMATTER_STORAGE_SIZE(TEST_BUFFER_SIZE)];
    struct SolidSyslogFormatter* formatter = nullptr;
    struct SolidSyslogSdValue value;

    void setup() override
    {
        formatter = SolidSyslogFormatter_Create(storage, TEST_BUFFER_SIZE);
        SolidSyslogSdValue_FromFormatter(&value, formatter);
    }

    void writeString(const char* source) { SolidSyslogSdValue_String(&value, source); }
    void writeBoundedString(const char* source, size_t maxDecodedLength)
    {
        SolidSyslogSdValue_BoundedString(&value, source, maxDecodedLength);
    }
    void writeUint32(uint32_t number) { SolidSyslogSdValue_Uint32(&value, number); }
};

// clang-format on

TEST(SolidSyslogSdValue, StringPassesPlainAsciiThrough)
{
    writeString("hello");

    CHECK_VALUE("hello");
}

TEST(SolidSyslogSdValue, StringEscapesDoubleQuote)
{
    writeString("a\"b");

    CHECK_VALUE("a\\\"b");
}

TEST(SolidSyslogSdValue, StringEscapesBackslash)
{
    writeString("a\\b");

    CHECK_VALUE("a\\\\b");
}

TEST(SolidSyslogSdValue, StringEscapesCloseBracket)
{
    writeString("a]b");

    CHECK_VALUE("a\\]b");
}

TEST(SolidSyslogSdValue, StringEscapesAllThreeSpecialsInOneValue)
{
    writeString("\"\\]");

    CHECK_VALUE("\\\"\\\\\\]");
}

TEST(SolidSyslogSdValue, StringPassesValidUtf8CodepointThrough)
{
    /* U+00A9 COPYRIGHT SIGN, a valid two-byte sequence — passes byte-for-byte. */
    writeString("\xC2\xA9");

    CHECK_VALUE("\xC2\xA9");
}

TEST(SolidSyslogSdValue, StringSubstitutesIllFormedByteWithReplacementCharacter)
{
    /* A lone continuation byte is ill-formed UTF-8 — substituted with U+FFFD. */
    writeString("\x80");

    CHECK_VALUE("\xEF\xBF\xBD");
}

TEST(SolidSyslogSdValue, Uint32EmitsDecimalDigits)
{
    writeUint32(42);

    CHECK_VALUE("42");
}

TEST(SolidSyslogSdValue, BoundedStringPassesValueShorterThanCapThrough)
{
    writeBoundedString("hi", 8);

    CHECK_VALUE("hi");
}

TEST(SolidSyslogSdValue, BoundedStringTruncatesAtMaxDecodedLength)
{
    writeBoundedString("hello", 3);

    CHECK_VALUE("hel");
}

TEST(SolidSyslogSdValue, BoundedStringCapCountsEscapePairAsOneDecodedByte)
{
    /* The cap bounds the decoded length a receiver un-escapes, not the on-wire
     * bytes: 'a' (1) + '"' -> \" (1 decoded) reaches the cap of 2, so 'b' is
     * dropped even though four bytes were written. */
    writeBoundedString("a\"b", 2);

    CHECK_VALUE("a\\\"");
}
