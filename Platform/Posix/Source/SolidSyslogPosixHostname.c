#include "SolidSyslogPosixHostname.h"

#include <unistd.h>

#include "SolidSyslogHeaderField.h"

struct SolidSyslogHeaderField;

enum
{
    MAX_HOSTNAME_SIZE = 256U
};

void SolidSyslogPosixHostname_Get(struct SolidSyslogHeaderField* field, void* context)
{
    char hostname[MAX_HOSTNAME_SIZE];

    (void) context;

    if (gethostname(hostname, sizeof(hostname)) == 0)
    {
        hostname[sizeof(hostname) - 1U] = '\0';
        SolidSyslogHeaderField_PrintUsAscii(field, hostname, sizeof(hostname));
    }
}
