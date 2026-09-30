#include "vxWorks.h"

#include "VxWorks64NetFake.h"

#include "inetLib.h"

static unsigned long VxWorks64NetFake_InetAddrReturn = 0UL;

void VxWorks64NetFake_Reset(void)
{
    VxWorks64NetFake_InetAddrReturn = 0UL;
}

void VxWorks64NetFake_SetInetAddrReturn(unsigned long value)
{
    VxWorks64NetFake_InetAddrReturn = value;
}

unsigned long inet_addr(char* inetString)
{
    (void) inetString;
    return VxWorks64NetFake_InetAddrReturn;
}
