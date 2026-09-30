#include "VxWorks64SemFake.h"

#include <stddef.h>

#include "semLib.h"

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
    return NULL;
}
