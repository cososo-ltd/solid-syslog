#include "BddTargetTickedClock.h"

#include "BddTargetCalendar.h"

#include <stddef.h>
#include <string.h>

enum
{
    LAST_MICROSECOND = 999999
};

static uint32_t TickedClock_NoCounter(void);
static inline void TickedClock_Start(uint32_t secondsSinceEpoch, uint32_t counterAtItsStart);
static inline uint32_t TickedClock_MicrosecondsSince(uint32_t counterAtStart, uint32_t counterNow);

static BddTargetTickedClockCounterFunction clockCounter = TickedClock_NoCounter;
static uint32_t clockCountsPerMicrosecond = 1U;

/* The second under way and the count it started at, written together by
 * TickedClock_Start. The sequence counts those writes, so a reading that
 * finds it moved knows the two may not belong together and reads again. */
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

/* A target that wires no counter still has a clock, in whole seconds. */
static uint32_t TickedClock_NoCounter(void)
{
    return 0U;
}

void BddTargetTickedClock_Tick(uint32_t secondsSinceEpoch)
{
    TickedClock_Start(secondsSinceEpoch, clockCounter());
}

static inline void TickedClock_Start(uint32_t secondsSinceEpoch, uint32_t counterAtItsStart)
{
    clockSequence++;
    clockSeconds = secondsSinceEpoch;
    clockCounterAtSecond = counterAtItsStart;
    clockIsSet = true;
}

/* The counter wraps, and so does the subtraction: this is the count the
 * counter stood at, or would have, when the second started. */
void BddTargetTickedClock_Set(uint32_t secondsSinceEpoch, uint32_t microseconds)
{
    TickedClock_Start(secondsSinceEpoch, clockCounter() - (microseconds * clockCountsPerMicrosecond));
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
 * second's worth and the tick that starts the next has not arrived. */
static inline uint32_t TickedClock_MicrosecondsSince(uint32_t counterAtStart, uint32_t counterNow)
{
    uint32_t microseconds = (counterNow - counterAtStart) / clockCountsPerMicrosecond;
    if (microseconds > (uint32_t) LAST_MICROSECOND)
    {
        microseconds = (uint32_t) LAST_MICROSECOND;
    }
    return microseconds;
}
