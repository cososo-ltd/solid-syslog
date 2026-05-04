#include "SolidSyslogPosixHostname.h"

#include <unistd.h>

#include "SolidSyslogFormatter.h"

struct SolidSyslogFormatter;

enum
{
    MAX_HOSTNAME_SIZE = 256
};

void SolidSyslogPosixHostname_Get(struct SolidSyslogFormatter* formatter)
{
    char hostname[MAX_HOSTNAME_SIZE];

    if (gethostname(hostname, sizeof(hostname)) == 0)
    {
        hostname[sizeof(hostname) - 1] = '\0';
        SolidSyslogFormatter_PrintUsAsciiString(formatter, hostname, sizeof(hostname));
    }
}
