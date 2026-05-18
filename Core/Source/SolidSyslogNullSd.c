#include "SolidSyslogNullSd.h"

#include "SolidSyslogStructuredDataDefinition.h"

static void NullSd_Format(struct SolidSyslogStructuredData* base, struct SolidSyslogFormatter* formatter);

static struct SolidSyslogStructuredData instance = {.Format = NullSd_Format};

struct SolidSyslogStructuredData* SolidSyslogNullSd_Get(void)
{
    return &instance;
}

static void NullSd_Format(struct SolidSyslogStructuredData* base, struct SolidSyslogFormatter* formatter)
{
    (void) base;
    (void) formatter;
}
