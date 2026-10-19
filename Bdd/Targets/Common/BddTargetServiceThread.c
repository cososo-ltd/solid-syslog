#include "BddTargetServiceThread.h"
#include "SolidSyslog.h"

enum
{
    IDLE_YIELD_MILLISECONDS = 1U,
    READY_YIELD_MILLISECONDS = 0U,
    BLOCKED_YIELD_MILLISECONDS = 20U
};

static uint32_t BddTargetServiceThread_YieldFor(enum SolidSyslogServiceStatus status);

// cppcheck-suppress constParameter -- volatile bool written by another thread; const would be incorrect
// NOLINTNEXTLINE(readability-non-const-parameter) -- volatile bool written by another thread; const would be incorrect
void BddTargetServiceThread_Run(struct SolidSyslog* handle, volatile bool* shutdown, SolidSyslogSleepFunction sleep)
{
    while (!(*shutdown))
    {
        /* Consume the servicing status as a wakeup hint: when more work is ready now, loop
           without the idle delay; back off while a record waits on the sender; otherwise
           yield so an idle target does not spin. */
        sleep(BddTargetServiceThread_YieldFor(SolidSyslog_Service(handle)));
    }
}

static uint32_t BddTargetServiceThread_YieldFor(enum SolidSyslogServiceStatus status)
{
    uint32_t milliseconds = IDLE_YIELD_MILLISECONDS;

    if (status == SOLIDSYSLOG_SERVICE_READY)
    {
        milliseconds = READY_YIELD_MILLISECONDS;
    }
    else if (status == SOLIDSYSLOG_SERVICE_BLOCKED)
    {
        milliseconds = BLOCKED_YIELD_MILLISECONDS;
    }
    else
    {
        /* IDLE and HALTED: the idle yield (initial value). */
    }

    return milliseconds;
}
