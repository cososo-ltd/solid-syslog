#ifndef BDDTARGETTICKEDCLOCK_H
#define BDDTARGETTICKEDCLOCK_H

#include "SolidSyslogExternC.h"
#include "SolidSyslogTimestamp.h"

#include <stdbool.h>
#include <stdint.h>

/* A clock for a target whose hardware keeps the time in whole seconds and
   signals the start of each: a real-time clock with a once-a-second interrupt.

   The seconds come from that hardware. The time within a second is measured
   on a faster free-running counter of the target's, from its count when the
   second started. A reading therefore has the counter's resolution and does
   not touch the hardware clock.

   The counter and the hardware clock run from different oscillators, so the
   counter can reach a second's worth a little before the next tick arrives.
   Until the tick, a reading stays at the last microsecond of its second, so
   time never reads as running backwards.

   Until the clock is told the time, a reading is a zeroed timestamp, as
   SolidSyslogClockFunction allows. */

SOLIDSYSLOG_EXTERN_C_BEGIN

    /* Returns the free-running counter. It may wrap, provided it takes longer
       to wrap than a tick can be late. */
    typedef uint32_t (*BddTargetTickedClockCounterFunction)(void);

    /* Forgets the time. countsPerMicrosecond is how fast the counter counts. */
    void BddTargetTickedClock_Initialise(BddTargetTickedClockCounterFunction counter, uint32_t countsPerMicrosecond);

    /* The second secondsSinceEpoch, in UTC, has just started. Call from the
       hardware clock's once-a-second interrupt. */
    void BddTargetTickedClock_Tick(uint32_t secondsSinceEpoch);

    /* The time is `microseconds` into the second secondsSinceEpoch. For a
       target that finds its hardware clock already running, part way through
       a second. */
    void BddTargetTickedClock_Set(uint32_t secondsSinceEpoch, uint32_t microseconds);

    bool BddTargetTickedClock_IsSet(void);

    /* A SolidSyslogClockFunction. Safe to call from any task while Tick is
       called from an interrupt: it reads again if a tick arrived during the
       read. That is its only guard, so call Tick, Set and Initialise where no
       reading can interrupt them: from the interrupt, or with interrupts
       disabled. */
    void BddTargetTickedClock_GetTimestamp(struct SolidSyslogTimestamp * timestamp);

SOLIDSYSLOG_EXTERN_C_END

#endif /* BDDTARGETTICKEDCLOCK_H */
