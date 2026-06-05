#include "SolidSyslogSdValuePrivate.h"

#include <stdint.h>

#include "SolidSyslogFormatter.h"

void SolidSyslogSdValue_FromFormatter(struct SolidSyslogSdValue* value, struct SolidSyslogFormatter* formatter)
{
    value->Formatter = formatter;
}

void SolidSyslogSdValue_String(struct SolidSyslogSdValue* value, const char* source)
{
    SolidSyslogFormatter_EscapedString(value->Formatter, source, SIZE_MAX);
}

void SolidSyslogSdValue_BoundedString(struct SolidSyslogSdValue* value, const char* source, size_t maxDecodedLength)
{
    SolidSyslogFormatter_EscapedString(value->Formatter, source, maxDecodedLength);
}

void SolidSyslogSdValue_Uint32(struct SolidSyslogSdValue* value, uint32_t number)
{
    SolidSyslogFormatter_Uint32(value->Formatter, number);
}
