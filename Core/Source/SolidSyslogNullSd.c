#include "SolidSyslogNullSd.h"

#include "SolidSyslogStructuredDataDefinition.h"

struct SolidSyslogFormatter;

static void NullSd_Format(struct SolidSyslogStructuredData* base, struct SolidSyslogFormatter* formatter);

struct SolidSyslogStructuredData* SolidSyslogNullSd_Get(void)
{
    static struct SolidSyslogStructuredData instance = {.Format = NullSd_Format};
    return &instance;
}

static void NullSd_Format(struct SolidSyslogStructuredData* base, struct SolidSyslogFormatter* formatter)
{
    (void) base;
    (void) formatter;
}
