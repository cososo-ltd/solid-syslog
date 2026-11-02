#ifndef SCENARIOSUITE_H
#define SCENARIOSUITE_H

#include <cstddef>
#include <cstdint>
#include <string>

#include "ScenarioBench.h"
#include "SolidSyslogDiscardPolicy.h"

/* The fixture a script runs over. */
struct ScenarioRig
{
    const char* Name;
    ScenarioBenchOptions Options;
};

/* The rigs: the policy the logger, buffer and store start under, over a
 * ring and a store small enough that a walk fills them. */
inline const ScenarioRig SCENARIO_RIG_OLDEST = {"oldest", {4U, 512U, 3U, SOLIDSYSLOG_DISCARD_POLICY_OLDEST}};
inline const ScenarioRig SCENARIO_RIG_NEWEST = {"newest", {4U, 512U, 3U, SOLIDSYSLOG_DISCARD_POLICY_NEWEST}};
inline const ScenarioRig SCENARIO_RIG_HALT = {"halt", {8U, 1024U, 4U, SOLIDSYSLOG_DISCARD_POLICY_HALT}};

/* What a run found: a pass, or the first invariant to fail, where and why. */
struct ScenarioVerdict
{
    bool Passed;
    std::string Invariant;
    std::string Reason;
    size_t Pass;
};

/* A script is one letter per step: L logs a record, S makes a Service pass,
 * D and U take the sender down and up, F fails the block device, and O, N
 * and H set the discard policy to Oldest, Newest or Halt. The invariants are
 * checked after every pass, and once more after the run drains what the
 * script left queued. */
ScenarioVerdict ScenarioSuite_Run(const ScenarioRig& rig, const std::string& script);

/* The rig and script a seed stands for: a random walk of the steps above. */
const ScenarioRig& ScenarioSuite_RigForSeed(uint32_t seed);
std::string ScenarioSuite_ScriptForSeed(uint32_t seed);

/* How a failure is printed: everything a rerun needs. */
std::string ScenarioSuite_Describe(const ScenarioRig& rig, const std::string& script, const ScenarioVerdict& verdict);

/* The one seed to run, from SOLIDSYSLOG_SCENARIO_SEED; whether one was given. */
bool ScenarioSuite_SeedGiven(uint32_t* seed);

/* Random walks per run of the suite, from SOLIDSYSLOG_SCENARIO_RUNS: a smoke
 * count when unset. */
size_t ScenarioSuite_Runs();

#endif /* SCENARIOSUITE_H */
