#include "VxWorks64SemFake.h"

#include <stddef.h>

#include "semLib.h"

static unsigned VxWorks64SemFake_MCreateCount = 0U;

void VxWorks64SemFake_Reset(void)
{
    VxWorks64SemFake_MCreateCount = 0U;
}

unsigned VxWorks64SemFake_SemMCreateCallCount(void)
{
    return VxWorks64SemFake_MCreateCount;
}

SEM_ID semMCreate(int options)
{
    (void) options;
    VxWorks64SemFake_MCreateCount++;
    return NULL;
}
