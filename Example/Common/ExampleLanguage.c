#include "ExampleLanguage.h"
#include "SolidSyslogFormatter.h"

static const char LANGUAGE_TAG[] = "en-GB";

void ExampleLanguage_Get(struct SolidSyslogFormatter* formatter)
{
    SolidSyslogFormatter_EscapedString(formatter, LANGUAGE_TAG, sizeof(LANGUAGE_TAG) - 1);
}
