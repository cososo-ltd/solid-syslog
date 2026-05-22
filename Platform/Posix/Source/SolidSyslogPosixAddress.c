#include "SolidSyslogPosixAddressPrivate.h"

#include <netinet/in.h>
#include <string.h>

struct SolidSyslogAddress;

void PosixAddress_Initialise(struct SolidSyslogAddress* base)
{
    struct SolidSyslogPosixAddress* self = (struct SolidSyslogPosixAddress*) base;
    memset(&self->Sockaddr, 0, sizeof(self->Sockaddr));
}

void PosixAddress_Cleanup(struct SolidSyslogAddress* base)
{
    (void) base;
}
