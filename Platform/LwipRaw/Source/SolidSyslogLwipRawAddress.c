#include "SolidSyslogLwipRawAddressPrivate.h"

#include <string.h>

struct SolidSyslogAddress;

void LwipRawAddress_Initialise(struct SolidSyslogAddress* base)
{
    struct SolidSyslogLwipRawAddress* self = SolidSyslogLwipRawAddress_As(base);
    (void) memset(self, 0, sizeof *self);
}

void LwipRawAddress_Cleanup(struct SolidSyslogAddress* base)
{
    (void) base;
}
