#include "BddTargetCalendar.h"

enum
{
    SECONDS_PER_MINUTE = 60,
    MINUTES_PER_HOUR = 60,
    SECONDS_PER_HOUR = 3600,
    SECONDS_PER_DAY = 86400,

    /* After Howard Hinnant's civil_from_days and days_from_civil: years are
     * taken to start on 1 March, so a leap day is the last day of its year,
     * and the calendar repeats every 400 years. */
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

static inline void Calendar_SetTimeOfDay(struct SolidSyslogTimestamp* timestamp, uint32_t secondsIntoDay);
static inline void Calendar_SetDate(struct SolidSyslogTimestamp* timestamp, uint32_t daysSinceEpoch);
static inline uint32_t Calendar_DaysSinceEpoch(const struct SolidSyslogTimestamp* timestamp);
static inline uint32_t Calendar_DaysBeforeMonth(uint32_t monthFromMarch);

void BddTargetCalendar_FromEpoch(uint32_t secondsSinceEpoch, struct SolidSyslogTimestamp* timestamp)
{
    Calendar_SetTimeOfDay(timestamp, secondsSinceEpoch % (uint32_t) SECONDS_PER_DAY);
    Calendar_SetDate(timestamp, secondsSinceEpoch / (uint32_t) SECONDS_PER_DAY);
    timestamp->Microsecond = 0U;
    timestamp->UtcOffsetMinutes = 0;
}

static inline void Calendar_SetTimeOfDay(struct SolidSyslogTimestamp* timestamp, uint32_t secondsIntoDay)
{
    uint32_t minutesIntoDay = secondsIntoDay / (uint32_t) SECONDS_PER_MINUTE;
    timestamp->Hour = (uint8_t) (minutesIntoDay / (uint32_t) MINUTES_PER_HOUR);
    timestamp->Minute = (uint8_t) (minutesIntoDay % (uint32_t) MINUTES_PER_HOUR);
    timestamp->Second = (uint8_t) (secondsIntoDay % (uint32_t) SECONDS_PER_MINUTE);
}

static inline void Calendar_SetDate(struct SolidSyslogTimestamp* timestamp, uint32_t daysSinceEpoch)
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
    /* January and February fall in the calendar year after the one their
     * March-first year started in. */
    if (month <= (uint32_t) FEBRUARY)
    {
        year++;
    }
    timestamp->Year = (uint16_t) year;
    timestamp->Month = (uint8_t) month;
    timestamp->Day = (uint8_t) (dayOfYear - Calendar_DaysBeforeMonth(monthFromMarch) + 1U);
}

uint32_t BddTargetCalendar_ToEpoch(const struct SolidSyslogTimestamp* timestamp)
{
    uint32_t secondsIntoDay = ((uint32_t) timestamp->Hour * (uint32_t) SECONDS_PER_HOUR) +
                              ((uint32_t) timestamp->Minute * (uint32_t) SECONDS_PER_MINUTE) +
                              (uint32_t) timestamp->Second;
    return (Calendar_DaysSinceEpoch(timestamp) * (uint32_t) SECONDS_PER_DAY) + secondsIntoDay;
}

static inline uint32_t Calendar_DaysSinceEpoch(const struct SolidSyslogTimestamp* timestamp)
{
    uint32_t year = timestamp->Year;
    uint32_t monthFromMarch = (uint32_t) timestamp->Month + (uint32_t) MONTHS_PER_YEAR - (uint32_t) MARCH;
    if (timestamp->Month > (uint8_t) FEBRUARY)
    {
        monthFromMarch -= (uint32_t) MONTHS_PER_YEAR;
    }
    else
    {
        /* January and February end the previous March-first year. */
        year--;
    }
    uint32_t era = year / (uint32_t) YEARS_PER_ERA;
    uint32_t yearOfEra = year % (uint32_t) YEARS_PER_ERA;
    uint32_t dayOfYear = Calendar_DaysBeforeMonth(monthFromMarch) + (uint32_t) timestamp->Day - 1U;
    uint32_t dayOfEra = (yearOfEra * (uint32_t) DAYS_PER_YEAR) + (yearOfEra / 4U) - (yearOfEra / 100U) + dayOfYear;
    return (era * (uint32_t) DAYS_PER_ERA) + dayOfEra - (uint32_t) DAYS_FROM_MARCH_0000_TO_EPOCH;
}

/* The days in a March-first year before the given month, with March as 0.
 * From March the month lengths repeat 31, 30, 31, 30, 31, which is what the
 * division encodes. */
static inline uint32_t Calendar_DaysBeforeMonth(uint32_t monthFromMarch)
{
    return (((uint32_t) DAYS_PER_FIVE_MONTHS * monthFromMarch) + 2U) / 5U;
}
