#include "BddTargetServiceThread.h"
#include "SolidSyslog.h"

enum
{
    IDLE_YIELD_MILLISECONDS = 1
};

// cppcheck-suppress constParameter -- volatile bool written by another thread; const would be incorrect
// NOLINTNEXTLINE(readability-non-const-parameter) -- volatile bool written by another thread; const would be incorrect
void BddTargetServiceThread_Run(struct SolidSyslog* handle, volatile bool* shutdown, SolidSyslogSleepFunction sleep)
{
    while (!(*shutdown))
    {
        SolidSyslog_Service(handle);
        sleep(IDLE_YIELD_MILLISECONDS);
    }
}
