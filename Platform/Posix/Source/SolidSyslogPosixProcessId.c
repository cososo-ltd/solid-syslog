#include "SolidSyslogPosixProcessId.h"

#include <unistd.h>
#include <stdint.h>

#include "SolidSyslogHeaderField.h"

struct SolidSyslogHeaderField;

void SolidSyslogPosix_GetProcessId(struct SolidSyslogHeaderField* field, void* context)
{
    (void) context;
    SolidSyslogHeaderField_Uint32(field, (uint32_t) getpid());
}
