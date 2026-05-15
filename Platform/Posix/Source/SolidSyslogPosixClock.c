#include "SolidSyslogPosixClock.h"

#include <stdbool.h>
#include <time.h>
#include <stdint.h>

#include "SolidSyslogTimestamp.h"

struct timespec;
struct tm;

static inline bool PosixClock_GetBrokenDownTime(struct timespec* now, struct tm* breakdown);
static inline void PosixClock_PopulateTimestamp(
    struct SolidSyslogTimestamp* timestamp,
    const struct timespec* now,
    const struct tm* breakdown
);

void SolidSyslogPosixClock_GetTimestamp(struct SolidSyslogTimestamp* timestamp)
{
    struct timespec now;
    struct tm breakdown;

    *timestamp = (struct SolidSyslogTimestamp) {0};

    if (PosixClock_GetBrokenDownTime(&now, &breakdown))
    {
        PosixClock_PopulateTimestamp(timestamp, &now, &breakdown);
    }
}

static inline bool PosixClock_GetBrokenDownTime(struct timespec* now, struct tm* breakdown)
{
    return (clock_gettime(CLOCK_REALTIME, now) == 0) && (gmtime_r(&now->tv_sec, breakdown) != NULL);
}

static inline void PosixClock_PopulateTimestamp(
    struct SolidSyslogTimestamp* timestamp,
    const struct timespec* now,
    const struct tm* breakdown
)
{
    timestamp->Year = (uint16_t) (breakdown->tm_year + 1900);
    timestamp->Month = (uint8_t) (breakdown->tm_mon + 1);
    timestamp->Day = (uint8_t) breakdown->tm_mday;
    timestamp->Hour = (uint8_t) breakdown->tm_hour;
    timestamp->Minute = (uint8_t) breakdown->tm_min;
    timestamp->Second = (uint8_t) breakdown->tm_sec;
    timestamp->Microsecond = (uint32_t) (now->tv_nsec / 1000);
}
