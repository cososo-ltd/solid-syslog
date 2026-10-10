/* A test stand-in for the VxWorks 6.4 time header.
 *
 * Supplies the subset of time.h that Platform/VxWorks64 calls, with the
 * prototypes of the public API reference. It shadows the host's time.h, and
 * renames each function to the clock fake's, as README.md in this directory
 * describes.
 */
#ifndef TIME_H
#define TIME_H

/* NOLINTBEGIN(cppcoreguidelines-macro-to-enum,modernize-macro-to-enum) */
#define CLOCK_REALTIME 0x0
/* NOLINTEND(cppcoreguidelines-macro-to-enum,modernize-macro-to-enum) */

#define clock_gettime VxWorks64ClockFake_ClockGettime
#define clock_settime VxWorks64ClockFake_ClockSettime
#define gmtime_r VxWorks64ClockFake_GmtimeR

typedef int clockid_t;
typedef unsigned long time_t;

struct timespec
{
    time_t tv_sec;
    long tv_nsec;
};

struct tm
{
    int tm_sec;
    int tm_min;
    int tm_hour;
    int tm_mday;
    int tm_mon;
    int tm_year;
    int tm_wday;
    int tm_yday;
    int tm_isdst;
};

int clock_gettime(clockid_t clockId, struct timespec* tp);
int clock_settime(clockid_t clockId, const struct timespec* tp);
/* Returns OK or ERROR, not a pointer as a hosted C library's does. */
int gmtime_r(const time_t* tod, struct tm* result);

#endif /* TIME_H */
