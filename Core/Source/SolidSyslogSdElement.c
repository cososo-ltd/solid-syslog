#include "SolidSyslogSdElementPrivate.h"

#include <stddef.h>

#include "SolidSyslogFormatter.h"

enum
{
    SDELEMENT_NAME_MAX = 32
};

static inline void SdElement_CloseOpenValue(struct SolidSyslogSdElement* element);
static inline struct SolidSyslogSdValue* SdElement_SkipParam(struct SolidSyslogSdElement* element);

void SolidSyslogSdElement_FromFormatter(struct SolidSyslogSdElement* element, struct SolidSyslogFormatter* formatter)
{
    element->Formatter = formatter;
    element->DropFormatter = SolidSyslogFormatter_Create(element->DropStorage, 0);
    element->ValueOpen = false;
    element->Suppressed = false;
}

void SolidSyslogSdElement_Begin(struct SolidSyslogSdElement* element, const char* name, uint32_t enterpriseNumber)
{
    element->ValueOpen = false;
    element->Suppressed = (name == NULL);
    if (!element->Suppressed)
    {
        SolidSyslogFormatter_AsciiCharacter(element->Formatter, '[');
        SolidSyslogFormatter_PrintUsAsciiString(element->Formatter, name, SDELEMENT_NAME_MAX);
        if (enterpriseNumber != 0U)
        {
            SolidSyslogFormatter_AsciiCharacter(element->Formatter, '@');
            SolidSyslogFormatter_Uint32(element->Formatter, enterpriseNumber);
        }
    }
}

struct SolidSyslogSdValue* SolidSyslogSdElement_Param(struct SolidSyslogSdElement* element, const char* name)
{
    struct SolidSyslogSdValue* sink = NULL;
    SdElement_CloseOpenValue(element);
    if (element->Suppressed || (name == NULL))
    {
        sink = SdElement_SkipParam(element);
    }
    else
    {
        SolidSyslogFormatter_AsciiCharacter(element->Formatter, ' ');
        SolidSyslogFormatter_PrintUsAsciiString(element->Formatter, name, SDELEMENT_NAME_MAX);
        SolidSyslogFormatter_AsciiCharacter(element->Formatter, '=');
        SolidSyslogFormatter_AsciiCharacter(element->Formatter, '"');
        SolidSyslogSdValue_FromFormatter(&element->Value, element->Formatter);
        element->ValueOpen = true;
        sink = &element->Value;
    }
    return sink;
}

/* A skipped param (NULL name, or a suppressed element) opens no framing and
 * hands back a sink over the drop formatter, so the caller's value writes are
 * absorbed without disturbing the element. ValueOpen stays false — there is no
 * quote for the next _Param / _End to close. */
static inline struct SolidSyslogSdValue* SdElement_SkipParam(struct SolidSyslogSdElement* element)
{
    SolidSyslogSdValue_FromFormatter(&element->Value, element->DropFormatter);
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
    if (!element->Suppressed)
    {
        SolidSyslogFormatter_AsciiCharacter(element->Formatter, ']');
    }
}
