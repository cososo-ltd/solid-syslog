/* ST's NUCLEO-F767ZI behind the board seam's clock - see BddTargetBoardClock.h.
 *
 * The part's real-time clock keeps the time, in whole seconds, and interrupts
 * as each one starts. The time is read from BddTargetTickedClock, which takes
 * the second from that interrupt and measures how far into it a reading is on
 * the core's cycle counter. So a reading is good to a microsecond, costs a
 * few instructions, and leaves the real-time clock to its interrupt.
 *
 * Not host-TDD'd: this file exists only in the cross build. The target
 * checklist in README.md is what proves it, on the board. */

#include "BddTargetBoardClock.h"

#include "BddTargetTickedClock.h"
#include "BoardClock.h"
#include "BoardRtc.h"

#include "stm32f7xx_hal.h"

enum
{
    MICROSECONDS_PER_SECOND = 1000000
};

/* What the cycle counter's registers are unlocked with on this core. */
static const uint32_t CYCLE_COUNTER_UNLOCK = 0xC5ACCE55U;

static inline void BoardClock_StartCycleCounter(void);
static uint32_t BoardClock_CycleCount(void);

/* The software clock is told the time with interrupts off, here and in
 * BddTargetBoardClock_Set, so that the second's interrupt cannot land between
 * the real-time clock being read or written and the count being taken. */
void BoardClock_Start(void)
{
    BoardClock_StartCycleCounter();
    BddTargetTickedClock_Initialise(BoardClock_CycleCount, SystemCoreClock / (uint32_t) MICROSECONDS_PER_SECOND);
    if (BoardRtc_Start(BddTargetTickedClock_Tick))
    {
        uint32_t interrupts = __get_PRIMASK();
        __disable_irq();
        if (BoardRtc_HasTime())
        {
            uint32_t seconds = 0U;
            uint32_t microseconds = 0U;
            BoardRtc_Read(&seconds, &microseconds);
            BddTargetTickedClock_Set(seconds, microseconds);
        }
        __set_PRIMASK(interrupts);
    }
}

/* The counter is part of the core's debug unit, and counts whether or not a
 * debugger is attached once the unit is switched on. It counts the core's
 * clock, 216 to the microsecond, and wraps every twenty seconds, which the
 * software clock allows for. */
static inline void BoardClock_StartCycleCounter(void)
{
    SET_BIT(CoreDebug->DEMCR, CoreDebug_DEMCR_TRCENA_Msk);
    DWT->LAR = CYCLE_COUNTER_UNLOCK;
    SET_BIT(DWT->CTRL, DWT_CTRL_CYCCNTENA_Msk);
}

static uint32_t BoardClock_CycleCount(void)
{
    return DWT->CYCCNT;
}

bool BddTargetBoardClock_Set(uint32_t secondsSinceEpoch)
{
    uint32_t interrupts = __get_PRIMASK();
    __disable_irq();
    bool taken = BoardRtc_Set(secondsSinceEpoch);
    if (taken)
    {
        BddTargetTickedClock_Set(secondsSinceEpoch, 0U);
    }
    __set_PRIMASK(interrupts);
    return taken;
}

bool BddTargetBoardClock_IsSet(void)
{
    return BddTargetTickedClock_IsSet();
}

void BddTargetBoardClock_GetTimestamp(struct SolidSyslogTimestamp* timestamp)
{
    BddTargetTickedClock_GetTimestamp(timestamp);
}
