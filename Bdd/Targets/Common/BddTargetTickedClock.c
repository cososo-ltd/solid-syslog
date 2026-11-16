#include "BddTargetTickedClock.h"

#include "BddTargetCalendar.h"

#include <stddef.h>
#include <string.h>

enum
{
    LAST_MICROSECOND = 999999
};

static uint32_t TickedClock_NoCounter(void);
static inline uint32_t TickedClock_MicrosecondsSince(uint32_t counterAtStart, uint32_t counterNow);

static BddTargetTickedClockCounterFunction clockCounter = TickedClock_NoCounter;
static uint32_t clockCountsPerMicrosecond = 1U;

/* The current second and the count it started at, written together by
 * BddTargetTickedClock_Set. The sequence counts those writes, so a reading
 * that sees it change reads again. */
static volatile uint32_t clockSequence;
static volatile bool clockIsSet;
static volatile uint32_t clockSeconds;
static volatile uint32_t clockCounterAtSecond;

void BddTargetTickedClock_Initialise(BddTargetTickedClockCounterFunction counter, uint32_t countsPerMicrosecond)
{
    clockCounter = (counter != NULL) ? counter : TickedClock_NoCounter;
    clockCountsPerMicrosecond = (countsPerMicrosecond != 0U) ? countsPerMicrosecond : 1U;
    clockSequence++;
    clockIsSet = false;
}

/* With no counter the clock still works, in whole seconds. */
static uint32_t TickedClock_NoCounter(void)
{
    return 0U;
}

void BddTargetTickedClock_Tick(uint32_t secondsSinceEpoch)
{
    BddTargetTickedClock_Set(secondsSinceEpoch, 0U);
}

/* The subtraction wraps as the counter does, giving the count at which the
 * second started. The two parameters are one time, seconds then fraction, in
 * the order a timeval keeps them. */
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
void BddTargetTickedClock_Set(uint32_t secondsSinceEpoch, uint32_t microseconds)
{
    uint32_t counterAtItsStart = clockCounter() - (microseconds * clockCountsPerMicrosecond);
    clockSequence++;
    clockSeconds = secondsSinceEpoch;
    clockCounterAtSecond = counterAtItsStart;
    clockIsSet = true;
}

bool BddTargetTickedClock_IsSet(void)
{
    return clockIsSet;
}

void BddTargetTickedClock_GetTimestamp(struct SolidSyslogTimestamp* timestamp)
{
    uint32_t sequence = 0U;
    bool isSet = false;
    uint32_t seconds = 0U;
    uint32_t microseconds = 0U;
    do
    {
        sequence = clockSequence;
        isSet = clockIsSet;
        seconds = clockSeconds;
        microseconds = TickedClock_MicrosecondsSince(clockCounterAtSecond, clockCounter());
    } while (sequence != clockSequence);

    memset(timestamp, 0, sizeof(*timestamp));
    if (isSet)
    {
        BddTargetCalendar_FromEpoch(seconds, timestamp);
        timestamp->Microsecond = microseconds;
    }
}

/* Held at the last microsecond of the second when the counter has counted a
 * second's worth and the next tick has not arrived. */
static inline uint32_t TickedClock_MicrosecondsSince(uint32_t counterAtStart, uint32_t counterNow)
{
    uint32_t microseconds = (counterNow - counterAtStart) / clockCountsPerMicrosecond;
    if (microseconds > (uint32_t) LAST_MICROSECOND)
    {
        microseconds = (uint32_t) LAST_MICROSECOND;
    }
    return microseconds;
}
