#include "SolidSyslogWinsockAddressPrivate.h"

#include <string.h>

struct SolidSyslogAddress;

void WinsockAddress_Initialise(struct SolidSyslogAddress* base)
{
    struct SolidSyslogWinsockAddress* self = (struct SolidSyslogWinsockAddress*) base;
    memset(&self->Sockaddr, 0, sizeof(self->Sockaddr));
}

void WinsockAddress_Cleanup(struct SolidSyslogAddress* base)
{
    (void) base;
}
