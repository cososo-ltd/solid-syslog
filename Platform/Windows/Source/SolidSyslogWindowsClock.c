#include "SolidSyslogWindowsClock.h"
#include "SolidSyslogWindowsClockInternal.h"

#include <stdbool.h>
#include <stdint.h>

/* File-local forwarder. Taking the address of an imported Windows API
   for static initialisation may trigger MSVC C4232 in some configurations;
   forwarding through a static function whose address IS a compile-time
   constant avoids the warning without a suppression. */
static void WINAPI WindowsClock_CallGetSystemTimeAsFileTime(LPFILETIME fileTime);

WindowsGetSystemTimeAsFileTimeFn WindowsClock_GetSystemTimeAsFileTime = WindowsClock_CallGetSystemTimeAsFileTime;

static void WINAPI WindowsClock_CallGetSystemTimeAsFileTime(LPFILETIME fileTime)
{
    GetSystemTimeAsFileTime(fileTime);
}

enum
{
    HUNDRED_NS_PER_MICROSECOND = 10,
    HUNDRED_NS_PER_SECOND = 10000000
};

static inline bool WindowsClock_BreakDownFileTime(const FILETIME* fileTime, SYSTEMTIME* breakdown);
static inline uint32_t WindowsClock_MicrosecondsFromFileTime(const FILETIME* fileTime);
static inline void WindowsClock_PopulateTimestamp(
    struct SolidSyslogTimestamp* timestamp,
    const SYSTEMTIME* breakdown,
    uint32_t microseconds
);

void SolidSyslogWindowsClock_GetTimestamp(struct SolidSyslogTimestamp* timestamp)
{
    FILETIME fileTime;
    SYSTEMTIME breakdown;

    *timestamp = (struct SolidSyslogTimestamp) {0};

    WindowsClock_GetSystemTimeAsFileTime(&fileTime);

    if (WindowsClock_BreakDownFileTime(&fileTime, &breakdown))
    {
        WindowsClock_PopulateTimestamp(timestamp, &breakdown, WindowsClock_MicrosecondsFromFileTime(&fileTime));
    }
}

static inline bool WindowsClock_BreakDownFileTime(const FILETIME* fileTime, SYSTEMTIME* breakdown)
{
    return FileTimeToSystemTime(fileTime, breakdown) != 0;
}

static inline uint32_t WindowsClock_MicrosecondsFromFileTime(const FILETIME* fileTime)
{
    uint64_t hundredNs = ((uint64_t) fileTime->dwHighDateTime << 32) | (uint64_t) fileTime->dwLowDateTime;
    return (uint32_t) ((hundredNs % HUNDRED_NS_PER_SECOND) / HUNDRED_NS_PER_MICROSECOND);
}

static inline void WindowsClock_PopulateTimestamp(
    struct SolidSyslogTimestamp* timestamp,
    const SYSTEMTIME* breakdown,
    uint32_t microseconds
)
{
    timestamp->Year = breakdown->wYear;
    timestamp->Month = (uint8_t) breakdown->wMonth;
    timestamp->Day = (uint8_t) breakdown->wDay;
    timestamp->Hour = (uint8_t) breakdown->wHour;
    timestamp->Minute = (uint8_t) breakdown->wMinute;
    timestamp->Second = (uint8_t) breakdown->wSecond;
    timestamp->Microsecond = microseconds;
}
