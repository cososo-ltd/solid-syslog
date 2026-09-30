#include "vxWorks.h"

#include "VxWorks64NetFake.h"

#include <stddef.h>

#include "hostLib.h"
#include "inetLib.h"

static unsigned long VxWorks64NetFake_InetAddrReturn = 0UL;
static const char* VxWorks64NetFake_InetAddrString = NULL;
static int VxWorks64NetFake_HostGetByNameReturn = 0;
static unsigned VxWorks64NetFake_HostGetByNameCount = 0U;
static const char* VxWorks64NetFake_HostGetByNameName = NULL;

void VxWorks64NetFake_Reset(void)
{
    VxWorks64NetFake_InetAddrReturn = 0UL;
    VxWorks64NetFake_InetAddrString = NULL;
    VxWorks64NetFake_HostGetByNameReturn = 0;
    VxWorks64NetFake_HostGetByNameCount = 0U;
    VxWorks64NetFake_HostGetByNameName = NULL;
}

void VxWorks64NetFake_SetInetAddrReturn(unsigned long value)
{
    VxWorks64NetFake_InetAddrReturn = value;
}

const char* VxWorks64NetFake_LastInetAddrString(void)
{
    return VxWorks64NetFake_InetAddrString;
}

void VxWorks64NetFake_SetHostGetByNameReturn(int value)
{
    VxWorks64NetFake_HostGetByNameReturn = value;
}

unsigned VxWorks64NetFake_HostGetByNameCallCount(void)
{
    return VxWorks64NetFake_HostGetByNameCount;
}

const char* VxWorks64NetFake_LastHostGetByNameName(void)
{
    return VxWorks64NetFake_HostGetByNameName;
}

unsigned long inet_addr(char* inetString)
{
    VxWorks64NetFake_InetAddrString = inetString;
    return VxWorks64NetFake_InetAddrReturn;
}

int hostGetByName(char* name)
{
    VxWorks64NetFake_HostGetByNameCount++;
    VxWorks64NetFake_HostGetByNameName = name;
    return VxWorks64NetFake_HostGetByNameReturn;
}
