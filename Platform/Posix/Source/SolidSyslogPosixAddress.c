#include "SolidSyslogError.h"
#include "SolidSyslogPosixAddressErrors.h"
#include "SolidSyslogPosixAddressPrivate.h"

#include <netinet/in.h>
#include <string.h>

const struct SolidSyslogErrorSource PosixAddressErrorSource = {"PosixAddress"};

struct SolidSyslogAddress;

void PosixAddress_Initialise(struct SolidSyslogAddress* base)
{
    struct SolidSyslogPosixAddress* self = (struct SolidSyslogPosixAddress*) base;
    (void) memset(&self->Sockaddr, 0, sizeof(self->Sockaddr));
}

void PosixAddress_Cleanup(struct SolidSyslogAddress* base)
{
    (void) base;
}
