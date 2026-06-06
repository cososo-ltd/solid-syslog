#include "SolidSyslogNullSd.h"

#include "SolidSyslogStructuredDataDefinition.h"

struct SolidSyslogSdElement;

static void NullSd_Format(struct SolidSyslogStructuredData* base, struct SolidSyslogSdElement* element);

struct SolidSyslogStructuredData* SolidSyslogNullSd_Get(void)
{
    static struct SolidSyslogStructuredData instance = {.Format = NullSd_Format};
    return &instance;
}

static void NullSd_Format(struct SolidSyslogStructuredData* base, struct SolidSyslogSdElement* element)
{
    (void) base;
    (void) element;
}
