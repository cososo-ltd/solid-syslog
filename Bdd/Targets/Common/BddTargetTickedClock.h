#ifndef BDDTARGETTICKEDCLOCK_H
#define BDDTARGETTICKEDCLOCK_H

#include "SolidSyslogExternC.h"
#include "SolidSyslogTimestamp.h"

#include <stdbool.h>
#include <stdint.h>

/* A clock for a target whose hardware keeps the time in whole seconds and
   says when each one starts: a real-time clock with a once-a-second interrupt.

   The seconds come from that hardware. The time within a second is measured
   here, on a free-running counter of the target's own that counts far faster,
   from the count it stood at as the second started. So a reading has the
   counter's resolution and never touches the hardware clock, which only its
   tick and whoever sets it read.

   The counter and the hardware clock run from different oscillators, so the
   counter can reach a second's worth a little before the next tick arrives.
   A reading stops at the last microsecond of its second until it does, so
   time is never read as running backwards.

   Until it is told the time the clock has none, and a reading says so the way
   SolidSyslogClockFunction lets it: with the timestamp zeroed. */

SOLIDSYSLOG_EXTERN_C_BEGIN

    /* The free-running counter, as it stands now. It may wrap, as long as it
       takes longer to come round than the longest a tick can be late: a
       reading is held in its second only until the counter has. */
    typedef uint32_t (*BddTargetTickedClockCounterFunction)(void);

    /* Forgets the time. countsPerMicrosecond is how fast the counter counts. */
    void BddTargetTickedClock_Initialise(BddTargetTickedClockCounterFunction counter, uint32_t countsPerMicrosecond);

    /* A second has just started: secondsSinceEpoch, in UTC. For the hardware
       clock's once-a-second interrupt. */
    void BddTargetTickedClock_Tick(uint32_t secondsSinceEpoch);

    /* It is microseconds into the second secondsSinceEpoch. For a target that
       finds its hardware clock already running, part way through a second. */
    void BddTargetTickedClock_Set(uint32_t secondsSinceEpoch, uint32_t microseconds);

    bool BddTargetTickedClock_IsSet(void);

    /* A SolidSyslogClockFunction. Safe to call from any task while Tick is
       called from an interrupt: it reads again if a tick arrived as it read.
       That is all it guards against, so Tick, Set and Initialise are each
       called where no reading can interrupt them: from the interrupt, or with
       interrupts held off. */
    void BddTargetTickedClock_GetTimestamp(struct SolidSyslogTimestamp * timestamp);

SOLIDSYSLOG_EXTERN_C_END

#endif /* BDDTARGETTICKEDCLOCK_H */
