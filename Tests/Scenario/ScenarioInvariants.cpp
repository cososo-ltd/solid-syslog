#include "ScenarioInvariants.h"

#include <set>
#include <string>
#include <tuple>
#include <vector>

#include "SolidSyslogBufferCategories.h"
#include "SolidSyslogErrorCategory.h"
#include "SolidSyslogSenderCategories.h"
#include "SolidSyslogStoreCategories.h"

namespace
{
/* A pass moves at most what the ring held, the record held from the last
 * pass and one from the store, with two sends and a few device operations
 * per record. Tripwires, not budgets: a call that spins passes them at
 * once. */
const size_t RECORDS_BEYOND_THE_RING = 2U;
const size_t SENDS_PER_RECORD = 2U;
const size_t DEVICE_OPERATIONS_PER_RECORD = 4U;

typedef std::string (*Check)(const ScenarioState& state);

struct Invariant
{
    const char* Name;
    Check Judge;
};

std::string Count(const char* what, size_t count)
{
    return std::string(what) + "=" + std::to_string(count);
}

bool IsLoss(const ScenarioEvent& event)
{
    return (event.Category == SOLIDSYSLOG_CAT_BUFFER_RECORDS_LOST) ||
           (event.Category == SOLIDSYSLOG_CAT_SENDER_RECORDS_LOST) ||
           (event.Category == SOLIDSYSLOG_CAT_STORE_RECORDS_LOST) ||
           (event.Category == SOLIDSYSLOG_CAT_STORE_RECORD_DISCARDED) ||
           (event.Category == SOLIDSYSLOG_CAT_STORE_WRITE_FAILED) || (event.Category == SOLIDSYSLOG_CAT_STORE_FROZEN);
}

size_t CountCategory(const std::vector<ScenarioEvent>& events, uint16_t category)
{
    size_t count = 0U;
    for (const ScenarioEvent& event : events)
    {
        count += (event.Category == category) ? 1U : 0U;
    }
    return count;
}

std::string SequenceIdsAreUsedOnce(const ScenarioState& state)
{
    std::set<unsigned> seen;
    std::string reason;
    for (const ScenarioDelivery& delivery : state.Bench.Delivered())
    {
        if (reason.empty() && !seen.insert(delivery.SequenceId).second)
        {
            reason = Count("sequenceId", delivery.SequenceId) + " delivered twice";
        }
    }
    return reason;
}

std::string EveryDeliveryIsALoggedRecordOnce(const ScenarioState& state)
{
    std::set<unsigned> seen;
    std::string reason;
    for (const ScenarioDelivery& delivery : state.Bench.Delivered())
    {
        if (reason.empty() && (delivery.Ordinal > state.Bench.Logged()))
        {
            reason = Count("ordinal", delivery.Ordinal) + " was never logged";
        }
        if (reason.empty() && !seen.insert(delivery.Ordinal).second)
        {
            reason = Count("ordinal", delivery.Ordinal) + " delivered twice";
        }
    }
    return reason;
}

std::string EveryFrameIsARecord(const ScenarioState& state)
{
    return (state.Bench.Garbage() == 0U) ? std::string() : Count("garbage frames", state.Bench.Garbage());
}

std::string NoUniversalError(const ScenarioState& state)
{
    std::string reason;
    for (const ScenarioEvent& event : state.Bench.Events())
    {
        bool universal =
            (event.Category == SOLIDSYSLOG_CAT_BAD_CONFIG) || (event.Category == SOLIDSYSLOG_CAT_BAD_ARGUMENT) ||
            (event.Category == SOLIDSYSLOG_CAT_POOL_EXHAUSTED) || (event.Category == SOLIDSYSLOG_CAT_UNKNOWN_DESTROY);
        if (reason.empty() && universal)
        {
            reason = Count("category", event.Category) + " " + Count("detail", static_cast<size_t>(event.Detail));
        }
    }
    return reason;
}

/* One DELIVERY_FAILED opens an outage and one RESTORED closes it. */
std::string SenderEdgesAlternate(const ScenarioState& state)
{
    bool failed = false;
    std::string reason;
    for (const ScenarioEvent& event : state.Bench.Events())
    {
        if (reason.empty() && (event.Category == SOLIDSYSLOG_CAT_SENDER_DELIVERY_FAILED))
        {
            reason = failed ? "DELIVERY_FAILED twice without RESTORED" : "";
            failed = true;
        }
        if (reason.empty() && (event.Category == SOLIDSYSLOG_CAT_SENDER_DELIVERY_RESTORED))
        {
            reason = failed ? "" : "RESTORED without DELIVERY_FAILED";
            failed = false;
        }
    }
    return reason;
}

std::string SenderEdgesFollowTheFault(const ScenarioState& state)
{
    std::string reason;
    const std::vector<ScenarioEvent>& events = state.Bench.Events();
    for (size_t i = state.EventsJudged; reason.empty() && (i < events.size()); i++)
    {
        if ((events[i].Category == SOLIDSYSLOG_CAT_SENDER_DELIVERY_FAILED) && !state.SenderDown)
        {
            reason = "DELIVERY_FAILED with the sender up";
        }
        if ((events[i].Category == SOLIDSYSLOG_CAT_SENDER_DELIVERY_RESTORED) && state.SenderDown)
        {
            reason = "RESTORED with the sender down";
        }
    }
    return reason;
}

std::string StoreFaultsFollowTheDevice(const ScenarioState& state)
{
    std::string reason;
    const std::vector<ScenarioEvent>& events = state.Bench.Events();
    for (size_t i = state.EventsJudged; reason.empty() && (i < events.size()); i++)
    {
        bool deviceFault = (events[i].Category == SOLIDSYSLOG_CAT_STORE_FROZEN) ||
                           (events[i].Category == SOLIDSYSLOG_CAT_STORE_WRITE_FAILED);
        if (deviceFault && !state.DeviceFailed)
        {
            reason = Count("category", events[i].Category) + " with the device working";
        }
    }
    if (reason.empty() && (CountCategory(events, SOLIDSYSLOG_CAT_STORE_FROZEN) > 1U))
    {
        reason = "STORE_FROZEN more than once";
    }
    return reason;
}

/* A latched report stands for a run; the same report twice in one pass is
 * a latch that did not hold. */
std::string ALossIsReportedOncePerRun(const ScenarioState& state)
{
    std::set<std::tuple<const struct SolidSyslogErrorSource*, uint16_t, int32_t>> seen;
    std::string reason;
    const std::vector<ScenarioEvent>& events = state.Bench.Events();
    for (size_t i = state.EventsJudged; reason.empty() && (i < events.size()); i++)
    {
        if (IsLoss(events[i]) &&
            !seen.insert(std::make_tuple(events[i].Source, events[i].Category, events[i].Detail)).second)
        {
            reason = Count("category", events[i].Category) + " " +
                     Count("detail", static_cast<size_t>(events[i].Detail)) + " twice in one pass";
        }
    }
    return reason;
}

std::string HaltedOnlyUnderHalt(const ScenarioState& state)
{
    std::string reason;
    if ((state.Status == SOLIDSYSLOG_SERVICE_HALTED) && (state.Policy != SOLIDSYSLOG_DISCARD_POLICY_HALT))
    {
        reason = "HALTED under a policy that is not Halt";
    }
    if (reason.empty() && (state.Bench.Halts() > state.HaltPeriods))
    {
        reason = Count("halts", state.Bench.Halts()) + " " + Count("halt periods", state.HaltPeriods);
    }
    return reason;
}

std::string NothingMovesWhileHalted(const ScenarioState& state)
{
    std::string reason;
    bool stillHalted = (state.StatusBeforePass == SOLIDSYSLOG_SERVICE_HALTED) && !state.PolicyChangedSinceLastPass;
    if (stillHalted && (state.Status != SOLIDSYSLOG_SERVICE_HALTED))
    {
        reason = "left HALTED without a policy change";
    }
    if (stillHalted && (state.Bench.Delivered().size() != state.DeliveredBeforePass))
    {
        reason = "delivered while HALTED";
    }
    return reason;
}

std::string WorkPerPassIsBounded(const ScenarioState& state)
{
    size_t records = (state.Bench.RingBytes() / SCENARIO_SMALLEST_RECORD_BYTES) + RECORDS_BEYOND_THE_RING;
    std::string reason;
    if (state.Bench.DeviceOperationsInLastPass() > (records * DEVICE_OPERATIONS_PER_RECORD))
    {
        reason = Count("device operations", state.Bench.DeviceOperationsInLastPass());
    }
    if (state.Bench.SendsInLastPass() > (records * SENDS_PER_RECORD))
    {
        reason = Count("sends", state.Bench.SendsInLastPass());
    }
    return reason;
}

const Invariant AFTER_EVERY_PASS[] = {
    {"sequence ids are used once", SequenceIdsAreUsedOnce},
    {"every delivery is a logged record, once", EveryDeliveryIsALoggedRecordOnce},
    {"every frame is a record", EveryFrameIsARecord},
    {"no universal error", NoUniversalError},
    {"sender edges alternate", SenderEdgesAlternate},
    {"sender edges follow the fault", SenderEdgesFollowTheFault},
    {"store faults follow the device", StoreFaultsFollowTheDevice},
    {"a loss is reported once per run", ALossIsReportedOncePerRun},
    {"halted only under halt", HaltedOnlyUnderHalt},
    {"nothing moves while halted", NothingMovesWhileHalted},
    {"work per pass is bounded", WorkPerPassIsBounded},
};

std::string TheDrainEndsIdle(const ScenarioState& state)
{
    return (state.Status == SOLIDSYSLOG_SERVICE_IDLE) ? std::string() : "the last pass was not IDLE";
}

/* Without a loss report, every record logged reached the sender. With one
 * the shortfall is legitimate but unaccounted: a report is latched once per
 * run and the Core has no per-record count of what its queues discarded, so
 * the exact account waits on a story that adds one. */
std::string EveryRecordIsDeliveredOrItsLossReported(const ScenarioState& state)
{
    bool lossReported = false;
    for (const ScenarioEvent& event : state.Bench.Events())
    {
        lossReported = lossReported || IsLoss(event);
    }
    bool shortfall = state.Bench.Delivered().size() != state.Bench.Logged();
    return (shortfall && !lossReported)
               ? (Count("logged", state.Bench.Logged()) + " " + Count("delivered", state.Bench.Delivered().size()))
               : std::string();
}

const Invariant AT_END[] = {
    {"the drain ends idle", TheDrainEndsIdle},
    {"every record is delivered or its loss reported", EveryRecordIsDeliveredOrItsLossReported},
};

template <size_t N> ScenarioVerdict Judge(const Invariant (&table)[N], const ScenarioState& state)
{
    ScenarioVerdict verdict = {true, "", "", state.Bench.Passes()};
    for (size_t i = 0U; verdict.Passed && (i < N); i++)
    {
        std::string reason = table[i].Judge(state);
        if (!reason.empty())
        {
            verdict.Passed = false;
            verdict.Invariant = table[i].Name;
            verdict.Reason = reason;
        }
    }
    return verdict;
}
} // namespace

ScenarioVerdict ScenarioInvariants_CheckAfterPass(const ScenarioState& state)
{
    return Judge(AFTER_EVERY_PASS, state);
}

ScenarioVerdict ScenarioInvariants_CheckAtEnd(const ScenarioState& state)
{
    return Judge(AT_END, state);
}
