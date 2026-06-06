#include "SolidSyslogEndpointHostPrivate.h"

#include "SolidSyslogFormatter.h"

void SolidSyslogEndpointHost_FromFormatter(struct SolidSyslogEndpointHost* host, struct SolidSyslogFormatter* formatter)
{
    host->Formatter = formatter;
}

void SolidSyslogEndpointHost_String(struct SolidSyslogEndpointHost* host, const char* source, size_t maxLength)
{
    SolidSyslogFormatter_BoundedString(host->Formatter, source, maxLength);
}
