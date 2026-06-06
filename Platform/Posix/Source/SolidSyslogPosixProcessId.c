#include "SolidSyslogPosixProcessId.h"

#include <unistd.h>
#include <stdint.h>

#include "SolidSyslogHeaderField.h"

struct SolidSyslogHeaderField;

void SolidSyslogPosixProcessId_Get(struct SolidSyslogHeaderField* field, void* context)
{
    (void) context;
    SolidSyslogHeaderField_Uint32(field, (uint32_t) getpid());
}
