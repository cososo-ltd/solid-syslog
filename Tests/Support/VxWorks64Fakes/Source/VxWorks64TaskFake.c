#include "vxWorks.h"

#include "VxWorks64TaskFake.h"

#include "errnoLib.h"
#include "routeLib.h"
#include "sysLib.h"
#include "taskLib.h"

static unsigned VxWorks64TaskFake_Spawns = 0U;
static unsigned VxWorks64TaskFake_Delays = 0U;
static int VxWorks64TaskFake_DelayTicks = 0;

void VxWorks64TaskFake_Reset(void)
{
    VxWorks64TaskFake_Spawns = 0U;
    VxWorks64TaskFake_Delays = 0U;
    VxWorks64TaskFake_DelayTicks = 0;
}

unsigned VxWorks64TaskFake_SpawnCount(void)
{
    return VxWorks64TaskFake_Spawns;
}

unsigned VxWorks64TaskFake_DelayCount(void)
{
    return VxWorks64TaskFake_Delays;
}

int VxWorks64TaskFake_LastDelayTicks(void)
{
    return VxWorks64TaskFake_DelayTicks;
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
    VxWorks64TaskFake_Delays++;
    VxWorks64TaskFake_DelayTicks = ticks;
    return OK;
}

/* The VxWorks default system clock rate. */
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
