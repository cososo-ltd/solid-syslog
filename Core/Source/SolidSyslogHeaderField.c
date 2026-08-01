#include "SolidSyslogHeaderFieldPrivate.h"

#include <stddef.h>

#include "SolidSyslogFormatter.h"

static inline size_t HeaderField_Consumed(const struct SolidSyslogHeaderField* field, size_t before);

void SolidSyslogHeaderField_FromFormatter(
    struct SolidSyslogHeaderField* field,
    struct SolidSyslogFormatter* formatter,
    size_t maxLength
)
{
    field->Formatter = formatter;
    field->Remaining = maxLength;
}

void SolidSyslogHeaderField_PrintUsAscii(struct SolidSyslogHeaderField* field, const char* source, size_t maxLength)
{
    size_t limit = (maxLength < field->Remaining) ? maxLength : field->Remaining;
    size_t before = SolidSyslogFormatter_Length(field->Formatter);

    SolidSyslogFormatter_PrintUsAsciiString(field->Formatter, source, limit);
    field->Remaining -= HeaderField_Consumed(field, before);
}

void SolidSyslogHeaderField_Uint32(struct SolidSyslogHeaderField* field, uint32_t value)
{
    /* Unlike SolidSyslogHeaderField_PrintUsAscii, SolidSyslogHeaderField_Uint32
     * cannot pre-clamp its output to the field budget (the formatter writes the
     * whole number), so guard the budget
     * directly: skip once the field is full, and clamp to zero rather than
     * letting the size_t subtraction underflow when this number alone overruns
     * the remainder. A header-field callback may mix any number of appends. */
    if (field->Remaining > 0U)
    {
        size_t before = SolidSyslogFormatter_Length(field->Formatter);

        SolidSyslogFormatter_Uint32(field->Formatter, value);
        size_t consumed = HeaderField_Consumed(field, before);
        field->Remaining = (consumed >= field->Remaining) ? 0U : (field->Remaining - consumed);
    }
}

static inline size_t HeaderField_Consumed(const struct SolidSyslogHeaderField* field, size_t before)
{
    return SolidSyslogFormatter_Length(field->Formatter) - before;
}
