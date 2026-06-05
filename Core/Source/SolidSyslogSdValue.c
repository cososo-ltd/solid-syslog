#include "SolidSyslogSdValuePrivate.h"

#include <stddef.h>
#include <stdint.h>

#include "SolidSyslogFormatter.h"
#include "SolidSyslogUtf8.h"

static size_t SdValue_DrainPending(struct SolidSyslogSdValue* value, const char* source);
static void SdValue_WriteBody(struct SolidSyslogSdValue* value, const char* source);
static inline size_t SdValue_ExpectedLength(char lead);
static inline void SdValue_EmitUnit(struct SolidSyslogSdValue* value, const char* bytes, size_t count);
static inline void SdValue_Hold(struct SolidSyslogSdValue* value, const char* bytes, size_t count);
static inline void SdValue_FlushPendingAsReplacement(struct SolidSyslogSdValue* value);
static inline void SdValue_FlushPendingIfHeld(struct SolidSyslogSdValue* value);

void SolidSyslogSdValue_FromFormatter(struct SolidSyslogSdValue* value, struct SolidSyslogFormatter* formatter)
{
    value->Formatter = formatter;
    value->PendingCount = 0;
}

void SolidSyslogSdValue_String(struct SolidSyslogSdValue* value, const char* source)
{
    size_t bodyStart = SdValue_DrainPending(value, source);
    SdValue_WriteBody(value, &source[bodyStart]);
}

/* Completes a held trailing sequence from the leading continuation bytes of
 * source, returning the number of source bytes consumed doing so. A completed
 * codepoint is emitted; a sequence interrupted by a non-continuation byte is
 * replaced with one U+FFFD; a sequence still incomplete at end-of-source stays
 * held for the next call. */
static size_t SdValue_DrainPending(struct SolidSyslogSdValue* value, const char* source)
{
    size_t consumed = 0;
    if (value->PendingCount > 0U)
    {
        size_t expected = SdValue_ExpectedLength(value->Pending[0]);
        while ((value->PendingCount < expected) && SolidSyslogUtf8_IsContinuationByte(source[consumed]))
        {
            value->Pending[value->PendingCount] = source[consumed];
            value->PendingCount++;
            consumed++;
        }
        if (value->PendingCount == expected)
        {
            SdValue_EmitUnit(value, value->Pending, value->PendingCount);
            value->PendingCount = 0;
        }
        else if (source[consumed] != '\0')
        {
            SdValue_FlushPendingAsReplacement(value);
        }
        else
        {
            /* source exhausted mid-sequence — keep the tail for the next call */
        }
    }
    return consumed;
}

/* Walks source one UTF-8 unit at a time, emitting each complete unit through
 * the formatter's escaper. A unit whose continuation bytes run out at
 * end-of-source is held for the next call rather than emitted. */
static void SdValue_WriteBody(struct SolidSyslogSdValue* value, const char* source)
{
    size_t i = 0;
    while (source[i] != '\0')
    {
        size_t expected = SdValue_ExpectedLength(source[i]);
        size_t avail = 1;
        while ((avail < expected) && SolidSyslogUtf8_IsContinuationByte(source[i + avail]))
        {
            avail++;
        }
        if ((avail < expected) && (source[i + avail] == '\0'))
        {
            SdValue_Hold(value, &source[i], avail);
        }
        else
        {
            SdValue_EmitUnit(value, &source[i], avail);
        }
        i += avail;
    }
}

/* The number of bytes a UTF-8 sequence starting with lead occupies, by its
 * top-bit structure alone. ASCII, escapable, orphan-continuation and invalid
 * leads are single-byte units; validity is left to the formatter's escaper. */
static inline size_t SdValue_ExpectedLength(char lead)
{
    size_t length = 1;
    if (SolidSyslogUtf8_IsTwoByteLead(lead))
    {
        length = 2;
    }
    else if (SolidSyslogUtf8_IsThreeByteLead(lead))
    {
        length = 3;
    }
    else if (SolidSyslogUtf8_IsFourByteLead(lead))
    {
        length = 4;
    }
    else
    {
        /* single-byte unit */
    }
    return length;
}

/* Emits one UTF-8 unit through the formatter's escaper — the single source of
 * truth for escaping ('"', '\\', ']') and per-byte ill-formed substitution. */
static inline void SdValue_EmitUnit(struct SolidSyslogSdValue* value, const char* bytes, size_t count)
{
    char unit[SDVALUE_MAX_CODEPOINT_BYTES + 1U];
    for (size_t i = 0; i < count; i++)
    {
        unit[i] = bytes[i];
    }
    unit[count] = '\0';
    SolidSyslogFormatter_EscapedString(value->Formatter, unit, SIZE_MAX);
}

static inline void SdValue_Hold(struct SolidSyslogSdValue* value, const char* bytes, size_t count)
{
    for (size_t i = 0; i < count; i++)
    {
        value->Pending[i] = bytes[i];
    }
    value->PendingCount = count;
}

/* Re-uses the formatter's own ill-formed-byte substitution: a lone
 * continuation byte decodes to exactly one U+FFFD, so the held tail is replaced
 * without SdValue duplicating the replacement character. */
static inline void SdValue_FlushPendingAsReplacement(struct SolidSyslogSdValue* value)
{
    static const char LONE_CONTINUATION[] = {'\x80', '\0'};
    SolidSyslogFormatter_EscapedString(value->Formatter, LONE_CONTINUATION, SIZE_MAX);
    value->PendingCount = 0;
}

void SolidSyslogSdValue_BoundedString(struct SolidSyslogSdValue* value, const char* source, size_t maxDecodedLength)
{
    SdValue_FlushPendingIfHeld(value);
    SolidSyslogFormatter_EscapedString(value->Formatter, source, maxDecodedLength);
}

void SolidSyslogSdValue_Uint32(struct SolidSyslogSdValue* value, uint32_t number)
{
    SdValue_FlushPendingIfHeld(value);
    SolidSyslogFormatter_Uint32(value->Formatter, number);
}

void SolidSyslogSdValue_Close(struct SolidSyslogSdValue* value)
{
    SdValue_FlushPendingIfHeld(value);
}

/* A held incomplete UTF-8 tail is the dangling state of a prior _String call.
 * Any non-_String write (or close) terminates it with one U+FFFD before its own
 * output, so the tail is never reordered after, or merged into, that output.
 * Only consecutive _String calls continue a tail (streaming continuation). */
static inline void SdValue_FlushPendingIfHeld(struct SolidSyslogSdValue* value)
{
    if (value->PendingCount > 0U)
    {
        SdValue_FlushPendingAsReplacement(value);
    }
}
