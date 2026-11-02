#ifndef SCENARIOSUITE_H
#define SCENARIOSUITE_H

#include <cstddef>
#include <cstdint>
#include <string>

#include "ScenarioBench.h"

/* The fixture a script runs over. */
struct ScenarioRig
{
    const char* Name;
    ScenarioBenchOptions Options;
};

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
