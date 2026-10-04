/* A test stand-in for the VxWorks 6.4 time header.
 *
 * Supplies the subset of time.h that Platform/VxWorks64 calls, with the
 * prototypes of the public API reference.
 *
 * Unlike the other stand-ins, it renames each function with a macro. A host
 * has its own time.h and its own clock_gettime, which the test framework and
 * the sanitizer runtimes call; substituting the function at link time, as the
 * socket calls are, would hand them the fake too. The rename keeps the fake to
 * the sources compiled against this header, and the pack itself compiles
 * against the real header unchanged.
 *
 * It shadows the host's time.h, so it is on the include path of those C
 * sources only, never a test executable's.
 */
#ifndef TIME_H
#define TIME_H

/* NOLINTBEGIN(cppcoreguidelines-macro-to-enum,modernize-macro-to-enum) */
#define CLOCK_REALTIME 0x0
/* NOLINTEND(cppcoreguidelines-macro-to-enum,modernize-macro-to-enum) */

#define clock_gettime VxWorks64ClockFake_ClockGettime
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
/* Returns OK or ERROR, not a pointer as a hosted C library's does. */
int gmtime_r(const time_t* tod, struct tm* result);

#endif /* TIME_H */
