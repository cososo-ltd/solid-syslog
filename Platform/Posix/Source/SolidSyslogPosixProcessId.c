#include "SolidSyslogPosixProcessId.h"

#include <unistd.h>
#include <stdint.h>

#include "SolidSyslogFormatter.h"

struct SolidSyslogFormatter;

void SolidSyslogPosixProcessId_Get(struct SolidSyslogFormatter* formatter)
{
    SolidSyslogFormatter_Uint32(formatter, (uint32_t) getpid());
}
