/* The part's real-time clock - see BoardRtc.h.
 *
 * Not host-TDD'd: this file exists only in the cross build, and what it does
 * is write the part's registers. The target checklist in README.md is what
 * proves it, on the board. The calendar arithmetic is BddTargetCalendar's,
 * which is tested on the host. */

#include "BoardRtc.h"

#include "BddTargetCalendar.h"
#include "SolidSyslogTimestamp.h"

#include "stm32f7xx_hal.h"

#include <stddef.h>

enum
{
    /* The crystal is given this long to start. Cold, it takes a second or two. */
    CRYSTAL_START_MILLISECONDS = 5000,

    /* The clock leaves reset dividing 32768 Hz by 128 and then by 256, which
     * is one second, and is left so. Its sub-second register counts the 256
     * down, from this to zero. */
    SUBSECONDS_PER_SECOND = 256,
    SUBSECOND_AT_SECOND_START = SUBSECONDS_PER_SECOND - 1,
    MICROSECONDS_PER_SECOND = 1000000,

    /* What the clock's registers are unlocked with, and locked with again. */
    WRITE_KEY_FIRST = 0xCA,
    WRITE_KEY_SECOND = 0x53,
    WRITE_LOCK = 0xFF,

    /* The clock answers a request within a few cycles of its 32768 Hz, some
     * tens of microseconds. This many turns of a loop is far longer, and is
     * counted, not timed, so it works with interrupts off. */
    CLOCK_WAIT_LOOPS = 1000000,

    /* Above the ceiling the kernel sets for interrupts that call it, so a
     * critical section in the kernel does not hold a second's start back. The
     * handler calls nothing in the kernel. */
    SECOND_INTERRUPT_PRIORITY = 4,

    /* The clock keeps two digits of year. 2000 is left out because the clock
     * tells a year of 00 from having been set at all. */
    CALENDAR_CENTURY = 2000,
    BCD_TENS = 10,
    BCD_TENS_SHIFT = 4,
    BCD_UNITS_MASK = 0xF,

    /* 1970-01-01 was a Thursday, and the clock counts Monday as 1. */
    SECONDS_PER_DAY = 86400,
    DAYS_PER_WEEK = 7,
    EPOCH_WEEKDAY_FROM_MONDAY = 3
};

/* 2001-01-01T00:00:00Z and 2099-12-31T23:59:59Z. */
static const uint32_t FIRST_SECOND_KEPT = 978307200U;
static const uint32_t LAST_SECOND_KEPT = 4102444799U;

static bool rtcIsRunning;
static BoardRtcSecondFunction rtcOnSecond;

/* The name ST's vector table carries. */
void RTC_Alarm_IRQHandler(void);

static inline bool BoardRtc_StartCrystal(void);
static inline void BoardRtc_ClockFromCrystal(void);
static inline void BoardRtc_InterruptEverySecond(void);
static inline void BoardRtc_Unlock(void);
static inline void BoardRtc_Lock(void);
static inline bool BoardRtc_WaitFor(uint32_t flag);
static inline uint32_t BoardRtc_Seconds(uint32_t time, uint32_t date);
static inline uint8_t BoardRtc_FromBcd(uint32_t bcd);
static inline uint32_t BoardRtc_ToBcd(uint32_t value);
static inline bool BoardRtc_IsKept(uint32_t secondsSinceEpoch);
static inline uint32_t BoardRtc_TimeRegister(const struct SolidSyslogTimestamp* calendar);
static inline uint32_t BoardRtc_DateRegister(const struct SolidSyslogTimestamp* calendar, uint32_t secondsSinceEpoch);

bool BoardRtc_Start(BoardRtcSecondFunction onSecond)
{
    rtcOnSecond = onSecond;
    /* The clock and its crystal are in the backup domain, which is locked
     * against writes until the power controller is told otherwise. */
    __HAL_RCC_PWR_CLK_ENABLE();
    SET_BIT(PWR->CR1, PWR_CR1_DBP);
    rtcIsRunning = BoardRtc_StartCrystal();
    if (rtcIsRunning)
    {
        BoardRtc_ClockFromCrystal();
        BoardRtc_InterruptEverySecond();
    }
    return rtcIsRunning;
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

/* The clock's source can be chosen once after the backup domain is reset, so
 * it is chosen only where none has been: a clock already running is not
 * touched. */
static inline void BoardRtc_ClockFromCrystal(void)
{
    if (READ_BIT(RCC->BDCR, RCC_BDCR_RTCSEL) == 0U)
    {
        SET_BIT(RCC->BDCR, RCC_BDCR_RTCSEL_0);
    }
    SET_BIT(RCC->BDCR, RCC_BDCR_RTCEN);
}

/* Alarm A with every field of its match masked matches every second, as the
 * second starts. It reaches the processor through line 17 of the external
 * interrupt controller, on a rising edge.
 *
 * The calendar is read straight from its counters, not from the copies the
 * clock keeps of them, which lag the counters by a cycle or two of the
 * crystal: the interrupt reads as the second starts, and wants that second. */
static inline void BoardRtc_InterruptEverySecond(void)
{
    BoardRtc_Unlock();
    SET_BIT(RTC->CR, RTC_CR_BYPSHAD);
    CLEAR_BIT(RTC->CR, RTC_CR_ALRAE | RTC_CR_ALRAIE);
    if (BoardRtc_WaitFor(RTC_ISR_ALRAWF))
    {
        RTC->ALRMAR = RTC_ALRMAR_MSK1 | RTC_ALRMAR_MSK2 | RTC_ALRMAR_MSK3 | RTC_ALRMAR_MSK4;
        RTC->ALRMASSR = 0U;
        SET_BIT(RTC->CR, RTC_CR_ALRAE | RTC_CR_ALRAIE);
    }
    BoardRtc_Lock();

    SET_BIT(EXTI->RTSR, EXTI_RTSR_TR17);
    SET_BIT(EXTI->IMR, EXTI_IMR_MR17);
    HAL_NVIC_SetPriority(RTC_Alarm_IRQn, (uint32_t) SECOND_INTERRUPT_PRIORITY, 0U);
    HAL_NVIC_EnableIRQ(RTC_Alarm_IRQn);
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

bool BoardRtc_HasTime(void)
{
    return rtcIsRunning && (READ_BIT(RTC->ISR, RTC_ISR_INITS) != 0U);
}

/* The counters are read while they count, so they are read until two readings
 * agree, and again if the sub-second count went up between its two readings:
 * it counts down, so that is a second starting part way through. */
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

/* The calendar is written with the clock stopped, which is asked for and
 * waited on, and the clock starts again on the second written. */
bool BoardRtc_Set(uint32_t secondsSinceEpoch)
{
    bool taken = false;
    if (rtcIsRunning && BoardRtc_IsKept(secondsSinceEpoch))
    {
        struct SolidSyslogTimestamp calendar = {0};
        BddTargetCalendar_FromEpoch(secondsSinceEpoch, &calendar);
        BoardRtc_Unlock();
        /* Written whole, so that no flag is cleared: a flag in this register
         * is cleared by writing it 0 and left alone by writing it 1. */
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

static inline uint32_t BoardRtc_DateRegister(const struct SolidSyslogTimestamp* calendar, uint32_t secondsSinceEpoch)
{
    uint32_t daysSinceEpoch = secondsSinceEpoch / (uint32_t) SECONDS_PER_DAY;
    uint32_t weekday = ((daysSinceEpoch + (uint32_t) EPOCH_WEEKDAY_FROM_MONDAY) % (uint32_t) DAYS_PER_WEEK) + 1U;
    return (BoardRtc_ToBcd((uint32_t) calendar->Year - (uint32_t) CALENDAR_CENTURY) << RTC_DR_YU_Pos) |
           (weekday << RTC_DR_WDU_Pos) | (BoardRtc_ToBcd(calendar->Month) << RTC_DR_MU_Pos) |
           (BoardRtc_ToBcd(calendar->Day) << RTC_DR_DU_Pos);
}

static inline uint32_t BoardRtc_ToBcd(uint32_t value)
{
    return ((value / (uint32_t) BCD_TENS) << (uint32_t) BCD_TENS_SHIFT) | (value % (uint32_t) BCD_TENS);
}

/* A second has started. The alarm's flag is cleared by writing it 0 with
 * every other flag written 1, and the request to stop the clock, which shares
 * the register, written as it stands. A clock not yet set counts from the
 * start of 2000, which is no time to tell anyone. */
void RTC_Alarm_IRQHandler(void)
{
    EXTI->PR = EXTI_PR_PR17;
    if (READ_BIT(RTC->ISR, RTC_ISR_ALRAF) != 0U)
    {
        RTC->ISR = ~(RTC_ISR_ALRAF | RTC_ISR_INIT) | READ_BIT(RTC->ISR, RTC_ISR_INIT);
        if ((rtcOnSecond != NULL) && BoardRtc_HasTime())
        {
            uint32_t seconds = 0U;
            uint32_t microseconds = 0U;
            BoardRtc_Read(&seconds, &microseconds);
            rtcOnSecond(seconds);
        }
    }
}
