#include "vxWorks.h"

#include "VxWorks64TaskFake.h"

#include "errnoLib.h"
#include "routeLib.h"
#include "sysLib.h"
#include "taskLib.h"

static unsigned VxWorks64TaskFake_Spawns = 0U;

void VxWorks64TaskFake_Reset(void)
{
    VxWorks64TaskFake_Spawns = 0U;
}

unsigned VxWorks64TaskFake_SpawnCount(void)
{
    return VxWorks64TaskFake_Spawns;
}

/* Records the spawn and runs nothing: a test calls a task's entry itself when
 * it wants the task's behaviour. */
int taskSpawn(
    char* name,
    int priority,
    int options,
    int stackSize,
    FUNCPTR entryPt,
    int arg1,
    int arg2,
    int arg3,
    int arg4,
    int arg5,
    int arg6,
    int arg7,
    int arg8,
    int arg9,
    int arg10
)
{
    (void) name;
    (void) priority;
    (void) options;
    (void) stackSize;
    (void) entryPt;
    (void) arg1;
    (void) arg2;
    (void) arg3;
    (void) arg4;
    (void) arg5;
    (void) arg6;
    (void) arg7;
    (void) arg8;
    (void) arg9;
    (void) arg10;
    VxWorks64TaskFake_Spawns++;
    return (int) VxWorks64TaskFake_Spawns;
}

STATUS taskDelay(int ticks)
{
    (void) ticks;
    return OK;
}

int sysClkRateGet(void)
{
    return 60;
}

STATUS routeAdd(char* destination, char* gateway)
{
    (void) destination;
    (void) gateway;
    return OK;
}

int errnoGet(void)
{
    return 0;
}
