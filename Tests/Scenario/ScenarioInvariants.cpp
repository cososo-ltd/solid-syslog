#include "ScenarioInvariants.h"

#include <map>
#include <set>
#include <string>
#include <tuple>
#include <vector>

#include "SolidSyslogBufferCategories.h"
#include "SolidSyslogErrorCategory.h"
#include "SolidSyslogErrors.h"
#include "SolidSyslogSenderCategories.h"
#include "SolidSyslogStoreCategories.h"

namespace
{
/* A pass moves at most what the ring held, the record held from the last
 * pass, a report and one from the store, with two sends and a few device
 * operations per record. Tripwires, not budgets: a call that spins passes
 * them at once. */
const size_t RECORDS_BEYOND_THE_RING = 3U;

/* The kinds Service reports so far; a story that reports another adds it. */
const uint16_t REPORTED_KINDS[] = {SOLIDSYSLOG_CAT_BUFFER_RECORDS_LOST};
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

/* A report never passes through the buffer, so only the store and the sender
 * can lose one. */
bool IsDownstreamLoss(const ScenarioEvent& event)
{
    return IsLoss(event) && (event.Category != SOLIDSYSLOG_CAT_BUFFER_RECORDS_LOST);
}

bool IsReportedKind(uint16_t category)
{
    bool reported = false;
    for (uint16_t kind : REPORTED_KINDS)
    {
        reported = reported || (kind == category);
    }
    return reported;
}

std::string NameOf(uint16_t category)
{
    const char* name = SolidSyslogErrorCategory_Name(category);
    return (name != nullptr) ? std::string(name) : std::string();
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

/* A report takes its sequenceId from the meta SD's count, as a record does,
 * so records and reports together use each one once. */
std::string SequenceIdsAreUsedOnce(const ScenarioState& state)
{
    std::vector<unsigned> used;
    for (const ScenarioDelivery& delivery : state.Bench.Delivered())
    {
        used.push_back(delivery.SequenceId);
    }
    for (const ScenarioReport& report : state.Bench.Reports())
    {
        used.push_back(report.SequenceId);
    }
    std::set<unsigned> seen;
    std::string reason;
    for (unsigned sequenceId : used)
    {
        if (reason.empty() && !seen.insert(sequenceId).second)
        {
            reason = Count("sequenceId", sequenceId) + " delivered twice";
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

/* Service asks the switch only when it is about to make a report, so a
 * report beyond the times it answered on was made without asking. */
std::string EveryReportWasMadeWithReportingOn(const ScenarioState& state)
{
    size_t reports = state.Bench.Reports().size();
    return (reports <= state.Bench.ReportsMade())
               ? std::string()
               : (Count("reports", reports) + " " + Count("made with reporting on", state.Bench.ReportsMade()));
}

/* A kind raised many times before Service takes it is one report, so a kind
 * reported more often than it was raised is a report repeating itself. */
std::string EveryReportStandsForARaisedEvent(const ScenarioState& state)
{
    std::map<std::string, size_t> raised;
    for (const ScenarioEvent& event : state.Bench.Events())
    {
        raised[NameOf(event.Category)]++;
    }
    std::map<std::string, size_t> reported;
    std::string reason;
    for (const ScenarioReport& report : state.Bench.Reports())
    {
        reported[report.Category]++;
        if (reason.empty() && (reported[report.Category] > raised[report.Category]))
        {
            reason = report.Category + " " + Count("reported", reported[report.Category]) + " " +
                     Count("raised", raised[report.Category]);
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

/* One DELIVERY_FAILED opens an outage and one RESTORED closes it; a pass
 * that put a send to a refusing sender is inside an open outage, and one
 * that put a send through an accepting sender is outside any. The edges, not
 * the script, say which run the sender is in: a sender taken up and down
 * again with nothing sent between is still the one outage. */
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
    bool sent = state.Bench.SendsInLastPass() > 0U;
    if (reason.empty() && sent && state.SenderDown && !failed)
    {
        reason = "a send was refused without DELIVERY_FAILED";
    }
    if (reason.empty() && sent && !state.SenderDown && failed)
    {
        reason = "a send was accepted without RESTORED";
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

/* STORE_FROZEN is raised only by a failed device, in a pass that touched
 * it, once the store has touched it, and at most once. The device fails for
 * good, so a freeze reported at any time since is the one for this run.
 * STORE_WRITE_FAILED is the security policy refusing to seal a record, which
 * the bench cannot provoke, so it has no invariant. */
std::string StoreFaultsFollowTheDevice(const ScenarioState& state)
{
    std::string reason;
    const std::vector<ScenarioEvent>& events = state.Bench.Events();
    bool used = state.Bench.DeviceOperationsInLastPass() > 0U;
    for (size_t i = state.EventsJudged; reason.empty() && (i < events.size()); i++)
    {
        bool frozen = events[i].Category == SOLIDSYSLOG_CAT_STORE_FROZEN;
        if (frozen && !state.DeviceFailed)
        {
            reason = "STORE_FROZEN with the device working";
        }
        if (frozen && !used)
        {
            reason = "STORE_FROZEN in a pass that did not use the device";
        }
    }
    size_t reported = CountCategory(events, SOLIDSYSLOG_CAT_STORE_FROZEN);
    if (reason.empty() && state.DeviceFailed && used && (reported == 0U))
    {
        reason = "the failed device was used without STORE_FROZEN";
    }
    if (reason.empty() && (reported > 1U))
    {
        reason = "STORE_FROZEN more than once";
    }
    return reason;
}

/* A latched report stands for a run, so the same report twice in one pass
 * is a latch that did not hold - unless the policy changed between them. A
 * record kept ends a run of refusals and a refusal ends a run of evictions,
 * so each change can start one run more before the next pass. */
std::string ALossIsReportedOncePerRun(const ScenarioState& state)
{
    std::map<std::tuple<const struct SolidSyslogErrorSource*, uint16_t, int32_t>, size_t> seen;
    size_t runsAllowed = 1U + state.PolicyChangesSinceLastPass;
    std::string reason;
    const std::vector<ScenarioEvent>& events = state.Bench.Events();
    for (size_t i = state.EventsJudged; reason.empty() && (i < events.size()); i++)
    {
        size_t raised =
            IsLoss(events[i]) ? ++seen[std::make_tuple(events[i].Source, events[i].Category, events[i].Detail)] : 0U;
        if (raised > runsAllowed)
        {
            reason = Count("category", events[i].Category) + " " +
                     Count("detail", static_cast<size_t>(events[i].Detail)) + " " + Count("raised", raised) +
                     " in one pass " + Count("policy changes", state.PolicyChangesSinceLastPass);
        }
    }
    return reason;
}

bool IsHeldRecordDrop(const ScenarioEvent& event)
{
    return (event.Source == &SolidSyslogErrorSource) && (event.Detail == SOLIDSYSLOG_ERROR_HELD_RECORD_DROPPED);
}

/* Service drops a held record at the start of a pass, and only a record from
 * the buffer that it then deals with ends the run; a report it deals with
 * does not. With no store to take it, a record dealt with is a record
 * delivered, so a drop reported again needs one delivered since the pass of
 * the report before. */
std::string ARunOfHeldDropsIsReportedOnce(const ScenarioState& state)
{
    bool reportedBefore = false;
    size_t reportPass = 0U;
    std::string reason;
    for (const ScenarioEvent& event : state.Bench.Events())
    {
        if (IsHeldRecordDrop(event))
        {
            bool runEnded = true;
            if (reportedBefore)
            {
                runEnded = false;
                for (const ScenarioDelivery& delivery : state.Bench.Delivered())
                {
                    runEnded = runEnded || ((delivery.Pass >= reportPass) && (delivery.Pass < event.Pass));
                }
            }
            if (reason.empty() && !runEnded)
            {
                reason = Count("held record drop reported again at pass", event.Pass) +
                         " with nothing delivered since " + Count("pass", reportPass);
            }
            reportedBefore = true;
            reportPass = event.Pass;
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
    bool stillHalted =
        (state.StatusBeforePass == SOLIDSYSLOG_SERVICE_HALTED) && (state.PolicyChangesSinceLastPass == 0U);
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
    {"every report was made with reporting on", EveryReportWasMadeWithReportingOn},
    {"every report stands for a raised event", EveryReportStandsForARaisedEvent},
    {"every frame is a record", EveryFrameIsARecord},
    {"no universal error", NoUniversalError},
    {"sender edges alternate", SenderEdgesAlternate},
    {"sender edges follow the fault", SenderEdgesFollowTheFault},
    {"store faults follow the device", StoreFaultsFollowTheDevice},
    {"a loss is reported once per run", ALossIsReportedOncePerRun},
    {"a run of held drops is reported once", ARunOfHeldDropsIsReportedOnce},
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

/* A report the store or the sender lost raised a loss of its own; any other
 * report made has reached the sender by the end of the drain. */
std::string EveryReportMadeIsDeliveredOrItsLossReported(const ScenarioState& state)
{
    bool lossReported = false;
    for (const ScenarioEvent& event : state.Bench.Events())
    {
        lossReported = lossReported || IsDownstreamLoss(event);
    }
    bool shortfall = state.Bench.Reports().size() != state.Bench.ReportsMade();
    return (shortfall && !lossReported)
               ? (Count("made", state.Bench.ReportsMade()) + " " + Count("delivered", state.Bench.Reports().size()))
               : std::string();
}

/* The drain runs with reporting on, so once the logger is idle the last
 * event of a reported kind has been taken into a report: an event left
 * waiting at IDLE would sit there until something else woke the loop. */
std::string TheLastEventOfAReportedKindIsReported(const ScenarioState& state)
{
    const std::vector<ScenarioEvent>& events = state.Bench.Events();
    bool waiting = false;
    std::string reason;
    for (size_t i = 0U; i < events.size(); i++)
    {
        if (IsReportedKind(events[i].Category))
        {
            waiting = i >= state.Bench.EventsWhenLastReportMade();
            reason = NameOf(events[i].Category) + " " + Count("event", i) + " never reported";
        }
    }
    return waiting ? reason : std::string();
}

const Invariant AT_END[] = {
    {"the drain ends idle", TheDrainEndsIdle},
    {"every record is delivered or its loss reported", EveryRecordIsDeliveredOrItsLossReported},
    {"every report made is delivered or its loss reported", EveryReportMadeIsDeliveredOrItsLossReported},
    {"the last event of a reported kind is reported", TheLastEventOfAReportedKindIsReported},
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
