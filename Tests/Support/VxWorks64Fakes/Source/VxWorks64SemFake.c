#include "VxWorks64SemFake.h"

#include <stddef.h>
#include <stdint.h>

#include "semLib.h"

/* What semMCreate hands back when it succeeds: an address the adapter can only
 * store and pass back, standing in for the kernel's opaque semaphore. */
static uint8_t VxWorks64SemFake_Semaphore = 0U;

static unsigned VxWorks64SemFake_MCreateCount = 0U;
static int VxWorks64SemFake_MCreateOptions = 0;

void VxWorks64SemFake_Reset(void)
{
    VxWorks64SemFake_MCreateCount = 0U;
    VxWorks64SemFake_MCreateOptions = 0;
}

unsigned VxWorks64SemFake_SemMCreateCallCount(void)
{
    return VxWorks64SemFake_MCreateCount;
}

int VxWorks64SemFake_LastSemMCreateOptions(void)
{
    return VxWorks64SemFake_MCreateOptions;
}

SEM_ID semMCreate(int options)
{
    VxWorks64SemFake_MCreateOptions = options;
    VxWorks64SemFake_MCreateCount++;
    return (SEM_ID) &VxWorks64SemFake_Semaphore;
}
