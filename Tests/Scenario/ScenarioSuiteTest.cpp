#include <cstdint>
#include <string>

#include "ScenarioSuite.h"
#include "SolidSyslogCircularBuffer.h"
#include "CppUTest/TestHarness.h"

namespace
{
const ScenarioRig& OLDEST = SCENARIO_RIG_OLDEST;
const ScenarioRig& NEWEST = SCENARIO_RIG_NEWEST;
const ScenarioRig& HALT = SCENARIO_RIG_HALT;
const ScenarioRig& OLDEST_FILE = SCENARIO_RIG_OLDEST_FILE;
const ScenarioRig& OLDEST_NO_STORE = SCENARIO_RIG_OLDEST_NO_STORE;
const ScenarioRig& NEWEST_NO_STORE = SCENARIO_RIG_NEWEST_NO_STORE;
const ScenarioRig& HALT_NO_STORE = SCENARIO_RIG_HALT_NO_STORE;

/* Enough records to fill the rig's store with room to spare. */
size_t StoreFill(const ScenarioRig& rig)
{
    return ((rig.Options.MaxBlocks * rig.Options.BlockSize) / SCENARIO_SMALLEST_RECORD_BYTES) + 20U;
}

/* Enough records to fill the rig's ring with room to spare. */
size_t RingFill(const ScenarioRig& rig)
{
    return (SOLIDSYSLOG_CIRCULAR_BUFFER_RING_BYTES(rig.Options.BufferMessages) / SCENARIO_SMALLEST_RECORD_BYTES) + 20U;
}

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

TEST(ScenarioSuite, ASenderRefusingWhatWasQueuedIsReportedOnce)
{
    CHECK_SCENARIO(OLDEST, "LDSSULSDSUS");
}

TEST(ScenarioSuite, ADeviceFailingAtAPassFreezesTheStoreOnce)
{
    CHECK_SCENARIO(OLDEST, "LSLSFLSLSLS");
}

TEST(ScenarioSuite, AFileSystemFailingAtAPassIsReportedAndFreezesTheStoreOnce)
{
    CHECK_SCENARIO(OLDEST_FILE, "LSLSFLSLSLS");
}

TEST(ScenarioSuite, ADeviceFailingWithNothingQueuedIsNotYetAStoreFault)
{
    CHECK_SCENARIO(OLDEST, "LSFSSLS");
}

TEST(ScenarioSuite, ADeviceFailingDuringAnOutageLosesWhatTheStoreHeld)
{
    CHECK_SCENARIO(OLDEST, "D" + Repeat("LS", 20U) + "F" + Repeat("LS", 5U) + "U" + Repeat("S", 5U));
}

TEST(ScenarioSuite, ABufferFillingUnderOldestEvictsOnce)
{
    CHECK_SCENARIO(OLDEST, Repeat("L", RingFill(OLDEST)) + Repeat("S", 10U) + Repeat("L", RingFill(OLDEST)));
}

TEST(ScenarioSuite, ABufferFillingUnderNewestRefusesOnce)
{
    CHECK_SCENARIO(NEWEST, Repeat("L", RingFill(NEWEST)) + Repeat("S", 10U) + Repeat("L", RingFill(NEWEST)));
}

TEST(ScenarioSuite, AStoreFillingUnderOldestRotatesAwayUnsentRecords)
{
    CHECK_SCENARIO(OLDEST, "D" + Repeat("LS", StoreFill(OLDEST)) + "U");
}

TEST(ScenarioSuite, AStoreFillingUnderNewestRefusesTheNewRecords)
{
    CHECK_SCENARIO(NEWEST, "D" + Repeat("LS", StoreFill(NEWEST)) + "U");
}

TEST(ScenarioSuite, AStoreFillingUnderHaltStopsTheLoggerUntilThePolicyChanges)
{
    CHECK_SCENARIO(HALT, "D" + Repeat("LS", StoreFill(HALT)) + "O" + Repeat("LS", 10U) + "U");
}

TEST(ScenarioSuite, APolicyChangedMidOutageIsHonouredFromTheNextRecord)
{
    size_t fill = StoreFill(OLDEST);
    CHECK_SCENARIO(OLDEST, "D" + Repeat("LS", fill) + "N" + Repeat("LS", fill) + "H" + Repeat("LS", fill) + "U");
}

TEST(ScenarioSuite, AWithheldKeyIsReportedOnceAndTheStoreWaitsForIt)
{
    CHECK_SCENARIO(OLDEST, "LSDLLLSKU" + Repeat("S", 5U) + "W" + Repeat("S", 5U));
}

TEST(ScenarioSuite, AFailingPolicyBackendIsReportedOnceForTheRun)
{
    CHECK_SCENARIO(OLDEST, "LLLP" + Repeat("S", 5U) + "W" + Repeat("S", 5U));
}

/* A pass sends one record from the store, so the records kept while the
 * backend worked are opened on every pass after it fails, and wait in the
 * store until it works. */
TEST(ScenarioSuite, AFailingPolicyBackendOnReadIsReportedOnceAndTheStoreWaitsForIt)
{
    CHECK_SCENARIO(OLDEST, "LLLSP" + Repeat("S", 5U) + "W" + Repeat("S", 5U));
}

TEST(ScenarioSuite, ARecordThatFailsVerificationIsDiscardedAndReportedOnce)
{
    CHECK_SCENARIO(OLDEST, "DLLLSUT" + Repeat("S", 5U) + "W" + Repeat("S", 5U));
}

TEST(ScenarioSuite, ARefusedHandshakeIsReportedOnceAheadOfTheOutage)
{
    CHECK_SCENARIO(OLDEST, "LSJLLL" + Repeat("S", 5U) + "A" + Repeat("S", 5U));
}

TEST(ScenarioSuite, ARefusedHandshakeWithNoStoreIsReportedOnceTheSenderTakesIt)
{
    CHECK_SCENARIO(OLDEST_NO_STORE, "LSJLLL" + Repeat("S", 5U) + "A" + Repeat("S", 5U));
}

TEST(ScenarioSuite, AMisroutedSenderIsReportedOnceForTheRun)
{
    CHECK_SCENARIO(OLDEST, "MLLL" + Repeat("S", 5U) + "B" + Repeat("S", 5U));
}

/* With reporting off, what the sender refuses is a record from the buffer,
 * which Oldest drops when the ring fills behind it. */
TEST(ScenarioSuite, AMisroutedSenderDroppingAHeldRecordEndsTheRunOfHeldDrops)
{
    std::string fill = Repeat("L", RingFill(OLDEST_NO_STORE));
    std::string heldDrop = fill + "S" + fill + "S";
    CHECK_SCENARIO(OLDEST_NO_STORE, "QD" + heldDrop + "MSB" + heldDrop + "USS");
}

TEST(ScenarioSuite, ACorruptRingIsReportedOnceAsItsBackendFailing)
{
    CHECK_SCENARIO(OLDEST_NO_STORE, "LC" + Repeat("S", 5U));
}

TEST(ScenarioSuite, ABufferLossIsReportedOnceThroughTheStore)
{
    CHECK_SCENARIO(OLDEST, Repeat("L", RingFill(OLDEST)) + Repeat("S", 10U));
}

TEST(ScenarioSuite, ABufferLossIsReportedOnceWithNoStore)
{
    CHECK_SCENARIO(OLDEST_NO_STORE, Repeat("L", RingFill(OLDEST_NO_STORE)) + Repeat("S", 10U));
}

TEST(ScenarioSuite, ALossRaisedWhileReportingIsOffIsReportedOnceItIsOn)
{
    CHECK_SCENARIO(OLDEST_NO_STORE, "Q" + Repeat("L", RingFill(OLDEST_NO_STORE)) + Repeat("S", 10U) + "RSS");
}

TEST(ScenarioSuite, AReportTheSenderRefusesIsHeldAndLandsOnRecovery)
{
    CHECK_SCENARIO(OLDEST_NO_STORE, Repeat("L", RingFill(OLDEST_NO_STORE)) + "D" + Repeat("S", 5U) + "USS");
}

TEST(ScenarioSuite, AHeldReportIsNotLostWhileTheRingRefillsBehindIt)
{
    CHECK_SCENARIO(OLDEST_NO_STORE, "D" + Repeat(Repeat("L", RingFill(OLDEST_NO_STORE)) + "S", 3U) + "U");
}

TEST(ScenarioSuite, AHeldReportUnderNewestWaitsForTheSender)
{
    CHECK_SCENARIO(NEWEST_NO_STORE, "D" + Repeat(Repeat("L", RingFill(NEWEST_NO_STORE)) + "S", 3U) + "U");
}

TEST(ScenarioSuite, AHeldReportUnderHaltWaitsOutTheHalt)
{
    CHECK_SCENARIO(HALT_NO_STORE, "D" + Repeat("L", RingFill(HALT_NO_STORE)) + "SSOSU");
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
