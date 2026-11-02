#include <cstdint>
#include <string>

#include "ScenarioSuite.h"
#include "SolidSyslogDiscardPolicy.h"
#include "CppUTest/TestHarness.h"

namespace
{
const ScenarioRig OLDEST = {"oldest", {4U, 512U, 3U, SOLIDSYSLOG_DISCARD_POLICY_OLDEST}};
const ScenarioRig NEWEST = {"newest", {4U, 512U, 3U, SOLIDSYSLOG_DISCARD_POLICY_NEWEST}};
const ScenarioRig HALT = {"halt", {4U, 512U, 3U, SOLIDSYSLOG_DISCARD_POLICY_HALT}};

std::string Repeat(const std::string& steps, size_t times)
{
    std::string script;
    for (size_t i = 0U; i < times; i++)
    {
        script += steps;
    }
    return script;
}
} // namespace

/* A scripted shape passes when every invariant holds after every pass and
 * after the drain; a failure prints what reproduces it. */
#define CHECK_SCENARIO(rig, script)                                         \
    {                                                                       \
        ScenarioVerdict verdict = ScenarioSuite_Run((rig), (script));       \
        if (!verdict.Passed)                                                \
        {                                                                   \
            FAIL(ScenarioSuite_Describe((rig), (script), verdict).c_str()); \
        }                                                                   \
    }

// clang-format off
TEST_GROUP(ScenarioSuite)
{
};

// clang-format on

TEST(ScenarioSuite, AScriptWithAnUnknownStepIsRefusedWithItsScript)
{
    ScenarioVerdict verdict = ScenarioSuite_Run(OLDEST, "LSX");
    CHECK_FALSE(verdict.Passed);
    STRCMP_EQUAL("the script is well formed", verdict.Invariant.c_str());
    STRCMP_CONTAINS("script=LSX", ScenarioSuite_Describe(OLDEST, "LSX", verdict).c_str());
}

TEST(ScenarioSuite, ASenderRefusingForSeveralPassesIsOneOutage)
{
    CHECK_SCENARIO(OLDEST, "LLLDSSSSULSS");
}

TEST(ScenarioSuite, ASenderRefusingBeforeAnythingIsQueuedIsNoOutage)
{
    CHECK_SCENARIO(OLDEST, "DSSSULS");
}

TEST(ScenarioSuite, ADeviceFailingAtAPassFreezesTheStoreOnce)
{
    CHECK_SCENARIO(OLDEST, "LSLSFLSLSLS");
}

TEST(ScenarioSuite, ADeviceFailingDuringAnOutageLosesWhatTheStoreHeld)
{
    CHECK_SCENARIO(OLDEST, "D" + Repeat("LS", 20U) + "F" + Repeat("LS", 5U) + "U" + Repeat("S", 5U));
}

TEST(ScenarioSuite, ABufferFillingUnderOldestEvictsOnce)
{
    CHECK_SCENARIO(OLDEST, Repeat("L", 200U) + Repeat("S", 10U) + Repeat("L", 200U));
}

TEST(ScenarioSuite, ABufferFillingUnderNewestRefusesOnce)
{
    CHECK_SCENARIO(NEWEST, Repeat("L", 200U) + Repeat("S", 10U) + Repeat("L", 200U));
}

TEST(ScenarioSuite, AStoreFillingUnderOldestRotatesAwayUnsentRecords)
{
    CHECK_SCENARIO(OLDEST, "D" + Repeat("LS", 150U) + "U");
}

TEST(ScenarioSuite, AStoreFillingUnderNewestRefusesTheNewRecords)
{
    CHECK_SCENARIO(NEWEST, "D" + Repeat("LS", 150U) + "U");
}

TEST(ScenarioSuite, AStoreFillingUnderHaltStopsTheLoggerUntilThePolicyChanges)
{
    CHECK_SCENARIO(HALT, "D" + Repeat("LS", 150U) + "O" + Repeat("LS", 10U) + "U");
}

TEST(ScenarioSuite, APolicyChangedMidOutageIsHonouredFromTheNextRecord)
{
    CHECK_SCENARIO(OLDEST, "D" + Repeat("LS", 100U) + "N" + Repeat("LS", 100U) + "H" + Repeat("LS", 100U) + "U");
}

TEST(ScenarioSuite, ARandomWalkHoldsEveryInvariant)
{
    uint32_t seed = 0U;
    bool one = ScenarioSuite_SeedGiven(&seed);
    size_t runs = one ? 1U : ScenarioSuite_Runs();
    for (size_t i = 0U; i < runs; i++)
    {
        uint32_t walk = one ? seed : static_cast<uint32_t>(i + 1U);
        const ScenarioRig& rig = ScenarioSuite_RigForSeed(walk);
        std::string script = ScenarioSuite_ScriptForSeed(walk);
        ScenarioVerdict verdict = ScenarioSuite_Run(rig, script);
        if (!verdict.Passed)
        {
            FAIL(("seed=" + std::to_string(walk) + " " + ScenarioSuite_Describe(rig, script, verdict)).c_str());
        }
    }
}
