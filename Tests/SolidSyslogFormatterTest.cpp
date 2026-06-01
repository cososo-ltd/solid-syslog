#include <stdint.h>
#include <cstring>

#include "SolidSyslogFormatter.h"
#include "CppUTest/TestHarness.h"

class TEST_SolidSyslogFormatter_AsFormattedBufferReturnsFormattedContent_Test;
class TEST_SolidSyslogFormatter_AsFormattedBufferTrimsFourByteLeadWithOnlyOneContinuation_Test;
class TEST_SolidSyslogFormatter_AsFormattedBufferTrimsFourByteLeadWithOnlyTwoContinuations_Test;
class TEST_SolidSyslogFormatter_AsFormattedBufferTrimsThreeByteLeadWithOnlyOneContinuation_Test;
class TEST_SolidSyslogFormatter_AsFormattedBufferTrimsTruncatedFourByteLeadAtBufferTail_Test;
class TEST_SolidSyslogFormatter_AsFormattedBufferTrimsTruncatedThreeByteLeadAtBufferTail_Test;
class TEST_SolidSyslogFormatter_AsFormattedBufferTrimsTruncatedTwoByteLeadAtBufferTail_Test;
class TEST_SolidSyslogFormatter_AsFormattedBufferZerosBothOrphanContinuationsWhenFourByteTrimmedAtAntepenultimate_Test;
class TEST_SolidSyslogFormatter_AsFormattedBufferZerosOrphanContinuationWhenFourByteTrimmedAtPenultimate_Test;
class TEST_SolidSyslogFormatter_AsFormattedBufferZerosOrphanContinuationWhenThreeByteTrimmedAtPenultimate_Test;
class TEST_SolidSyslogFormatter_AsciiCharacterAcceptsSpace_Test;
class TEST_SolidSyslogFormatter_AsciiCharacterStopsWhenFull_Test;
class TEST_SolidSyslogFormatter_AsciiCharacterSubstitutesControlCharWithQuestionMark_Test;
class TEST_SolidSyslogFormatter_AsciiCharacterSubstitutesDelWithQuestionMark_Test;
class TEST_SolidSyslogFormatter_AsciiCharacterSubstitutesHighBitByteWithQuestionMark_Test;
class TEST_SolidSyslogFormatter_AsciiCharacterWritesIntoBuffer_Test;
class TEST_SolidSyslogFormatter_BomWritesUtf8ByteOrderMark_Test;
class TEST_SolidSyslogFormatter_BoundedStringAppendsAfterAsciiCharacter_Test;
class TEST_SolidSyslogFormatter_BoundedStringFillsExactCapacity_Test;
class TEST_SolidSyslogFormatter_BoundedStringPassesFourByteCodepointWithF1LeadThrough_Test;
class TEST_SolidSyslogFormatter_BoundedStringPassesHighestValidTwoByteCodepointThrough_Test;
class TEST_SolidSyslogFormatter_BoundedStringPassesMiddleValidTwoByteCodepointThrough_Test;
class TEST_SolidSyslogFormatter_BoundedStringPassesSecondValidTwoByteCodepointThrough_Test;
class TEST_SolidSyslogFormatter_BoundedStringPassesThreeByteCodepointWithE1LeadThrough_Test;
class TEST_SolidSyslogFormatter_BoundedStringPassesValidFourByteCodepointThrough_Test;
class TEST_SolidSyslogFormatter_BoundedStringPassesValidThreeByteCodepointThrough_Test;
class TEST_SolidSyslogFormatter_BoundedStringPassesValidTwoByteCodepointThrough_Test;
class TEST_SolidSyslogFormatter_BoundedStringReplacementHiddenFromAsFormattedBufferWhenOutputTooSmall_Test;
class TEST_SolidSyslogFormatter_BoundedStringReplacesF5AsFourByteLead_Test;
class TEST_SolidSyslogFormatter_BoundedStringReplacesF6AsFourByteLead_Test;
class TEST_SolidSyslogFormatter_BoundedStringReplacesF7AsFourByteLead_Test;
class TEST_SolidSyslogFormatter_BoundedStringReplacesFourByteEncodingBeyondUnicodeRange_Test;
class TEST_SolidSyslogFormatter_BoundedStringReplacesInvalidLeadByteWithReplacementChar_Test;
class TEST_SolidSyslogFormatter_BoundedStringReplacesInvalidLeadsF5ToFF_Test;
class TEST_SolidSyslogFormatter_BoundedStringReplacesInvalidLeadsInF8ToFFMid_Test;
class TEST_SolidSyslogFormatter_BoundedStringReplacesOverlongFourByteEncodingPerByte_Test;
class TEST_SolidSyslogFormatter_BoundedStringReplacesOverlongLeadC0_Test;
class TEST_SolidSyslogFormatter_BoundedStringReplacesOverlongThreeByteEncodingAtSubrangeTop_Test;
class TEST_SolidSyslogFormatter_BoundedStringReplacesOverlongThreeByteEncodingPerByte_Test;
class TEST_SolidSyslogFormatter_BoundedStringReplacesOverlongTwoByteEncodingPerByte_Test;
class TEST_SolidSyslogFormatter_BoundedStringReplacesSmallestContinuationByte_Test;
class TEST_SolidSyslogFormatter_BoundedStringReplacesStragglingFourByteLeadWhenSourceTruncated_Test;
class TEST_SolidSyslogFormatter_BoundedStringReplacesStragglingThreeByteLeadWhenSourceTruncated_Test;
class TEST_SolidSyslogFormatter_BoundedStringReplacesStragglingTwoByteLeadWhenSourceTruncated_Test;
class TEST_SolidSyslogFormatter_BoundedStringReplacesThreeByteLeadFollowedByAscii_Test;
class TEST_SolidSyslogFormatter_BoundedStringReplacesTopOfThreeByteLeadRangeWhenInvalid_Test;
class TEST_SolidSyslogFormatter_BoundedStringReplacesUtf16SurrogateEncodingPerByte_Test;
class TEST_SolidSyslogFormatter_BoundedStringStopsAtBufferCapacity_Test;
class TEST_SolidSyslogFormatter_BoundedStringTruncatesAtMaxLength_Test;
class TEST_SolidSyslogFormatter_BoundedStringValidCodepointHiddenFromAsFormattedBufferWhenOutputTooSmall_Test;
class TEST_SolidSyslogFormatter_BoundedStringWritesNothingWhenFull_Test;
class TEST_SolidSyslogFormatter_BoundedStringWritesStringIntoBuffer_Test;
class TEST_SolidSyslogFormatter_EscapedStringBreaksWhenReplacementDoesNotFitDecodedBudget_Test;
class TEST_SolidSyslogFormatter_EscapedStringEscapesAllThreeSpecialsInOneValue_Test;
class TEST_SolidSyslogFormatter_EscapedStringEscapesBackslash_Test;
class TEST_SolidSyslogFormatter_EscapedStringEscapesCloseBracket_Test;
class TEST_SolidSyslogFormatter_EscapedStringEscapesDoubleQuote_Test;
class TEST_SolidSyslogFormatter_EscapedStringMaxDecodedLengthBoundsDecoderBufferNotEncodedOutput_Test;
class TEST_SolidSyslogFormatter_EscapedStringPassesOrdinaryCharacterThrough_Test;
class TEST_SolidSyslogFormatter_EscapedStringPassesValidUtf8CodepointsThroughAroundEscapedSpecial_Test;
class TEST_SolidSyslogFormatter_EscapedStringReplacementClaimsThreeBytesOfDecodedBudget_Test;
class TEST_SolidSyslogFormatter_EscapedStringReplacesInvalidUtf8ByteWithReplacementChar_Test;
class TEST_SolidSyslogFormatter_EscapedStringReplacesStragglingMultiByteLeadWhenSourceTruncated_Test;
class TEST_SolidSyslogFormatter_EscapedStringTruncatesAtMaxDecodedLength_Test;
class TEST_SolidSyslogFormatter_EscapedStringWithEmptyInputWritesNothing_Test;
class TEST_SolidSyslogFormatter_FourDigitBeyondMaxFormatsLeastSignificant_Test;
class TEST_SolidSyslogFormatter_FourDigitFormatsAllDigits_Test;
class TEST_SolidSyslogFormatter_FourDigitFormatsMax_Test;
class TEST_SolidSyslogFormatter_FourDigitFormatsZero_Test;
class TEST_SolidSyslogFormatter_LengthAdvancesWithWrites_Test;
class TEST_SolidSyslogFormatter_LengthStartsAtZero_Test;
class TEST_SolidSyslogFormatter_NilValueWritesHyphen_Test;
class TEST_SolidSyslogFormatter_OneByteBufferHoldsOnlyNullTerminator_Test;
class TEST_SolidSyslogFormatter_PrintUsAsciiStringPassesBangAndTildeBoundariesThrough_Test;
class TEST_SolidSyslogFormatter_PrintUsAsciiStringPassesPrintableCharacterThrough_Test;
class TEST_SolidSyslogFormatter_PrintUsAsciiStringSubstitutesControlCharacter_Test;
class TEST_SolidSyslogFormatter_PrintUsAsciiStringSubstitutesDel_Test;
class TEST_SolidSyslogFormatter_PrintUsAsciiStringSubstitutesHighBitByte_Test;
class TEST_SolidSyslogFormatter_PrintUsAsciiStringSubstitutesSpace_Test;
class TEST_SolidSyslogFormatter_PrintUsAsciiStringTruncatesAtMaxLength_Test;
class TEST_SolidSyslogFormatter_PrintUsAsciiStringTruncationBoundsSubstitution_Test;
class TEST_SolidSyslogFormatter_PrintUsAsciiStringWithEmptyInputWritesNothing_Test;
class TEST_SolidSyslogFormatter_SixDigitBeyondMaxFormatsLeastSignificant_Test;
class TEST_SolidSyslogFormatter_SixDigitFormatsAllDigits_Test;
class TEST_SolidSyslogFormatter_SixDigitFormatsMax_Test;
class TEST_SolidSyslogFormatter_SixDigitFormatsZero_Test;
class TEST_SolidSyslogFormatter_TwoAsciiCharactersAppendInSequence_Test;
class TEST_SolidSyslogFormatter_TwoDigitBeyondMaxFormatsLeastSignificant_Test;
class TEST_SolidSyslogFormatter_TwoDigitFormatsAllDigits_Test;
class TEST_SolidSyslogFormatter_TwoDigitFormatsMax_Test;
class TEST_SolidSyslogFormatter_TwoDigitFormatsZero_Test;
class TEST_SolidSyslogFormatter_Uint32AppendsAfterAsciiCharacter_Test;
class TEST_SolidSyslogFormatter_Uint32FitsExactly_Test;
class TEST_SolidSyslogFormatter_Uint32FormatsMultipleDigits_Test;
class TEST_SolidSyslogFormatter_Uint32FormatsSingleDigit_Test;
class TEST_SolidSyslogFormatter_Uint32FormatsZero_Test;
class TEST_SolidSyslogFormatter_Uint32TruncatedWhenBufferTooSmall_Test;
class TEST_SolidSyslogFormatter_ZeroSizeAsciiCharacterIsNoOp_Test;
class TEST_SolidSyslogFormatter_ZeroSizeBoundedStringIsNoOp_Test;
class TEST_SolidSyslogFormatter_ZeroSizeUint32IsNoOp_Test;
struct TEST_GROUP_CppUTestGroupSolidSyslogFormatter;

#define CREATE_FORMATTER(bufferSize) formatter = SolidSyslogFormatter_Create(storage, bufferSize)

#define CHECK_FORMATTED(expected)                                              \
    STRCMP_EQUAL(expected, SolidSyslogFormatter_AsFormattedBuffer(formatter)); \
    LONGS_EQUAL(strlen(expected), SolidSyslogFormatter_Length(formatter))

#define CHECK_LENGTH(expected) LONGS_EQUAL(expected, SolidSyslogFormatter_Length(formatter))

enum
{
    TEST_BUFFER_SIZE = 64
};

// clang-format off
TEST_GROUP(SolidSyslogFormatter)
{
    SolidSyslogFormatterStorage storage[SOLIDSYSLOG_FORMATTER_STORAGE_SIZE(TEST_BUFFER_SIZE)];
    struct SolidSyslogFormatter* formatter;

    void setup() override
    {
        CREATE_FORMATTER(TEST_BUFFER_SIZE);
    }

    void formatAsciiCharacter(char value) const { SolidSyslogFormatter_AsciiCharacter(formatter, value); }
    void formatBoundedString(const char* source, size_t maxLength) const { SolidSyslogFormatter_BoundedString(formatter, source, maxLength); }
    void formatUint32(uint32_t value) const { SolidSyslogFormatter_Uint32(formatter, value); }
    void formatTwoDigit(uint32_t value) const { SolidSyslogFormatter_TwoDigit(formatter, value); }
    void formatFourDigit(uint32_t value) const { SolidSyslogFormatter_FourDigit(formatter, value); }
    void formatSixDigit(uint32_t value) const { SolidSyslogFormatter_SixDigit(formatter, value); }
};

// clang-format on

TEST(SolidSyslogFormatter, AsciiCharacterWritesIntoBuffer)
{
    formatAsciiCharacter('A');

    CHECK_FORMATTED("A");
}

TEST(SolidSyslogFormatter, TwoAsciiCharactersAppendInSequence)
{
    formatAsciiCharacter('A');
    formatAsciiCharacter('B');

    CHECK_FORMATTED("AB");
}

TEST(SolidSyslogFormatter, AsciiCharacterSubstitutesHighBitByteWithQuestionMark)
{
    /* A high-bit byte (like a UTF-8 lead) is not PRINTUSASCII; it must be
     * replaced with the substitute '?'. This keeps AsciiCharacter safe to
     * hand to extension points — no way to smuggle non-ASCII in. */
    formatAsciiCharacter('\xC3');

    CHECK_FORMATTED("?");
}

TEST(SolidSyslogFormatter, AsciiCharacterSubstitutesControlCharWithQuestionMark)
{
    /* C0 control characters (0x01-0x1F) are not acceptable in the header
     * fields; AsciiCharacter must substitute them. */
    formatAsciiCharacter('\x01');

    CHECK_FORMATTED("?");
}

TEST(SolidSyslogFormatter, AsciiCharacterSubstitutesDelWithQuestionMark)
{
    /* DEL (0x7F) sits between the printable range and the high-bit
     * range; it is still non-printable and must substitute. */
    formatAsciiCharacter('\x7F');

    CHECK_FORMATTED("?");
}

TEST(SolidSyslogFormatter, AsciiCharacterAcceptsSpace)
{
    /* Space (0x20) is outside PRINTUSASCII but must pass through — the
     * library uses it as the structural separator between syslog header
     * fields. */
    formatAsciiCharacter(' ');

    CHECK_FORMATTED(" ");
}

TEST(SolidSyslogFormatter, BoundedStringWritesStringIntoBuffer)
{
    formatBoundedString("hello", 10);

    CHECK_FORMATTED("hello");
}

TEST(SolidSyslogFormatter, BoundedStringReplacesInvalidLeadByteWithReplacementChar)
{
    formatBoundedString("\x85", 1);

    CHECK_FORMATTED("\xEF\xBF\xBD");
}

TEST(SolidSyslogFormatter, BoundedStringReplacesSmallestContinuationByte)
{
    formatBoundedString("\x80", 1);

    CHECK_FORMATTED("\xEF\xBF\xBD");
}

TEST(SolidSyslogFormatter, BoundedStringReplacesOverlongTwoByteEncodingPerByte)
{
    /* \xC1\x81 — overlong 2-byte form of U+0041. Per RFC 3629 §10 and Unicode
     * §3.9, each invalid byte becomes its own U+FFFD substitution. */
    formatBoundedString("\xC1\x81", 2);

    CHECK_FORMATTED("\xEF\xBF\xBD\xEF\xBF\xBD");
}

TEST(SolidSyslogFormatter, BoundedStringReplacesOverlongLeadC0)
{
    /* \xC0 — the other overlong 2-byte lead forbidden by RFC 3629 §4. */
    formatBoundedString("\xC0", 1);

    CHECK_FORMATTED("\xEF\xBF\xBD");
}

TEST(SolidSyslogFormatter, BoundedStringReplacesInvalidLeadsF5ToFF)
{
    /* F5-F7: would encode codepoint > U+10FFFF (outside Unicode range).
     * F8-FF: 5+ byte prefix patterns, removed by RFC 3629.
     * \xF5 and \xFF exercise both ends of the range. */
    formatBoundedString("\xF5\xFF", 2);

    CHECK_FORMATTED("\xEF\xBF\xBD\xEF\xBF\xBD");
}

TEST(SolidSyslogFormatter, BoundedStringReplacesInvalidLeadsInF8ToFFMid)
{
    /* Interior of the 5+ byte prefix range — drives a mask that covers F8-FF,
     * rather than enumerating each value. */
    formatBoundedString("\xF8\xFE", 2);

    CHECK_FORMATTED("\xEF\xBF\xBD\xEF\xBF\xBD");
}

TEST(SolidSyslogFormatter, BoundedStringPassesValidTwoByteCodepointThrough)
{
    /* \xC2\x80 is the canonical 2-byte encoding of U+0080. Valid UTF-8. */
    formatBoundedString("\xC2\x80", 2);

    CHECK_FORMATTED("\xC2\x80");
}

TEST(SolidSyslogFormatter, BoundedStringPassesSecondValidTwoByteCodepointThrough)
{
    /* \xC3\xA9 is the canonical 2-byte encoding of U+00E9 (é). Valid UTF-8. */
    formatBoundedString("\xC3\xA9", 2);

    CHECK_FORMATTED("\xC3\xA9");
}

TEST(SolidSyslogFormatter, BoundedStringPassesHighestValidTwoByteCodepointThrough)
{
    /* \xDF\xBF is the canonical 2-byte encoding of U+07FF, the top of the 2-byte range. */
    formatBoundedString("\xDF\xBF", 2);

    CHECK_FORMATTED("\xDF\xBF");
}

TEST(SolidSyslogFormatter, BoundedStringPassesMiddleValidTwoByteCodepointThrough)
{
    /* \xCC\x80 is a valid 2-byte codepoint mid-range (U+0300, combining grave accent). */
    formatBoundedString("\xCC\x80", 2);

    CHECK_FORMATTED("\xCC\x80");
}

TEST(SolidSyslogFormatter, BoundedStringPassesValidThreeByteCodepointThrough)
{
    /* \xE0\xA0\x80 is the canonical 3-byte encoding of U+0800, the smallest
     * non-overlong 3-byte codepoint. Valid UTF-8. */
    formatBoundedString("\xE0\xA0\x80", 3);

    CHECK_FORMATTED("\xE0\xA0\x80");
}

TEST(SolidSyslogFormatter, BoundedStringPassesThreeByteCodepointWithE1LeadThrough)
{
    /* \xE1\x80\x80 is the canonical 3-byte encoding of U+1000. Forces the
     * lead byte check to generalise past \xE0. */
    formatBoundedString("\xE1\x80\x80", 3);

    CHECK_FORMATTED("\xE1\x80\x80");
}

TEST(SolidSyslogFormatter, BoundedStringReplacesOverlongThreeByteEncodingPerByte)
{
    /* \xE0\x80\x80 — overlong 3-byte encoding of U+0000. The E0 lead
     * requires a continuation in A0-BF; \x80 is below that subrange, so the
     * sequence is ill-formed and each invalid byte becomes its own U+FFFD. */
    formatBoundedString("\xE0\x80\x80", 3);

    CHECK_FORMATTED("\xEF\xBF\xBD\xEF\xBF\xBD\xEF\xBF\xBD");
}

TEST(SolidSyslogFormatter, BoundedStringReplacesOverlongThreeByteEncodingAtSubrangeTop)
{
    /* \xE0\x9F\x80 — also overlong: \x9F is still below the E0 subrange
     * lower bound of \xA0. Forces the exclusion to widen beyond a single
     * hardcoded continuation byte to the full 80-9F range. */
    formatBoundedString("\xE0\x9F\x80", 3);

    CHECK_FORMATTED("\xEF\xBF\xBD\xEF\xBF\xBD\xEF\xBF\xBD");
}

TEST(SolidSyslogFormatter, BoundedStringReplacesUtf16SurrogateEncodingPerByte)
{
    /* \xED\xA0\x80 — UTF-8 encoding of U+D800, a UTF-16 high surrogate.
     * RFC 3629 §3 forbids encoding surrogates; the ED lead requires a
     * continuation in 80-9F, not A0-BF. */
    formatBoundedString("\xED\xA0\x80", 3);

    CHECK_FORMATTED("\xEF\xBF\xBD\xEF\xBF\xBD\xEF\xBF\xBD");
}

TEST(SolidSyslogFormatter, BoundedStringReplacesThreeByteLeadFollowedByAscii)
{
    /* \xE1 is a 3-byte lead; the next byte \x40 ('@') is not a valid
     * continuation (continuations must be 80-BF). Per maximal-subpart rule
     * the lead alone becomes U+FFFD; the ASCII byte passes through; the
     * stray \x80 is another U+FFFD. */
    formatBoundedString("\xE1\x40\x80", 3);

    CHECK_FORMATTED("\xEF\xBF\xBD\x40\xEF\xBF\xBD");
}

TEST(SolidSyslogFormatter, BoundedStringReplacesTopOfThreeByteLeadRangeWhenInvalid)
{
    /* \xEF is the top of the 3-byte lead range (E0-EF). Here it isn't
     * followed by a valid continuation, so the lead alone becomes U+FFFD. */
    formatBoundedString("\xEF\x40\x80", 3);

    CHECK_FORMATTED("\xEF\xBF\xBD\x40\xEF\xBF\xBD");
}

TEST(SolidSyslogFormatter, BoundedStringPassesValidFourByteCodepointThrough)
{
    /* \xF0\x90\x80\x80 is the canonical 4-byte encoding of U+10000, the
     * smallest non-overlong 4-byte codepoint. Valid UTF-8. */
    formatBoundedString("\xF0\x90\x80\x80", 4);

    CHECK_FORMATTED("\xF0\x90\x80\x80");
}

TEST(SolidSyslogFormatter, BoundedStringPassesFourByteCodepointWithF1LeadThrough)
{
    /* \xF1\x80\x80\x80 is the canonical 4-byte encoding of U+40000. Forces
     * the 4-byte lead check to generalise past \xF0. */
    formatBoundedString("\xF1\x80\x80\x80", 4);

    CHECK_FORMATTED("\xF1\x80\x80\x80");
}

TEST(SolidSyslogFormatter, BoundedStringReplacesOverlongFourByteEncodingPerByte)
{
    /* \xF0\x80\x80\x80 — overlong 4-byte encoding of U+0000. The F0 lead
     * requires a continuation in 90-BF; \x80 is below that subrange. */
    formatBoundedString("\xF0\x80\x80\x80", 4);

    CHECK_FORMATTED("\xEF\xBF\xBD\xEF\xBF\xBD\xEF\xBF\xBD\xEF\xBF\xBD");
}

TEST(SolidSyslogFormatter, BoundedStringReplacesFourByteEncodingBeyondUnicodeRange)
{
    /* \xF4\x90\x80\x80 encodes a codepoint beyond U+10FFFF (the top of the
     * Unicode range). The F4 lead requires cont1 in 80-8F; \x90 is above
     * that subrange. */
    formatBoundedString("\xF4\x90\x80\x80", 4);

    CHECK_FORMATTED("\xEF\xBF\xBD\xEF\xBF\xBD\xEF\xBF\xBD\xEF\xBF\xBD");
}

TEST(SolidSyslogFormatter, BoundedStringReplacesF5AsFourByteLead)
{
    /* \xF5 is not a valid 4-byte lead — any codepoint with a F5 lead would
     * exceed U+10FFFF. RFC 3629 §3 restricts 4-byte leads to F0-F4. */
    formatBoundedString("\xF5\x80\x80\x80", 4);

    CHECK_FORMATTED("\xEF\xBF\xBD\xEF\xBF\xBD\xEF\xBF\xBD\xEF\xBF\xBD");
}

TEST(SolidSyslogFormatter, BoundedStringReplacesF6AsFourByteLead)
{
    /* \xF6 is not a valid 4-byte lead either — same reason as F5. */
    formatBoundedString("\xF6\x80\x80\x80", 4);

    CHECK_FORMATTED("\xEF\xBF\xBD\xEF\xBF\xBD\xEF\xBF\xBD\xEF\xBF\xBD");
}

TEST(SolidSyslogFormatter, BoundedStringReplacesF7AsFourByteLead)
{
    /* \xF7 is the top of the would-be 4-byte lead range but is still
     * outside the valid F0-F4 range. */
    formatBoundedString("\xF7\x80\x80\x80", 4);

    CHECK_FORMATTED("\xEF\xBF\xBD\xEF\xBF\xBD\xEF\xBF\xBD\xEF\xBF\xBD");
}

TEST(SolidSyslogFormatter, BoundedStringTruncatesAtMaxLength)
{
    formatBoundedString("hello", 3);

    CHECK_FORMATTED("hel");
}

TEST(SolidSyslogFormatter, BoundedStringReplacesStragglingTwoByteLeadWhenSourceTruncated)
{
    /* maxLength caps source at 1 byte, but \xC2 followed by \x80 in memory
     * would be a valid 2-byte codepoint. Per Unicode §3.9, a lead that
     * can't complete a codepoint within the input bound becomes a single
     * U+FFFD; the continuation beyond the cap is never consumed. */
    formatBoundedString("\xC2\x80", 1);

    CHECK_FORMATTED("\xEF\xBF\xBD");
}

TEST(SolidSyslogFormatter, BoundedStringReplacesStragglingThreeByteLeadWhenSourceTruncated)
{
    /* maxLength caps source at 1 byte of an otherwise-valid 3-byte
     * sequence. The same truncation rule as the 2-byte case applies. */
    formatBoundedString("\xE0\xA0\x80", 1);

    CHECK_FORMATTED("\xEF\xBF\xBD");
}

TEST(SolidSyslogFormatter, BoundedStringReplacesStragglingFourByteLeadWhenSourceTruncated)
{
    /* maxLength caps source at 1 byte of an otherwise-valid 4-byte
     * sequence. The same truncation rule applies. */
    formatBoundedString("\xF0\x90\x80\x80", 1);

    CHECK_FORMATTED("\xEF\xBF\xBD");
}

TEST(SolidSyslogFormatter, BoundedStringAppendsAfterAsciiCharacter)
{
    formatAsciiCharacter('<');
    formatBoundedString("hello", 10);

    CHECK_FORMATTED("<hello");
}

TEST(SolidSyslogFormatter, Uint32FormatsSingleDigit)
{
    formatUint32(7);

    CHECK_FORMATTED("7");
}

TEST(SolidSyslogFormatter, Uint32FormatsZero)
{
    formatUint32(0);

    CHECK_FORMATTED("0");
}

TEST(SolidSyslogFormatter, Uint32FormatsMultipleDigits)
{
    formatUint32(134);

    CHECK_FORMATTED("134");
}

TEST(SolidSyslogFormatter, Uint32AppendsAfterAsciiCharacter)
{
    formatAsciiCharacter('<');
    formatUint32(42);

    CHECK_FORMATTED("<42");
}

TEST(SolidSyslogFormatter, LengthStartsAtZero)
{
    CHECK_LENGTH(0);
}

TEST(SolidSyslogFormatter, LengthAdvancesWithWrites)
{
    formatBoundedString("hello", 10);

    CHECK_LENGTH(5);
}

TEST(SolidSyslogFormatter, AsFormattedBufferReturnsFormattedContent)
{
    formatBoundedString("test", 4);

    CHECK_FORMATTED("test");
}

TEST(SolidSyslogFormatter, AsFormattedBufferTrimsTruncatedTwoByteLeadAtBufferTail)
{
    /* A 2-byte output buffer (1 usable byte) fills after the lead of a valid
     * 2-byte codepoint; WriteChar clamps the continuation. AsFormattedBuffer
     * must mask the stray lead with a NUL so callers see valid UTF-8.
     * Length still reports the raw byte count so the gap records the trim. */
    CREATE_FORMATTER(2);

    formatBoundedString("\xC2\x80", 2);

    STRCMP_EQUAL("", SolidSyslogFormatter_AsFormattedBuffer(formatter));
    LONGS_EQUAL(1, SolidSyslogFormatter_Length(formatter));
}

TEST(SolidSyslogFormatter, AsFormattedBufferTrimsTruncatedThreeByteLeadAtBufferTail)
{
    /* Same pattern with a 3-byte codepoint: the lead slips in, the two
     * continuations clamp. Trim removes the stray lead. */
    CREATE_FORMATTER(2);

    formatBoundedString("\xE0\xA0\x80", 3);

    STRCMP_EQUAL("", SolidSyslogFormatter_AsFormattedBuffer(formatter));
    LONGS_EQUAL(1, SolidSyslogFormatter_Length(formatter));
}

TEST(SolidSyslogFormatter, AsFormattedBufferTrimsTruncatedFourByteLeadAtBufferTail)
{
    /* Same pattern with a 4-byte codepoint. */
    CREATE_FORMATTER(2);

    formatBoundedString("\xF0\x90\x80\x80", 4);

    STRCMP_EQUAL("", SolidSyslogFormatter_AsFormattedBuffer(formatter));
    LONGS_EQUAL(1, SolidSyslogFormatter_Length(formatter));
}

TEST(SolidSyslogFormatter, AsFormattedBufferTrimsThreeByteLeadWithOnlyOneContinuation)
{
    /* A 3-byte buffer (2 usable bytes) lets the 3-byte codepoint's lead
     * and first continuation through before clamping the last byte.
     * Trim inspects position-2 and removes the truncated pair. */
    CREATE_FORMATTER(3);

    formatBoundedString("\xE0\xA0\x80", 3);

    STRCMP_EQUAL("", SolidSyslogFormatter_AsFormattedBuffer(formatter));
    LONGS_EQUAL(2, SolidSyslogFormatter_Length(formatter));
}

TEST(SolidSyslogFormatter, AsFormattedBufferTrimsFourByteLeadWithOnlyOneContinuation)
{
    /* Same buffer size with a 4-byte codepoint: two of four bytes get in,
     * trim inspects position-2 to find the stray 4-byte lead. */
    CREATE_FORMATTER(3);

    formatBoundedString("\xF0\x90\x80\x80", 4);

    STRCMP_EQUAL("", SolidSyslogFormatter_AsFormattedBuffer(formatter));
    LONGS_EQUAL(2, SolidSyslogFormatter_Length(formatter));
}

TEST(SolidSyslogFormatter, AsFormattedBufferTrimsFourByteLeadWithOnlyTwoContinuations)
{
    /* A 4-byte buffer (3 usable bytes) lets three of the four bytes of a
     * 4-byte codepoint through before clamping. Trim inspects position-3
     * to find the stray lead. */
    CREATE_FORMATTER(4);

    formatBoundedString("\xF0\x90\x80\x80", 4);

    STRCMP_EQUAL("", SolidSyslogFormatter_AsFormattedBuffer(formatter));
    LONGS_EQUAL(3, SolidSyslogFormatter_Length(formatter));
}

TEST(SolidSyslogFormatter, AsFormattedBufferZerosOrphanContinuationWhenThreeByteTrimmedAtPenultimate)
{
    /* Trim masks the 3-byte lead with NUL; the orphan continuation that
     * followed must also be zeroed so callers forwarding (buffer, Length)
     * to a byte sink never observe stray UTF-8 past the mask. */
    CREATE_FORMATTER(3);

    formatBoundedString("\xE0\xA0\x80", 3);

    const char* buffer = SolidSyslogFormatter_AsFormattedBuffer(formatter);
    LONGS_EQUAL(2, SolidSyslogFormatter_Length(formatter));
    BYTES_EQUAL('\0', buffer[0]);
    BYTES_EQUAL('\0', buffer[1]);
}

TEST(SolidSyslogFormatter, AsFormattedBufferZerosOrphanContinuationWhenFourByteTrimmedAtPenultimate)
{
    /* Same rule with a 4-byte codepoint partially clamped to 2 bytes. */
    CREATE_FORMATTER(3);

    formatBoundedString("\xF0\x90\x80\x80", 4);

    const char* buffer = SolidSyslogFormatter_AsFormattedBuffer(formatter);
    LONGS_EQUAL(2, SolidSyslogFormatter_Length(formatter));
    BYTES_EQUAL('\0', buffer[0]);
    BYTES_EQUAL('\0', buffer[1]);
}

TEST(SolidSyslogFormatter, AsFormattedBufferZerosBothOrphanContinuationsWhenFourByteTrimmedAtAntepenultimate)
{
    /* 4-byte codepoint clamped after 3 bytes — both trailing continuations
     * must be zeroed alongside the masked lead. */
    CREATE_FORMATTER(4);

    formatBoundedString("\xF0\x90\x80\x80", 4);

    const char* buffer = SolidSyslogFormatter_AsFormattedBuffer(formatter);
    LONGS_EQUAL(3, SolidSyslogFormatter_Length(formatter));
    BYTES_EQUAL('\0', buffer[0]);
    BYTES_EQUAL('\0', buffer[1]);
    BYTES_EQUAL('\0', buffer[2]);
}

TEST(SolidSyslogFormatter, ZeroSizeAsciiCharacterIsNoOp)
{
    CREATE_FORMATTER(0);

    formatAsciiCharacter('A');

    CHECK_LENGTH(0);
}

TEST(SolidSyslogFormatter, ZeroSizeBoundedStringIsNoOp)
{
    CREATE_FORMATTER(0);

    formatBoundedString("hello", 5);

    CHECK_LENGTH(0);
}

TEST(SolidSyslogFormatter, ZeroSizeUint32IsNoOp)
{
    CREATE_FORMATTER(0);

    formatUint32(42);

    CHECK_LENGTH(0);
}

TEST(SolidSyslogFormatter, OneByteBufferHoldsOnlyNullTerminator)
{
    CREATE_FORMATTER(1);

    formatAsciiCharacter('X');

    CHECK_FORMATTED("");
}

TEST(SolidSyslogFormatter, TwoDigitFormatsAllDigits)
{
    formatTwoDigit(59);

    CHECK_FORMATTED("59");
}

TEST(SolidSyslogFormatter, TwoDigitFormatsZero)
{
    formatTwoDigit(0);

    CHECK_FORMATTED("00");
}

TEST(SolidSyslogFormatter, TwoDigitFormatsMax)
{
    formatTwoDigit(99);

    CHECK_FORMATTED("99");
}

TEST(SolidSyslogFormatter, TwoDigitBeyondMaxFormatsLeastSignificant)
{
    formatTwoDigit(123);

    CHECK_FORMATTED("23");
}

TEST(SolidSyslogFormatter, FourDigitFormatsAllDigits)
{
    formatFourDigit(2009);

    CHECK_FORMATTED("2009");
}

TEST(SolidSyslogFormatter, FourDigitFormatsZero)
{
    formatFourDigit(0);

    CHECK_FORMATTED("0000");
}

TEST(SolidSyslogFormatter, FourDigitFormatsMax)
{
    formatFourDigit(9999);

    CHECK_FORMATTED("9999");
}

TEST(SolidSyslogFormatter, FourDigitBeyondMaxFormatsLeastSignificant)
{
    formatFourDigit(12345);

    CHECK_FORMATTED("2345");
}

TEST(SolidSyslogFormatter, SixDigitFormatsAllDigits)
{
    formatSixDigit(123456);

    CHECK_FORMATTED("123456");
}

TEST(SolidSyslogFormatter, SixDigitFormatsZero)
{
    formatSixDigit(0);

    CHECK_FORMATTED("000000");
}

TEST(SolidSyslogFormatter, SixDigitFormatsMax)
{
    formatSixDigit(999999);

    CHECK_FORMATTED("999999");
}

TEST(SolidSyslogFormatter, SixDigitBeyondMaxFormatsLeastSignificant)
{
    formatSixDigit(1234567);

    CHECK_FORMATTED("234567");
}

TEST(SolidSyslogFormatter, BoundedStringStopsAtBufferCapacity)
{
    CREATE_FORMATTER(4);

    formatBoundedString("abcdef", 10);

    CHECK_FORMATTED("abc");
}

TEST(SolidSyslogFormatter, BoundedStringFillsExactCapacity)
{
    CREATE_FORMATTER(4);

    formatBoundedString("abc", 3);

    CHECK_FORMATTED("abc");
}

TEST(SolidSyslogFormatter, BoundedStringWritesNothingWhenFull)
{
    CREATE_FORMATTER(4);

    formatBoundedString("abc", 3);
    formatBoundedString("xyz", 3);

    CHECK_FORMATTED("abc");
}

TEST(SolidSyslogFormatter, BoundedStringReplacementHiddenFromAsFormattedBufferWhenOutputTooSmall)
{
    /* Replacement is 3 bytes. A 3-byte buffer provides 2 usable bytes
     * (one reserved for NUL), so U+FFFD cannot be fully written. The
     * formatter clamps the third byte; AsFormattedBuffer trims the truncated
     * lead so callers see no invalid UTF-8. Length still reflects the
     * raw bytes the formatter absorbed, making the gap observable. */
    CREATE_FORMATTER(3);

    formatBoundedString("\xC3", 1);

    STRCMP_EQUAL("", SolidSyslogFormatter_AsFormattedBuffer(formatter));
    LONGS_EQUAL(2, SolidSyslogFormatter_Length(formatter));
}

TEST(SolidSyslogFormatter, BoundedStringValidCodepointHiddenFromAsFormattedBufferWhenOutputTooSmall)
{
    /* After writing 'a', only 1 usable byte remains in the 3-byte
     * buffer. The 3-byte codepoint clamps after one byte; AsFormattedBuffer
     * trims the truncated lead so the visible tail is clean. */
    CREATE_FORMATTER(3);

    formatBoundedString("a\xE0\xA0\x80", 4);

    STRCMP_EQUAL("a", SolidSyslogFormatter_AsFormattedBuffer(formatter));
    LONGS_EQUAL(2, SolidSyslogFormatter_Length(formatter));
}

TEST(SolidSyslogFormatter, AsciiCharacterStopsWhenFull)
{
    CREATE_FORMATTER(3);

    formatAsciiCharacter('A');
    formatAsciiCharacter('B');
    formatAsciiCharacter('C');

    CHECK_FORMATTED("AB");
}

TEST(SolidSyslogFormatter, Uint32TruncatedWhenBufferTooSmall)
{
    CREATE_FORMATTER(3);

    formatUint32(12345);

    CHECK_FORMATTED("12");
}

TEST(SolidSyslogFormatter, Uint32FitsExactly)
{
    CREATE_FORMATTER(4);

    formatUint32(123);

    CHECK_FORMATTED("123");
}

TEST(SolidSyslogFormatter, EscapedStringWithEmptyInputWritesNothing)
{
    SolidSyslogFormatter_EscapedString(formatter, "", 0);

    CHECK_FORMATTED("");
}

TEST(SolidSyslogFormatter, EscapedStringPassesOrdinaryCharacterThrough)
{
    SolidSyslogFormatter_EscapedString(formatter, "a", 1);

    CHECK_FORMATTED("a");
}

TEST(SolidSyslogFormatter, EscapedStringTruncatesAtMaxDecodedLength)
{
    SolidSyslogFormatter_EscapedString(formatter, "hello", 3);

    CHECK_FORMATTED("hel");
}

TEST(SolidSyslogFormatter, EscapedStringEscapesDoubleQuote)
{
    SolidSyslogFormatter_EscapedString(formatter, "a\"b", 3);

    CHECK_FORMATTED("a\\\"b");
}

TEST(SolidSyslogFormatter, EscapedStringEscapesBackslash)
{
    SolidSyslogFormatter_EscapedString(formatter, "a\\b", 3);

    CHECK_FORMATTED("a\\\\b");
}

TEST(SolidSyslogFormatter, EscapedStringEscapesCloseBracket)
{
    SolidSyslogFormatter_EscapedString(formatter, "a]b", 3);

    CHECK_FORMATTED("a\\]b");
}

TEST(SolidSyslogFormatter, EscapedStringEscapesAllThreeSpecialsInOneValue)
{
    SolidSyslogFormatter_EscapedString(formatter, "\"\\]", 3);

    CHECK_FORMATTED("\\\"\\\\\\]");
}

TEST(SolidSyslogFormatter, EscapedStringMaxDecodedLengthBoundsDecoderBufferNotEncodedOutput)
{
    /* maxDecodedLength bounds what the reader's decoder would extract
     * (one byte per unescaped character), not the wire bytes we emit.
     * Two decoded quotes fit the bound; each expands to a 2-byte escape
     * pair on the wire, so the output is 4 bytes. */
    SolidSyslogFormatter_EscapedString(formatter, R"(""""")", 2);

    CHECK_FORMATTED(R"(\"\")");
}

TEST(SolidSyslogFormatter, EscapedStringPassesValidUtf8CodepointsThroughAroundEscapedSpecial)
{
    /* \xC2\x80 is a valid 2-byte UTF-8 codepoint. The embedded \" is the
     * only byte requiring escape; the UTF-8 codepoints on either side ride
     * through unchanged. Proves the shared UTF-8 core composes with the
     * single-byte escape decoration. */
    SolidSyslogFormatter_EscapedString(formatter, "\xC2\x80\"\xC2\x80", 5);

    CHECK_FORMATTED("\xC2\x80\\\"\xC2\x80");
}

TEST(SolidSyslogFormatter, EscapedStringReplacesInvalidUtf8ByteWithReplacementChar)
{
    /* \xC0 is an invalid 2-byte lead (overlong); EscapedString must
     * substitute it with U+FFFD via the shared UTF-8 core. The surrounding
     * ASCII rides through unescaped. The decoded budget must accommodate
     * the ASCII bytes plus U+FFFD's 3 decoded bytes. */
    SolidSyslogFormatter_EscapedString(
        formatter,
        "a\xC0"
        "b",
        5
    );

    CHECK_FORMATTED("a\xEF\xBF\xBD"
                    "b");
}

TEST(SolidSyslogFormatter, EscapedStringReplacesStragglingMultiByteLeadWhenSourceTruncated)
{
    /* \xC2 is a 2-byte lead with no continuation available in the source
     * (NUL-terminator follows). The lead cannot complete a codepoint and
     * is demoted to U+FFFD — the decoded budget must accommodate the
     * 3 decoded bytes of the replacement. */
    SolidSyslogFormatter_EscapedString(formatter, "\xC2", 3);

    CHECK_FORMATTED("\xEF\xBF\xBD");
}

TEST(SolidSyslogFormatter, EscapedStringReplacementClaimsThreeBytesOfDecodedBudget)
{
    /* U+FFFD occupies 3 bytes in the reader's decoder buffer. A 4-byte
     * decoded budget that already holds one ASCII byte can admit the
     * replacement (1 + 3 = 4), but has no room for a trailing ASCII byte.
     * This pins both the 3-byte fit check and the += 3 advance for the
     * replacement branch; a regression to 1 in either would let the
     * trailing 'b' through. */
    SolidSyslogFormatter_EscapedString(
        formatter,
        "a\xC0"
        "b",
        4
    );

    CHECK_FORMATTED("a\xEF\xBF\xBD");
}

TEST(SolidSyslogFormatter, EscapedStringBreaksWhenReplacementDoesNotFitDecodedBudget)
{
    /* U+FFFD's 3 decoded bytes do not fit a 2-byte decoded budget, so the
     * replacement cannot be emitted and the loop terminates. */
    SolidSyslogFormatter_EscapedString(formatter, "\xC0", 2);

    CHECK_FORMATTED("");
}

TEST(SolidSyslogFormatter, PrintUsAsciiStringWithEmptyInputWritesNothing)
{
    SolidSyslogFormatter_PrintUsAsciiString(formatter, "", 0);

    CHECK_FORMATTED("");
}

TEST(SolidSyslogFormatter, PrintUsAsciiStringPassesPrintableCharacterThrough)
{
    SolidSyslogFormatter_PrintUsAsciiString(formatter, "A", 1);

    CHECK_FORMATTED("A");
}

TEST(SolidSyslogFormatter, PrintUsAsciiStringTruncatesAtMaxLength)
{
    SolidSyslogFormatter_PrintUsAsciiString(formatter, "hello", 3);

    CHECK_FORMATTED("hel");
}

TEST(SolidSyslogFormatter, PrintUsAsciiStringSubstitutesSpace)
{
    SolidSyslogFormatter_PrintUsAsciiString(formatter, "a b", 3);

    CHECK_FORMATTED("a?b");
}

TEST(SolidSyslogFormatter, PrintUsAsciiStringSubstitutesControlCharacter)
{
    SolidSyslogFormatter_PrintUsAsciiString(
        formatter,
        "a\x01"
        "b",
        3
    );

    CHECK_FORMATTED("a?b");
}

TEST(SolidSyslogFormatter, PrintUsAsciiStringSubstitutesDel)
{
    SolidSyslogFormatter_PrintUsAsciiString(
        formatter,
        "a\x7F"
        "b",
        3
    );

    CHECK_FORMATTED("a?b");
}

TEST(SolidSyslogFormatter, PrintUsAsciiStringSubstitutesHighBitByte)
{
    SolidSyslogFormatter_PrintUsAsciiString(
        formatter,
        "a\xC3"
        "b",
        3
    );

    CHECK_FORMATTED("a?b");
}

TEST(SolidSyslogFormatter, PrintUsAsciiStringPassesBangAndTildeBoundariesThrough)
{
    SolidSyslogFormatter_PrintUsAsciiString(formatter, "!~", 2);

    CHECK_FORMATTED("!~");
}

TEST(SolidSyslogFormatter, PrintUsAsciiStringTruncationBoundsSubstitution)
{
    SolidSyslogFormatter_PrintUsAsciiString(
        formatter,
        "abc\x01"
        "def",
        3
    );

    CHECK_FORMATTED("abc");
}

TEST(SolidSyslogFormatter, BomWritesUtf8ByteOrderMark)
{
    SolidSyslogFormatter_Bom(formatter);

    CHECK_FORMATTED("\xEF\xBB\xBF");
}

TEST(SolidSyslogFormatter, NilValueWritesHyphen)
{
    SolidSyslogFormatter_NilValue(formatter);

    CHECK_FORMATTED("-");
}
