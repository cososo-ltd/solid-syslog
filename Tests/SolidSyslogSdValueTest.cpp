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
    struct SolidSyslogSdValue value{};

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
    void closeValue() { SolidSyslogSdValue_Close(&value); }
};

// clang-format on

TEST(SolidSyslogSdValue, StringWithEmptySourceWritesNothing)
{
    writeString("");

    CHECK_VALUE("");
}

TEST(SolidSyslogSdValue, StringPassesPlainAsciiThrough)
{
    writeString("hello");

    CHECK_VALUE("hello");
}

TEST(SolidSyslogSdValue, StringCannotOverflowTheFormatterBuffer)
{
    /* A value longer than the message buffer is truncated at the buffer edge,
     * never overflows. A four-byte formatter holds three chars + terminator. */
    SolidSyslogFormatterStorage tinyStorage[SOLIDSYSLOG_FORMATTER_STORAGE_SIZE(4)];
    struct SolidSyslogFormatter* tiny = SolidSyslogFormatter_Create(tinyStorage, 4);
    struct SolidSyslogSdValue tinyValue{};
    SolidSyslogSdValue_FromFormatter(&tinyValue, tiny);

    SolidSyslogSdValue_String(&tinyValue, "hello");

    STRCMP_EQUAL("hel", SolidSyslogFormatter_AsFormattedBuffer(tiny));
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

TEST(SolidSyslogSdValue, ReassemblesTwoByteCodepointSplitAcrossStringCalls)
{
    /* U+00A9 COPYRIGHT SIGN streamed as its lead byte then its continuation
     * byte across two _String calls — the value must reassemble the codepoint,
     * not emit a U+FFFD per orphaned half. */
    writeString("\xC2");
    writeString("\xA9");

    CHECK_VALUE("\xC2\xA9");
}

TEST(SolidSyslogSdValue, ReassemblesFourByteCodepointSplitAcrossStringCalls)
{
    /* U+1F600 GRINNING FACE (F0 9F 98 80) split lead+1 / +2 continuations. */
    writeString("\xF0\x9F");
    writeString("\x98\x80");

    CHECK_VALUE("\xF0\x9F\x98\x80");
}

TEST(SolidSyslogSdValue, ReassemblesFourByteCodepointSplitOneByteAtATime)
{
    /* The same codepoint dribbled a byte at a time keeps the partial sequence
     * held across calls that add no completing byte. */
    writeString("\xF0");
    writeString("\x9F");
    writeString("\x98");
    writeString("\x80");

    CHECK_VALUE("\xF0\x9F\x98\x80");
}

TEST(SolidSyslogSdValue, EmitsCompletePrefixAndHoldsTrailingIncompleteSequence)
{
    /* Plain text then a dangling lead byte in one chunk: the prefix is emitted
     * immediately, the lead byte waits for the next chunk to complete it. */
    writeString("hi\xC2");
    writeString("\xA9");

    CHECK_VALUE("hi\xC2\xA9");
}

TEST(SolidSyslogSdValue, SubstitutesInChunkLeadByteInterruptedByAscii)
{
    /* A lead byte followed immediately by ASCII within one chunk is ill-formed
     * and substituted per-byte; the ASCII passes through. */
    writeString("\xC2Z");

    CHECK_VALUE("\xEF\xBF\xBDZ");
}

TEST(SolidSyslogSdValue, SubstitutesHeldSequenceInterruptedByNextChunk)
{
    /* A held lead byte that the next chunk fails to continue is replaced with a
     * single U+FFFD, then the interrupting byte is processed normally. */
    writeString("\xE0");
    writeString("Z");

    CHECK_VALUE("\xEF\xBF\xBDZ");
}

TEST(SolidSyslogSdValue, CloseSubstitutesStillIncompleteTailWithReplacementCharacter)
{
    writeString("\xC2");
    closeValue();

    CHECK_VALUE("\xEF\xBF\xBD");
}

TEST(SolidSyslogSdValue, CloseWithNoHeldTailWritesNothing)
{
    writeString("hi");
    closeValue();

    CHECK_VALUE("hi");
}

TEST(SolidSyslogSdValue, Uint32FlushesHeldTailBeforeDigits)
{
    /* A held incomplete _String tail must resolve to U+FFFD before the digits,
     * not be silently reordered after them (or dropped). */
    writeString("\xC2");
    writeUint32(5);

    CHECK_VALUE("\xEF\xBF\xBD"
                "5");
}

TEST(SolidSyslogSdValue, BoundedStringFlushesHeldTailBeforeValue)
{
    writeString("\xC2");
    writeBoundedString("hi", 8);

    CHECK_VALUE("\xEF\xBF\xBDhi");
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
