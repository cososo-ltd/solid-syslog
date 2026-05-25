#include "SolidSyslogFreeRtosAddressPrivate.h"

#include <string.h>

struct SolidSyslogAddress;

void FreeRtosAddress_Initialise(struct SolidSyslogAddress* base)
{
    struct SolidSyslogFreeRtosAddress* self = (struct SolidSyslogFreeRtosAddress*) base;
    (void) memset(&self->Sockaddr, 0, sizeof(self->Sockaddr));
}

void FreeRtosAddress_Cleanup(struct SolidSyslogAddress* base)
{
    (void) base;
}
