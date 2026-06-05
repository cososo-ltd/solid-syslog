#include "SolidSyslogSdElementPrivate.h"

#include "SolidSyslogFormatter.h"

enum
{
    SDELEMENT_NAME_MAX = 32
};

void SolidSyslogSdElement_FromFormatter(struct SolidSyslogSdElement* element, struct SolidSyslogFormatter* formatter)
{
    element->Formatter = formatter;
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
    SolidSyslogFormatter_AsciiCharacter(element->Formatter, ' ');
    SolidSyslogFormatter_PrintUsAsciiString(element->Formatter, name, SDELEMENT_NAME_MAX);
    SolidSyslogFormatter_AsciiCharacter(element->Formatter, '=');
    SolidSyslogFormatter_AsciiCharacter(element->Formatter, '"');
    SolidSyslogSdValue_FromFormatter(&element->Value, element->Formatter);
    return &element->Value;
}
