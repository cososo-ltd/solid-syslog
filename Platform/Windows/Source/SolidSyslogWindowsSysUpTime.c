#include "SolidSyslogWindowsSysUpTime.h"
#include "SolidSyslogWindowsSysUpTimeInternal.h"

/* File-local forwarder. Taking the address of an imported Windows API
   for static initialisation may trigger MSVC C4232 in some configurations;
   forwarding through a static function whose address IS a compile-time
   constant avoids the warning without a suppression. */
static ULONGLONG WINAPI CallGetTickCount64(void);

WindowsGetTickCount64Fn WindowsSysUpTime_GetTickCount64 = CallGetTickCount64;

static ULONGLONG WINAPI CallGetTickCount64(void)
{
    return GetTickCount64();
}

enum
{
    MILLISECONDS_PER_HUNDREDTH = 10
};

uint32_t SolidSyslogWindowsSysUpTime_Get(void)
{
    ULONGLONG milliseconds = WindowsSysUpTime_GetTickCount64();
    return (uint32_t) (milliseconds / MILLISECONDS_PER_HUNDREDTH);
}
