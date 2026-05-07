#include "SolidSyslogPosixSleep.h"

#include <time.h>

enum
{
    MILLISECONDS_PER_SECOND     = 1000,
    NANOSECONDS_PER_MILLISECOND = 1000000L
};

void SolidSyslogPosixSleep(int milliseconds)
{
    struct timespec ts = {.tv_sec  = milliseconds / MILLISECONDS_PER_SECOND,
                          .tv_nsec = (long) (milliseconds % MILLISECONDS_PER_SECOND) * NANOSECONDS_PER_MILLISECOND};
    (void) nanosleep(&ts, NULL);
}
