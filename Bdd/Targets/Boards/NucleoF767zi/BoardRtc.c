/* The part's real-time clock - see BoardRtc.h.
 *
 * Cross build only, so not host-tested: the target checklist in README.md
 * covers it on the board. The calendar arithmetic is BddTargetCalendar's,
 * which is tested on the host. */

#include "BoardRtc.h"

#include "BddTargetCalendar.h"
#include "SolidSyslogTimestamp.h"

#include "stm32f7xx_hal.h"

#include <stddef.h>

enum
{
    /* From cold the crystal takes a second or two to start. */
    CRYSTAL_START_MILLISECONDS = 5000,

    /* The clock's reset prescalers divide 32768 Hz by 128 and then by 256,
     * giving one second, and are left as they are. The sub-second register
     * counts the 256 down, from 255 to zero. */
    SUBSECONDS_PER_SECOND = 256,
    SUBSECOND_AT_SECOND_START = SUBSECONDS_PER_SECOND - 1,
    MICROSECONDS_PER_SECOND = 1000000,

    /* The keys that unlock the clock's registers, and the value that locks
     * them again. */
    WRITE_KEY_FIRST = 0xCA,
    WRITE_KEY_SECOND = 0x53,
    WRITE_LOCK = 0xFF,

    /* The clock answers a request within a few cycles of its 32768 Hz, some
     * tens of microseconds. This loop count is far longer, and being a count
     * and not a timeout it works with interrupts off. */
    CLOCK_WAIT_LOOPS = 1000000,

    /* Above the kernel's interrupt ceiling, so a kernel critical section does
     * not delay the second's interrupt. The handler makes no kernel call. */
    SECOND_INTERRUPT_PRIORITY = 4,

    /* The clock keeps two digits of year. 2000 is excluded because a year of
     * 00 is how the clock shows it has never been set. */
    CALENDAR_CENTURY = 2000,
    BCD_TENS = 10,
    BCD_TENS_SHIFT = 4,
    BCD_UNITS_MASK = 0xF,

    /* 1970-01-01 was a Thursday, and the clock counts Monday as 1. */
    SECONDS_PER_DAY = 86400,
    DAYS_PER_WEEK = 7,
    EPOCH_WEEKDAY_FROM_MONDAY = 3
};

/* The clock treats every fourth year as a leap year, year 00 included, which
 * is right only in a century that begins with one. */
_Static_assert(((uint32_t) CALENDAR_CENTURY % 400U) == 0U, "the clock's year 00 has to be a leap year");

/* 2001-01-01T00:00:00Z and 2099-12-31T23:59:59Z. */
static const uint32_t FIRST_SECOND_KEPT = 978307200U;
static const uint32_t LAST_SECOND_KEPT = 4102444799U;

static bool rtcIsRunning;
static BoardRtcSecondFunction rtcOnSecond;

/* The name in ST's vector table. */
void RTC_Alarm_IRQHandler(void);

static inline bool BoardRtc_OpenBackupDomain(void);
static inline bool BoardRtc_StartCrystal(void);
static inline bool BoardRtc_ClockFromCrystal(void);
static inline bool BoardRtc_InterruptEverySecond(void);
static inline void BoardRtc_Unlock(void);
static inline void BoardRtc_Lock(void);
static inline bool BoardRtc_WaitFor(uint32_t flag);
static inline void BoardRtc_ClearSecondFlag(void);
static inline uint32_t BoardRtc_Seconds(uint32_t time, uint32_t date);
static inline uint8_t BoardRtc_FromBcd(uint32_t bcd);
static inline uint32_t BoardRtc_ToBcd(uint32_t value);
static inline bool BoardRtc_IsKept(uint32_t secondsSinceEpoch);
static inline uint32_t BoardRtc_TimeRegister(const struct SolidSyslogTimestamp* calendar);
static inline uint32_t BoardRtc_DateRegister(const struct SolidSyslogTimestamp* calendar, uint32_t secondsSinceEpoch);

bool BoardRtc_Start(BoardRtcSecondFunction onSecond)
{
    rtcOnSecond = onSecond;
    rtcIsRunning = BoardRtc_OpenBackupDomain() && BoardRtc_StartCrystal() && BoardRtc_ClockFromCrystal() &&
                   BoardRtc_InterruptEverySecond();
    return rtcIsRunning;
}

/* The clock and its crystal are in the backup domain, which is write-protected
 * until the power controller's DBP bit is set and reads back as set. */
static inline bool BoardRtc_OpenBackupDomain(void)
{
    __HAL_RCC_PWR_CLK_ENABLE();
    SET_BIT(PWR->CR1, PWR_CR1_DBP);
    for (uint32_t i = 0U; (READ_BIT(PWR->CR1, PWR_CR1_DBP) == 0U) && (i < (uint32_t) CLOCK_WAIT_LOOPS); i++)
    {
    }
    return READ_BIT(PWR->CR1, PWR_CR1_DBP) != 0U;
}

/* A reset leaves the crystal running, so this waits only after power-up. */
static inline bool BoardRtc_StartCrystal(void)
{
    uint32_t started = HAL_GetTick();
    SET_BIT(RCC->BDCR, RCC_BDCR_LSEON);
    while ((READ_BIT(RCC->BDCR, RCC_BDCR_LSERDY) == 0U) &&
           ((HAL_GetTick() - started) < (uint32_t) CRYSTAL_START_MILLISECONDS))
    {
    }
    return READ_BIT(RCC->BDCR, RCC_BDCR_LSERDY) != 0U;
}

/* The clock's source can be chosen only once after the backup domain is
 * reset, so it is chosen only if none has been, and a running clock is left
 * alone. False if other firmware run since power-up has put the clock on
 * another source, which cannot be undone here. */
static inline bool BoardRtc_ClockFromCrystal(void)
{
    if (READ_BIT(RCC->BDCR, RCC_BDCR_RTCSEL) == 0U)
    {
        SET_BIT(RCC->BDCR, RCC_BDCR_RTCSEL_0);
    }
    SET_BIT(RCC->BDCR, RCC_BDCR_RTCEN);
    return READ_BIT(RCC->BDCR, RCC_BDCR_RTCSEL) == RCC_BDCR_RTCSEL_0;
}

/* Alarm A with every field of its match masked matches every second, as the
 * second starts. It reaches the processor through line 17 of the external
 * interrupt controller, on a rising edge.
 *
 * A reset leaves the alarm running, with its flag set by any second that
 * started while nothing was handling it. The flag holds the line high, and a
 * line already high gives no edge, so the edge is enabled first and the flag
 * cleared before the alarm is switched on again.
 *
 * The calendar is read directly from its counters, not from the shadow
 * registers, which lag the counters by a cycle or two of the crystal: the
 * interrupt reads as the second starts, and needs that second. */
static inline bool BoardRtc_InterruptEverySecond(void)
{
    SET_BIT(EXTI->RTSR, EXTI_RTSR_TR17);
    SET_BIT(EXTI->IMR, EXTI_IMR_MR17);
    HAL_NVIC_SetPriority(RTC_Alarm_IRQn, (uint32_t) SECOND_INTERRUPT_PRIORITY, 0U);
    HAL_NVIC_EnableIRQ(RTC_Alarm_IRQn);

    BoardRtc_Unlock();
    SET_BIT(RTC->CR, RTC_CR_BYPSHAD);
    CLEAR_BIT(RTC->CR, RTC_CR_ALRAE | RTC_CR_ALRAIE);
    bool alarmIsSet = BoardRtc_WaitFor(RTC_ISR_ALRAWF);
    if (alarmIsSet)
    {
        RTC->ALRMAR = RTC_ALRMAR_MSK1 | RTC_ALRMAR_MSK2 | RTC_ALRMAR_MSK3 | RTC_ALRMAR_MSK4;
        RTC->ALRMASSR = 0U;
        BoardRtc_ClearSecondFlag();
        SET_BIT(RTC->CR, RTC_CR_ALRAE | RTC_CR_ALRAIE);
    }
    BoardRtc_Lock();
    return alarmIsSet;
}

static inline void BoardRtc_Unlock(void)
{
    RTC->WPR = (uint32_t) WRITE_KEY_FIRST;
    RTC->WPR = (uint32_t) WRITE_KEY_SECOND;
}

static inline void BoardRtc_Lock(void)
{
    RTC->WPR = (uint32_t) WRITE_LOCK;
}

static inline bool BoardRtc_WaitFor(uint32_t flag)
{
    for (uint32_t i = 0U; (READ_BIT(RTC->ISR, flag) == 0U) && (i < (uint32_t) CLOCK_WAIT_LOOPS); i++)
    {
    }
    return READ_BIT(RTC->ISR, flag) != 0U;
}

/* The alarm's flag is cleared by writing it 0, with every other flag written
 * 1 and the INIT request, which shares the register, written as it stands. */
static inline void BoardRtc_ClearSecondFlag(void)
{
    RTC->ISR = ~(RTC_ISR_ALRAF | RTC_ISR_INIT) | READ_BIT(RTC->ISR, RTC_ISR_INIT);
}

bool BoardRtc_HasTime(void)
{
    return rtcIsRunning && (READ_BIT(RTC->ISR, RTC_ISR_INITS) != 0U);
}

/* The counters are read while they count, so they are read until two readings
 * agree, and again if the sub-second count rose between its two readings: it
 * counts down, so a rise means a second started part way through. */
void BoardRtc_Read(uint32_t* secondsSinceEpoch, uint32_t* microseconds)
{
    uint32_t subsecondBefore = 0U;
    uint32_t subsecondAfter = 0U;
    uint32_t time = 0U;
    uint32_t date = 0U;
    do
    {
        subsecondBefore = READ_BIT(RTC->SSR, RTC_SSR_SS);
        time = RTC->TR;
        date = RTC->DR;
        subsecondAfter = READ_BIT(RTC->SSR, RTC_SSR_SS);
    } while ((time != RTC->TR) || (date != RTC->DR) || (subsecondAfter > subsecondBefore));

    uint32_t elapsed = 0U;
    if (subsecondAfter <= (uint32_t) SUBSECOND_AT_SECOND_START)
    {
        elapsed = (uint32_t) SUBSECOND_AT_SECOND_START - subsecondAfter;
    }
    *secondsSinceEpoch = BoardRtc_Seconds(time, date);
    *microseconds = (elapsed * (uint32_t) MICROSECONDS_PER_SECOND) / (uint32_t) SUBSECONDS_PER_SECOND;
}

static inline uint32_t BoardRtc_Seconds(uint32_t time, uint32_t date)
{
    struct SolidSyslogTimestamp calendar = {0};
    calendar.Year =
        (uint16_t) ((uint32_t) CALENDAR_CENTURY + BoardRtc_FromBcd((date & (RTC_DR_YT | RTC_DR_YU)) >> RTC_DR_YU_Pos));
    calendar.Month = BoardRtc_FromBcd((date & (RTC_DR_MT | RTC_DR_MU)) >> RTC_DR_MU_Pos);
    calendar.Day = BoardRtc_FromBcd((date & (RTC_DR_DT | RTC_DR_DU)) >> RTC_DR_DU_Pos);
    calendar.Hour = BoardRtc_FromBcd((time & (RTC_TR_HT | RTC_TR_HU)) >> RTC_TR_HU_Pos);
    calendar.Minute = BoardRtc_FromBcd((time & (RTC_TR_MNT | RTC_TR_MNU)) >> RTC_TR_MNU_Pos);
    calendar.Second = BoardRtc_FromBcd((time & (RTC_TR_ST | RTC_TR_SU)) >> RTC_TR_SU_Pos);
    return BddTargetCalendar_ToEpoch(&calendar);
}

static inline uint8_t BoardRtc_FromBcd(uint32_t bcd)
{
    return (uint8_t) (((bcd >> (uint32_t) BCD_TENS_SHIFT) * (uint32_t) BCD_TENS) + (bcd & (uint32_t) BCD_UNITS_MASK));
}

/* The calendar is written with the clock stopped, and the clock starts again
 * on the second written. */
bool BoardRtc_Set(uint32_t secondsSinceEpoch)
{
    bool taken = false;
    if (rtcIsRunning && BoardRtc_IsKept(secondsSinceEpoch))
    {
        struct SolidSyslogTimestamp calendar = {0};
        BddTargetCalendar_FromEpoch(secondsSinceEpoch, &calendar);
        BoardRtc_Unlock();
        /* All ones, so that no flag is cleared: a flag in this register is
         * cleared by writing it 0 and left alone by writing it 1. */
        RTC->ISR = ~0U;
        if (BoardRtc_WaitFor(RTC_ISR_INITF))
        {
            RTC->TR = BoardRtc_TimeRegister(&calendar);
            RTC->DR = BoardRtc_DateRegister(&calendar, secondsSinceEpoch);
            taken = true;
        }
        RTC->ISR = ~RTC_ISR_INIT;
        BoardRtc_Lock();
    }
    return taken;
}

static inline bool BoardRtc_IsKept(uint32_t secondsSinceEpoch)
{
    return (secondsSinceEpoch >= FIRST_SECOND_KEPT) && (secondsSinceEpoch <= LAST_SECOND_KEPT);
}

static inline uint32_t BoardRtc_TimeRegister(const struct SolidSyslogTimestamp* calendar)
{
    return (BoardRtc_ToBcd(calendar->Hour) << RTC_TR_HU_Pos) | (BoardRtc_ToBcd(calendar->Minute) << RTC_TR_MNU_Pos) |
           (BoardRtc_ToBcd(calendar->Second) << RTC_TR_SU_Pos);
}

static inline uint32_t BoardRtc_ToBcd(uint32_t value)
{
    return ((value / (uint32_t) BCD_TENS) << (uint32_t) BCD_TENS_SHIFT) | (value % (uint32_t) BCD_TENS);
}

static inline uint32_t BoardRtc_DateRegister(const struct SolidSyslogTimestamp* calendar, uint32_t secondsSinceEpoch)
{
    uint32_t daysSinceEpoch = secondsSinceEpoch / (uint32_t) SECONDS_PER_DAY;
    uint32_t weekday = ((daysSinceEpoch + (uint32_t) EPOCH_WEEKDAY_FROM_MONDAY) % (uint32_t) DAYS_PER_WEEK) + 1U;
    return (BoardRtc_ToBcd((uint32_t) calendar->Year - (uint32_t) CALENDAR_CENTURY) << RTC_DR_YU_Pos) |
           (weekday << RTC_DR_WDU_Pos) | (BoardRtc_ToBcd(calendar->Month) << RTC_DR_MU_Pos) |
           (BoardRtc_ToBcd(calendar->Day) << RTC_DR_DU_Pos);
}

/* A second has started. A clock not yet set counts from the start of 2000,
 * which is not reported. */
void RTC_Alarm_IRQHandler(void)
{
    EXTI->PR = EXTI_PR_PR17;
    if (READ_BIT(RTC->ISR, RTC_ISR_ALRAF) != 0U)
    {
        BoardRtc_ClearSecondFlag();
        if ((rtcOnSecond != NULL) && BoardRtc_HasTime())
        {
            uint32_t seconds = 0U;
            uint32_t microseconds = 0U;
            BoardRtc_Read(&seconds, &microseconds);
            rtcOnSecond(seconds);
        }
    }
}
