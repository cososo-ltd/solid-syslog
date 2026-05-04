#include "SolidSyslogStructuredDataDefinition.h"
#include "SolidSyslogStructuredData.h"

struct SolidSyslogFormatter;

void SolidSyslogStructuredData_Format(struct SolidSyslogStructuredData* sd, struct SolidSyslogFormatter* formatter)
{
    sd->Format(sd, formatter);
}
