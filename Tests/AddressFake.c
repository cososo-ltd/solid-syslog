#include "AddressFake.h"

struct SolidSyslogAddress;

static char instance;

struct SolidSyslogAddress* AddressFake_Get(void)
{
    return (struct SolidSyslogAddress*) &instance;
}
