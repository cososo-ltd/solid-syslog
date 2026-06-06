#include "SolidSyslogStructuredDataDefinition.h"
#include "SolidSyslogStructuredData.h"

struct SolidSyslogSdElement;

void SolidSyslogStructuredData_Format(struct SolidSyslogStructuredData* sd, struct SolidSyslogSdElement* element)
{
    sd->Format(sd, element);
}
