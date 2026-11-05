#ifndef SCENARIOINVARIANTS_H
#define SCENARIOINVARIANTS_H

#include <cstddef>

#include "ScenarioBench.h"
#include "ScenarioSuite.h"
#include "SolidSyslogDiscardPolicy.h"
#include "SolidSyslogServiceStatus.h"

/* What the invariants judge: the bench's record, and the faults the driver
 * has put in force. The driver keeps it current; the checks only read it. */
struct ScenarioState
{
    ScenarioBench& Bench;
    bool SenderDown;
    bool DeviceFailed;
    enum SolidSyslogDiscardPolicy Policy;
    size_t HaltPeriods; /* times the policy has been Halt, counting an initial Halt */
    size_t PolicyChangesSinceLastPass;
    enum SolidSyslogServiceStatus StatusBeforePass;
    enum SolidSyslogServiceStatus Status;
    size_t DeliveredBeforePass;
    size_t EventsJudged; /* events earlier checks have already seen */
};

/* Every invariant that holds after each pass; the first to fail is the verdict. */
ScenarioVerdict ScenarioInvariants_CheckAfterPass(const ScenarioState& state);

/* The invariants that hold once the run has drained. */
ScenarioVerdict ScenarioInvariants_CheckAtEnd(const ScenarioState& state);

#endif /* SCENARIOINVARIANTS_H */
