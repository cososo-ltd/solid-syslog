#include "BddTargetTickedClock.h"

#include <stddef.h>
#include <string.h>

enum
{
    SECONDS_PER_MINUTE = 60,
    MINUTES_PER_HOUR = 60,
    HOURS_PER_DAY = 24,
    SECONDS_PER_DAY = 86400,
    LAST_MICROSECOND = 999999,

    /* The civil calendar from a count of days, after Howard Hinnant's
     * days_from_civil: years are taken to start on 1 March, so a leap day is
     * the last day of its year, and the calendar repeats every 400 of them. */
    DAYS_FROM_MARCH_0000_TO_EPOCH = 719468,
    DAYS_PER_ERA = 146097,
    YEARS_PER_ERA = 400,
    DAYS_PER_YEAR = 365,
    DAYS_PER_FOUR_YEARS = 1460,
    DAYS_PER_CENTURY = 36524,
    LAST_DAY_OF_ERA = 146096,
    DAYS_PER_FIVE_MONTHS = 153,
    MONTHS_FROM_MARCH_TO_JANUARY = 10,
    MARCH = 3,
    FEBRUARY = 2,
    MONTHS_PER_YEAR = 12
};

static uint32_t TickedClock_NoCounter(void);
static inline void TickedClock_Start(uint32_t secondsSinceEpoch, uint32_t counterAtItsStart);
static inline uint32_t TickedClock_MicrosecondsSince(uint32_t counterAtStart, uint32_t counterNow);
static inline void TickedClock_SetTimeOfDay(struct SolidSyslogTimestamp* timestamp, uint32_t secondsIntoDay);
static inline void TickedClock_SetDate(struct SolidSyslogTimestamp* timestamp, uint32_t daysSinceEpoch);

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
        TickedClock_SetTimeOfDay(timestamp, seconds % (uint32_t) SECONDS_PER_DAY);
        TickedClock_SetDate(timestamp, seconds / (uint32_t) SECONDS_PER_DAY);
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

static inline void TickedClock_SetTimeOfDay(struct SolidSyslogTimestamp* timestamp, uint32_t secondsIntoDay)
{
    uint32_t minutesIntoDay = secondsIntoDay / (uint32_t) SECONDS_PER_MINUTE;
    timestamp->Hour = (uint8_t) (minutesIntoDay / (uint32_t) MINUTES_PER_HOUR);
    timestamp->Minute = (uint8_t) (minutesIntoDay % (uint32_t) MINUTES_PER_HOUR);
    timestamp->Second = (uint8_t) (secondsIntoDay % (uint32_t) SECONDS_PER_MINUTE);
}

static inline void TickedClock_SetDate(struct SolidSyslogTimestamp* timestamp, uint32_t daysSinceEpoch)
{
    uint32_t days = daysSinceEpoch + (uint32_t) DAYS_FROM_MARCH_0000_TO_EPOCH;
    uint32_t era = days / (uint32_t) DAYS_PER_ERA;
    uint32_t dayOfEra = days % (uint32_t) DAYS_PER_ERA;
    uint32_t yearOfEra = (dayOfEra - (dayOfEra / (uint32_t) DAYS_PER_FOUR_YEARS) +
                          (dayOfEra / (uint32_t) DAYS_PER_CENTURY) - (dayOfEra / (uint32_t) LAST_DAY_OF_ERA)) /
                         (uint32_t) DAYS_PER_YEAR;
    uint32_t dayOfYear = dayOfEra - (((uint32_t) DAYS_PER_YEAR * yearOfEra) + (yearOfEra / 4U) - (yearOfEra / 100U));
    uint32_t monthFromMarch = ((5U * dayOfYear) + 2U) / (uint32_t) DAYS_PER_FIVE_MONTHS;
    uint32_t year = yearOfEra + (era * (uint32_t) YEARS_PER_ERA);
    uint32_t month = monthFromMarch + (uint32_t) MARCH;
    if (monthFromMarch >= (uint32_t) MONTHS_FROM_MARCH_TO_JANUARY)
    {
        month -= (uint32_t) MONTHS_PER_YEAR;
    }
    /* January and February belong to the calendar year after the one their
     * March-first year started in. */
    if (month <= (uint32_t) FEBRUARY)
    {
        year++;
    }
    timestamp->Year = (uint16_t) year;
    timestamp->Month = (uint8_t) month;
    timestamp->Day = (uint8_t) (dayOfYear - ((((uint32_t) DAYS_PER_FIVE_MONTHS * monthFromMarch) + 2U) / 5U) + 1U);
}
