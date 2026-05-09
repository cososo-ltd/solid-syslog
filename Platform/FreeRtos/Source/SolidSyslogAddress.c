#include "SolidSyslogAddress.h"

struct SolidSyslogAddress* SolidSyslogAddress_FromStorage(SolidSyslogAddressStorage* storage)
{
    return (struct SolidSyslogAddress*) storage;
}
