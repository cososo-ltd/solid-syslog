#include "SolidSyslogSdElementPrivate.h"

#include "SolidSyslogFormatter.h"

enum
{
    SDELEMENT_NAME_MAX = 32
};

static inline void SdElement_CloseOpenValue(struct SolidSyslogSdElement* element);

void SolidSyslogSdElement_FromFormatter(struct SolidSyslogSdElement* element, struct SolidSyslogFormatter* formatter)
{
    element->Formatter = formatter;
    element->ValueOpen = false;
}

void SolidSyslogSdElement_Begin(struct SolidSyslogSdElement* element, const char* name, uint32_t enterpriseNumber)
{
    SolidSyslogFormatter_AsciiCharacter(element->Formatter, '[');
    SolidSyslogFormatter_PrintUsAsciiString(element->Formatter, name, SDELEMENT_NAME_MAX);
    if (enterpriseNumber != 0U)
    {
        SolidSyslogFormatter_AsciiCharacter(element->Formatter, '@');
        SolidSyslogFormatter_Uint32(element->Formatter, enterpriseNumber);
    }
}

struct SolidSyslogSdValue* SolidSyslogSdElement_Param(struct SolidSyslogSdElement* element, const char* name)
{
    SdElement_CloseOpenValue(element);
    SolidSyslogFormatter_AsciiCharacter(element->Formatter, ' ');
    SolidSyslogFormatter_PrintUsAsciiString(element->Formatter, name, SDELEMENT_NAME_MAX);
    SolidSyslogFormatter_AsciiCharacter(element->Formatter, '=');
    SolidSyslogFormatter_AsciiCharacter(element->Formatter, '"');
    SolidSyslogSdValue_FromFormatter(&element->Value, element->Formatter);
    element->ValueOpen = true;
    return &element->Value;
}

/* Closes an open param value: flushes any held UTF-8 tail (one U+FFFD) and
 * emits the closing quote. Idempotent when no value is open, so both _Param
 * (before the next param) and _End can call it unconditionally. */
static inline void SdElement_CloseOpenValue(struct SolidSyslogSdElement* element)
{
    if (element->ValueOpen)
    {
        SolidSyslogSdValue_Close(&element->Value);
        SolidSyslogFormatter_AsciiCharacter(element->Formatter, '"');
        element->ValueOpen = false;
    }
}

void SolidSyslogSdElement_End(struct SolidSyslogSdElement* element)
{
    SdElement_CloseOpenValue(element);
    SolidSyslogFormatter_AsciiCharacter(element->Formatter, ']');
}
