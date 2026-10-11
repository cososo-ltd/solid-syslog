#include "BddTargetServiceThread.h"
#include "SolidSyslog.h"

enum
{
    IDLE_YIELD_MILLISECONDS = 1U,
    READY_YIELD_MILLISECONDS = 0U
};

// cppcheck-suppress constParameter -- volatile bool written by another thread; const would be incorrect
// NOLINTNEXTLINE(readability-non-const-parameter) -- volatile bool written by another thread; const would be incorrect
void BddTargetServiceThread_Run(struct SolidSyslog* handle, volatile bool* shutdown, SolidSyslogSleepFunction sleep)
{
    while (!(*shutdown))
    {
        /* Consume the servicing status as a wakeup hint: when more work is ready now, loop
           without the idle delay; otherwise yield so an idle target does not spin. */
        const enum SolidSyslogServiceStatus status = SolidSyslog_Service(handle);
        const uint32_t yieldMilliseconds =
            (status == SOLIDSYSLOG_SERVICE_READY) ? READY_YIELD_MILLISECONDS : IDLE_YIELD_MILLISECONDS;
        sleep(yieldMilliseconds);
    }
}
