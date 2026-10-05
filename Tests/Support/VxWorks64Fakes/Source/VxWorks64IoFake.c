#include "vxWorks.h"

#include "VxWorks64IoFake.h"

#include "ioLib.h"

static unsigned VxWorks64IoFake_Closes = 0U;
static int VxWorks64IoFake_ClosedFd = -1;

void VxWorks64IoFake_Reset(void)
{
    VxWorks64IoFake_Closes = 0U;
    VxWorks64IoFake_ClosedFd = -1;
}

unsigned VxWorks64IoFake_CloseCallCount(void)
{
    return VxWorks64IoFake_Closes;
}

int VxWorks64IoFake_LastClosedFd(void)
{
    return VxWorks64IoFake_ClosedFd;
}

STATUS close(int fd)
{
    VxWorks64IoFake_Closes++;
    VxWorks64IoFake_ClosedFd = fd;
    return OK;
}
