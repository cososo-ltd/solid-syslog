#include "vxWorks.h"

#include "VxWorks64ArpFake.h"

#include "arpLib.h"

#include <errno.h>
#include <string.h>

enum
{
    TARGET_CAPACITY = 32
};

static unsigned VxWorks64ArpFake_Count = 0U;
static int VxWorks64ArpFake_Errno = 0;
static char VxWorks64ArpFake_Target[TARGET_CAPACITY];
static int VxWorks64ArpFake_NumTries = 0;
static int VxWorks64ArpFake_NumTicks = 0;

void VxWorks64ArpFake_Reset(void)
{
    VxWorks64ArpFake_Count = 0U;
    VxWorks64ArpFake_Errno = 0;
    memset(VxWorks64ArpFake_Target, 0, sizeof(VxWorks64ArpFake_Target));
    VxWorks64ArpFake_NumTries = 0;
    VxWorks64ArpFake_NumTicks = 0;
}

void VxWorks64ArpFake_FailWithErrno(int errnoValue)
{
    VxWorks64ArpFake_Errno = errnoValue;
}

unsigned VxWorks64ArpFake_ArpResolveCallCount(void)
{
    return VxWorks64ArpFake_Count;
}

const char* VxWorks64ArpFake_LastTarget(void)
{
    return VxWorks64ArpFake_Target;
}

int VxWorks64ArpFake_LastNumTries(void)
{
    return VxWorks64ArpFake_NumTries;
}

int VxWorks64ArpFake_LastNumTicks(void)
{
    return VxWorks64ArpFake_NumTicks;
}

// NOLINTNEXTLINE(readability-non-const-parameter) -- signature fixed by the VxWorks API
STATUS arpResolve(char* targetAddr, char* pHwAddr, int numTries, int numTicks)
{
    STATUS status = OK;
    size_t length = strlen(targetAddr);
    size_t copied = (length < sizeof(VxWorks64ArpFake_Target)) ? length : sizeof(VxWorks64ArpFake_Target) - 1U;
    VxWorks64ArpFake_Count++;
    (void) memcpy(VxWorks64ArpFake_Target, targetAddr, copied);
    VxWorks64ArpFake_Target[copied] = '\0';
    VxWorks64ArpFake_NumTries = numTries;
    VxWorks64ArpFake_NumTicks = numTicks;
    if (VxWorks64ArpFake_Errno != 0)
    {
        errno = VxWorks64ArpFake_Errno;
        status = ERROR;
    }
    else
    {
        (void) memset(pHwAddr, 0x02, 6U);
    }
    return status;
}
