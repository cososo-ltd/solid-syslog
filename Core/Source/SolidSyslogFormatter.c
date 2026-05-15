#include "SolidSyslogFormatter.h"
#include "SolidSyslogMacros.h"
#include "SolidSyslogUtf8.h"

#include <stdbool.h>
#include <stddef.h>

struct SolidSyslogFormatter
{
    size_t size;
    size_t position;
    char buffer[];
};

SOLIDSYSLOG_STATIC_ASSERT(
    sizeof(struct SolidSyslogFormatter) == SOLIDSYSLOG_FORMATTER_OVERHEAD * sizeof(SolidSyslogFormatterStorage),
    "SOLIDSYSLOG_FORMATTER_OVERHEAD does not match struct layout"
);

static const char QUOTE = '"';
static const char BACKSLASH = '\\';
static const char CLOSE_BRACKET = ']';
static const char ESCAPE_PREFIX = '\\';
static const char LOWEST_PRINTABLE_US_ASCII = '!';
static const char HIGHEST_PRINTABLE_US_ASCII = '~';
static const char NON_PRINTABLE_SUBSTITUTE = '?';

/* UTF-8 replacement character U+FFFD, emitted in place of each invalid
 * byte per Unicode §3.9 best practice for per-byte maximal subpart. */
static const char REPLACEMENT_CHARACTER[] = {'\xEF', '\xBF', '\xBD'};

/* UTF-8 byte order mark U+FEFF, required at the start of a UTF-8 MSG
 * by RFC 5424 §6.4. */
static const char UTF8_BOM[] = {'\xEF', '\xBB', '\xBF'};

/* An escape pair on the wire ('\' + char) decodes back to the single
 * character it was escaping — one byte in the reader's decoder buffer. */
static const size_t ESCAPED_CHARACTER_DECODED_LENGTH = 1;

/* Mutable state threaded through the EscapedString writer helpers.
 * sourcePos walks the source until NUL; decodedLength counts bytes the
 * reader's decoder would extract; exhausted flips when a writer can't
 * fit and the loop must terminate. */
struct EscapedContext
{
    struct SolidSyslogFormatter* formatter;
    const char* source;
    size_t sourcePos;
    size_t decodedLength;
    size_t maxDecodedLength;
    bool exhausted;
};

static inline bool Formatter_CodepointFits(size_t codepointLength, size_t remainingDecodedLength);
static inline bool Formatter_Fits(const struct EscapedContext* context, size_t decodedAdvance);
static inline bool Formatter_HasCapacity(const struct SolidSyslogFormatter* formatter);
static inline bool Formatter_IsAboveUnicodeMaxEncoding(char lead, char continuation1);
static inline bool Formatter_IsOverlongFourByteEncoding(char lead, char continuation1);
static inline bool Formatter_IsOverlongThreeByteEncoding(char lead, char continuation1);
static inline bool Formatter_IsOverlongTwoByteLead(char byte);
static inline bool Formatter_IsAsciiCharacter(char value);
static inline bool Formatter_IsExhausted(const struct EscapedContext* context);
static inline bool Formatter_IsPrintableUsAscii(char value);
static inline bool Formatter_IsUtf16SurrogateEncoding(char lead, char continuation1);
static inline bool Formatter_IsValidUtf8FourByte(char lead, char continuation1, char continuation2, char continuation3);
static inline bool Formatter_IsValidUtf8ThreeByte(char lead, char continuation1, char continuation2);
static inline bool Formatter_IsValidUtf8TwoByte(char lead, char continuation1);
static inline bool Formatter_NeedsEscape(char value);
static inline char Formatter_DigitToChar(uint32_t value);
static size_t Formatter_CountDigits(uint32_t value);
static inline size_t Formatter_Utf8CodepointLength(const char* source);
static inline void Formatter_Exhaust(struct EscapedContext* context);
static inline void Formatter_NullTerminate(struct SolidSyslogFormatter* formatter);
static inline void Formatter_TrimTruncatedMultiByteTail(struct SolidSyslogFormatter* formatter);
static inline void Formatter_WriteBytes(struct SolidSyslogFormatter* formatter, const char* bytes, size_t count);
static inline void Formatter_WriteChar(struct SolidSyslogFormatter* formatter, char value);
static inline void Formatter_WriteCodepoint(struct EscapedContext* context);
/* NOLINTBEGIN(bugprone-easily-swappable-parameters) -- 3 callers in this file pass compile-time constants or codepointLength; param names disambiguate */
static inline void Formatter_WriteContext(
    struct EscapedContext* context,
    const char* bytes,
    size_t byteCount,
    size_t sourceAdvance,
    size_t decodedAdvance
);
/* NOLINTEND(bugprone-easily-swappable-parameters) */
static inline void Formatter_WriteEscaped(struct EscapedContext* context);
static inline void Formatter_WritePrintableUsAsciiChar(struct SolidSyslogFormatter* formatter, char value);
static inline void Formatter_WriteReplacement(struct EscapedContext* context);

struct SolidSyslogFormatter* SolidSyslogFormatter_Create(SolidSyslogFormatterStorage* storage, size_t bufferSize)
{
    struct SolidSyslogFormatter* formatter = (struct SolidSyslogFormatter*) storage;
    formatter->size = bufferSize;
    formatter->position = 0;
    Formatter_NullTerminate(formatter);
    return formatter;
}

static inline void Formatter_NullTerminate(struct SolidSyslogFormatter* formatter)
{
    if (formatter->size > 0)
    {
        formatter->buffer[formatter->position] = '\0';
    }
}

void SolidSyslogFormatter_AsciiCharacter(struct SolidSyslogFormatter* formatter, char value)
{
    if (!Formatter_IsAsciiCharacter(value))
    {
        value = NON_PRINTABLE_SUBSTITUTE;
    }
    Formatter_WriteChar(formatter, value);
    Formatter_NullTerminate(formatter);
}

void SolidSyslogFormatter_Bom(struct SolidSyslogFormatter* formatter)
{
    Formatter_WriteBytes(formatter, UTF8_BOM, sizeof(UTF8_BOM));
    Formatter_NullTerminate(formatter);
}

static inline bool Formatter_IsAsciiCharacter(char value)
{
    return (value == ' ') || Formatter_IsPrintableUsAscii(value);
}

static inline void Formatter_WriteChar(struct SolidSyslogFormatter* formatter, char value)
{
    if (Formatter_HasCapacity(formatter))
    {
        formatter->buffer[formatter->position] = value;
        formatter->position++;
    }
}

static inline bool Formatter_HasCapacity(const struct SolidSyslogFormatter* formatter)
{
    return (formatter->size > 0) && (formatter->position < formatter->size - 1);
}

/*
 * UTF-8 validation per RFC 3629 §4 and Unicode §3.9.
 *
 *   1-byte  0xxxxxxx                                   U+0000  - U+007F
 *   2-byte  110xxxxx 10xxxxxx                          U+0080  - U+07FF    lead C2-DF
 *   3-byte  1110xxxx 10xxxxxx 10xxxxxx                 U+0800  - U+FFFF    lead E0-EF
 *   4-byte  11110xxx 10xxxxxx 10xxxxxx 10xxxxxx        U+10000 - U+10FFFF  lead F0-F4
 *
 * Excluded sequences:
 *   C0, C1             overlong 2-byte encodings
 *   E0 + cont1 80-9F   overlong 3-byte encodings
 *   ED + cont1 A0-BF   UTF-16 surrogate range (U+D800..U+DFFF)
 *   F0 + cont1 80-8F   overlong 4-byte encodings
 *   F4 + cont1 90-BF   codepoints above U+10FFFF
 *   F5-F7              codepoints above U+10FFFF
 *   F8-FF              5+ byte prefix patterns removed by RFC 3629
 */

void SolidSyslogFormatter_BoundedString(struct SolidSyslogFormatter* formatter, const char* source, size_t maxLength)
{
    size_t len = 0;

    while ((len < maxLength) && (source[len] != '\0'))
    {
        size_t codepointLength = Formatter_Utf8CodepointLength(&source[len]);

        if (Formatter_CodepointFits(codepointLength, maxLength - len))
        {
            Formatter_WriteBytes(formatter, &source[len], codepointLength);
            len += codepointLength;
        }
        else
        {
            Formatter_WriteBytes(formatter, REPLACEMENT_CHARACTER, sizeof(REPLACEMENT_CHARACTER));
            len += 1;
        }
    }
    Formatter_NullTerminate(formatter);
}

static inline bool Formatter_CodepointFits(size_t codepointLength, size_t remainingDecodedLength)
{
    return (codepointLength > 0) && (codepointLength <= remainingDecodedLength);
}

static inline size_t Formatter_Utf8CodepointLength(const char* source)
{
    size_t length = 0;

    if (SolidSyslogUtf8_IsAsciiByte(source[0]))
    {
        length = 1;
    }
    else if ((source[1] != '\0') && Formatter_IsValidUtf8TwoByte(source[0], source[1]))
    {
        length = 2;
    }
    else if ((source[1] != '\0') && (source[2] != '\0') &&
             Formatter_IsValidUtf8ThreeByte(source[0], source[1], source[2]))
    {
        length = 3;
    }
    else if ((source[1] != '\0') && (source[2] != '\0') && (source[3] != '\0') &&
             Formatter_IsValidUtf8FourByte(source[0], source[1], source[2], source[3]))
    {
        length = 4;
    }

    return length;
}

static inline bool Formatter_IsValidUtf8TwoByte(char lead, char continuation1)
{
    return SolidSyslogUtf8_IsTwoByteLead(lead) && !Formatter_IsOverlongTwoByteLead(lead) &&
           SolidSyslogUtf8_IsContinuationByte(continuation1);
}

static inline bool Formatter_IsOverlongTwoByteLead(char byte)
{
    return (byte & 0xFE) == 0xC0;
}

static inline bool Formatter_IsValidUtf8ThreeByte(char lead, char continuation1, char continuation2)
{
    return SolidSyslogUtf8_IsThreeByteLead(lead) && SolidSyslogUtf8_IsContinuationByte(continuation1) &&
           SolidSyslogUtf8_IsContinuationByte(continuation2) &&
           !Formatter_IsOverlongThreeByteEncoding(lead, continuation1) &&
           !Formatter_IsUtf16SurrogateEncoding(lead, continuation1);
}

static inline bool Formatter_IsOverlongThreeByteEncoding(char lead, char continuation1)
{
    return (lead == '\xE0') && ((continuation1 & 0xE0) == 0x80);
}

static inline bool Formatter_IsUtf16SurrogateEncoding(char lead, char continuation1)
{
    return (lead == '\xED') && ((continuation1 & 0xE0) == 0xA0);
}

static inline bool Formatter_IsValidUtf8FourByte(char lead, char continuation1, char continuation2, char continuation3)
{
    return SolidSyslogUtf8_IsFourByteLead(lead) && SolidSyslogUtf8_IsContinuationByte(continuation1) &&
           SolidSyslogUtf8_IsContinuationByte(continuation2) && SolidSyslogUtf8_IsContinuationByte(continuation3) &&
           !Formatter_IsOverlongFourByteEncoding(lead, continuation1) &&
           !Formatter_IsAboveUnicodeMaxEncoding(lead, continuation1);
}

static inline bool Formatter_IsOverlongFourByteEncoding(char lead, char continuation1)
{
    return (lead == '\xF0') && ((continuation1 & 0xF0) == 0x80);
}

static inline bool Formatter_IsAboveUnicodeMaxEncoding(char lead, char continuation1)
{
    bool f4WithCont1Above8F = (lead == '\xF4') && ((continuation1 & 0xF0) != 0x80);
    bool f5OrHigherLead = (lead == '\xF5') || (lead == '\xF6') || (lead == '\xF7');
    return f4WithCont1Above8F || f5OrHigherLead;
}

static inline void Formatter_WriteBytes(struct SolidSyslogFormatter* formatter, const char* bytes, size_t count)
{
    for (size_t i = 0; i < count; i++)
    {
        Formatter_WriteChar(formatter, bytes[i]);
    }
}

void SolidSyslogFormatter_EscapedString(
    struct SolidSyslogFormatter* formatter,
    const char* source,
    size_t maxDecodedLength
)
{
    struct EscapedContext context = {
        .formatter = formatter,
        .source = source,
        .sourcePos = 0,
        .decodedLength = 0,
        .maxDecodedLength = maxDecodedLength,
        .exhausted = false,
    };

    while (!Formatter_IsExhausted(&context))
    {
        if (Formatter_NeedsEscape(source[context.sourcePos]))
        {
            Formatter_WriteEscaped(&context);
        }
        else
        {
            Formatter_WriteCodepoint(&context);
        }
    }
    Formatter_NullTerminate(formatter);
}

static inline bool Formatter_NeedsEscape(char value)
{
    return (value == QUOTE) || (value == BACKSLASH) || (value == CLOSE_BRACKET);
}

static inline bool Formatter_IsExhausted(const struct EscapedContext* context)
{
    return context->exhausted || (context->source[context->sourcePos] == '\0');
}

static inline void Formatter_WriteEscaped(struct EscapedContext* context)
{
    if (Formatter_Fits(context, ESCAPED_CHARACTER_DECODED_LENGTH))
    {
        char escaped[] = {ESCAPE_PREFIX, context->source[context->sourcePos]};
        Formatter_WriteContext(context, escaped, sizeof(escaped), 1, ESCAPED_CHARACTER_DECODED_LENGTH);
        return;
    }
    Formatter_Exhaust(context);
}

static inline bool Formatter_Fits(const struct EscapedContext* context, size_t decodedAdvance)
{
    return (decodedAdvance > 0) && (decodedAdvance <= context->maxDecodedLength - context->decodedLength);
}

/* NOLINTBEGIN(bugprone-easily-swappable-parameters) -- see forward declaration */
static inline void Formatter_WriteContext(
    struct EscapedContext* context,
    const char* bytes,
    size_t byteCount,
    size_t sourceAdvance,
    size_t decodedAdvance
)
{
    Formatter_WriteBytes(context->formatter, bytes, byteCount);
    context->sourcePos += sourceAdvance;
    context->decodedLength += decodedAdvance;
}

/* NOLINTEND(bugprone-easily-swappable-parameters) */

static inline void Formatter_Exhaust(struct EscapedContext* context)
{
    context->exhausted = true;
}

/* Writes the source codepoint at sourcePos if it fits the decoded budget;
 * otherwise falls back to Formatter_WriteReplacement (which emits U+FFFD or marks
 * the context exhausted). Formatter_Fits also rejects invalid UTF-8 because
 * Formatter_Utf8CodepointLength returns 0 for ill-formed sequences. */
static inline void Formatter_WriteCodepoint(struct EscapedContext* context)
{
    size_t codepointLength = Formatter_Utf8CodepointLength(&context->source[context->sourcePos]);
    if (Formatter_Fits(context, codepointLength))
    {
        Formatter_WriteContext(
            context,
            &context->source[context->sourcePos],
            codepointLength,
            codepointLength,
            codepointLength
        );
        return;
    }
    Formatter_WriteReplacement(context);
}

static inline void Formatter_WriteReplacement(struct EscapedContext* context)
{
    if (Formatter_Fits(context, sizeof(REPLACEMENT_CHARACTER)))
    {
        Formatter_WriteContext(
            context,
            REPLACEMENT_CHARACTER,
            sizeof(REPLACEMENT_CHARACTER),
            1,
            sizeof(REPLACEMENT_CHARACTER)
        );
        return;
    }
    Formatter_Exhaust(context);
}

void SolidSyslogFormatter_PrintUsAsciiString(
    struct SolidSyslogFormatter* formatter,
    const char* source,
    size_t maxLength
)
{
    size_t len = 0;

    while ((len < maxLength) && (source[len] != '\0'))
    {
        Formatter_WritePrintableUsAsciiChar(formatter, source[len]);
        len++;
    }
    Formatter_NullTerminate(formatter);
}

static inline void Formatter_WritePrintableUsAsciiChar(struct SolidSyslogFormatter* formatter, char value)
{
    if (Formatter_IsPrintableUsAscii(value))
    {
        Formatter_WriteChar(formatter, value);
    }
    else
    {
        Formatter_WriteChar(formatter, NON_PRINTABLE_SUBSTITUTE);
    }
}

static inline bool Formatter_IsPrintableUsAscii(char value)
{
    return (value >= LOWEST_PRINTABLE_US_ASCII) && (value <= HIGHEST_PRINTABLE_US_ASCII);
}

void SolidSyslogFormatter_Uint32(struct SolidSyslogFormatter* formatter, uint32_t value)
{
    size_t digits = Formatter_CountDigits(value);
    uint32_t divisor = 1;

    for (size_t i = 1; i < digits; i++)
    {
        divisor *= 10U;
    }

    for (size_t i = 0; i < digits; i++)
    {
        Formatter_WriteChar(formatter, Formatter_DigitToChar(value / divisor));
        value %= divisor;
        divisor /= 10U;
    }
    Formatter_NullTerminate(formatter);
}

static size_t Formatter_CountDigits(uint32_t value)
{
    size_t count = 1;

    while (value >= 10U)
    {
        count++;
        value /= 10U;
    }

    return count;
}

static inline char Formatter_DigitToChar(uint32_t value)
{
    return (char) ('0' + (value % 10U));
}

void SolidSyslogFormatter_TwoDigit(struct SolidSyslogFormatter* formatter, uint32_t value)
{
    Formatter_WriteChar(formatter, Formatter_DigitToChar(value / 10U));
    Formatter_WriteChar(formatter, Formatter_DigitToChar(value));
    Formatter_NullTerminate(formatter);
}

void SolidSyslogFormatter_FourDigit(struct SolidSyslogFormatter* formatter, uint32_t value)
{
    Formatter_WriteChar(formatter, Formatter_DigitToChar(value / 1000U));
    Formatter_WriteChar(formatter, Formatter_DigitToChar(value / 100U));
    Formatter_WriteChar(formatter, Formatter_DigitToChar(value / 10U));
    Formatter_WriteChar(formatter, Formatter_DigitToChar(value));
    Formatter_NullTerminate(formatter);
}

void SolidSyslogFormatter_SixDigit(struct SolidSyslogFormatter* formatter, uint32_t value)
{
    Formatter_WriteChar(formatter, Formatter_DigitToChar(value / 100000U));
    Formatter_WriteChar(formatter, Formatter_DigitToChar(value / 10000U));
    Formatter_WriteChar(formatter, Formatter_DigitToChar(value / 1000U));
    Formatter_WriteChar(formatter, Formatter_DigitToChar(value / 100U));
    Formatter_WriteChar(formatter, Formatter_DigitToChar(value / 10U));
    Formatter_WriteChar(formatter, Formatter_DigitToChar(value));
    Formatter_NullTerminate(formatter);
}

const char* SolidSyslogFormatter_AsFormattedBuffer(struct SolidSyslogFormatter* formatter)
{
    Formatter_TrimTruncatedMultiByteTail(formatter);
    return formatter->buffer;
}

static inline void Formatter_TrimTruncatedMultiByteTail(struct SolidSyslogFormatter* formatter)
{
    char* buffer = formatter->buffer;
    size_t p = formatter->position;
    size_t trimFrom = p;

    if ((p >= 1) && (SolidSyslogUtf8_IsTwoByteLead(buffer[p - 1]) || SolidSyslogUtf8_IsThreeByteLead(buffer[p - 1]) ||
                     SolidSyslogUtf8_IsFourByteLead(buffer[p - 1])))
    {
        trimFrom = p - 1;
    }
    else if ((p >= 2) &&
             (SolidSyslogUtf8_IsThreeByteLead(buffer[p - 2]) || SolidSyslogUtf8_IsFourByteLead(buffer[p - 2])))
    {
        trimFrom = p - 2;
    }
    else if ((p >= 3) && SolidSyslogUtf8_IsFourByteLead(buffer[p - 3]))
    {
        trimFrom = p - 3;
    }
    for (size_t i = trimFrom; i < p; i++)
    {
        buffer[i] = '\0';
    }
}

size_t SolidSyslogFormatter_Length(const struct SolidSyslogFormatter* formatter)
{
    return formatter->position;
}
