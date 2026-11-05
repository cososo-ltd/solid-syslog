#include "ScenarioInvariants.h"

#include <map>
#include <set>
#include <string>
#include <tuple>
#include <vector>

#include "SolidSyslogBlockStoreErrors.h"
#include "SolidSyslogBufferCategories.h"
#include "SolidSyslogErrorCategory.h"
#include "SolidSyslogErrors.h"
#include "SolidSyslogFileCategories.h"
#include "SolidSyslogSecurityPolicyCategories.h"
#include "SolidSyslogSenderCategories.h"
#include "SolidSyslogStoreCategories.h"

namespace
{
/* A pass moves at most what the ring held, the record held from the last
 * pass, a report of each of the fourteen kinds Service reports and one record
 * from the store, with two sends and a few device operations per record.
 * Tripwires, not budgets: a call that spins passes them at once. */
const size_t REPORTS_PER_PASS = 14U;
const size_t RECORDS_BEYOND_THE_RING = 1U + REPORTS_PER_PASS + 1U;

/* The kinds whose last event the end invariant requires reported: those the
 * bench can provoke. */
const uint16_t REPORTED_KINDS[] = {
    SOLIDSYSLOG_CAT_FILE_IO_FAILED,
    SOLIDSYSLOG_CAT_SECURITY_POLICY_KEY_UNAVAILABLE,
    SOLIDSYSLOG_CAT_SECURITY_POLICY_SEAL_FAILED,
    SOLIDSYSLOG_CAT_SECURITY_POLICY_OPEN_FAILED,
    SOLIDSYSLOG_CAT_BUFFER_BACKEND_FAILED,
    SOLIDSYSLOG_CAT_BUFFER_RECORDS_LOST,
    SOLIDSYSLOG_CAT_STORE_RECORDS_LOST,
    SOLIDSYSLOG_CAT_STORE_RECORD_DISCARDED,
    SOLIDSYSLOG_CAT_STORE_WRITE_FAILED,
    SOLIDSYSLOG_CAT_STORE_FROZEN,
    SOLIDSYSLOG_CAT_SENDER_RECORDS_LOST,
    SOLIDSYSLOG_CAT_SENDER_DELIVERY_FAILED,
    SOLIDSYSLOG_CAT_SENDER_DELIVERY_RESTORED,
};
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

/* The categories that account for a record not delivered. A buffer whose
 * backend failed refused a write, losing the record, or met a head record too
 * large to read, which strands it and those behind it. */
bool IsLoss(const ScenarioEvent& event)
{
    return (event.Category == SOLIDSYSLOG_CAT_BUFFER_RECORDS_LOST) ||
           (event.Category == SOLIDSYSLOG_CAT_BUFFER_BACKEND_FAILED) ||
           (event.Category == SOLIDSYSLOG_CAT_SENDER_RECORDS_LOST) ||
           (event.Category == SOLIDSYSLOG_CAT_STORE_RECORDS_LOST) ||
           (event.Category == SOLIDSYSLOG_CAT_STORE_RECORD_DISCARDED) ||
           (event.Category == SOLIDSYSLOG_CAT_STORE_WRITE_FAILED) || (event.Category == SOLIDSYSLOG_CAT_STORE_FROZEN);
}

/* A report never passes through the buffer, and one the store will not take -
 * refused, or lost to its security policy - goes to the sender instead, which
 * on the bench loses nothing it takes. So only the store losing what it held
 * can lose a report: a rotation that evicted unsent records, a record
 * discarded on reading, or the freeze. */
bool CanLoseAReport(const ScenarioEvent& event)
{
    bool evicted = (event.Source == &SolidSyslogBlockStoreErrorSource) &&
                   (event.Detail == SOLIDSYSLOG_BLOCK_STORE_ERROR_BLOCK_EVICTED);
    return evicted || (event.Category == SOLIDSYSLOG_CAT_STORE_RECORD_DISCARDED) ||
           (event.Category == SOLIDSYSLOG_CAT_STORE_FROZEN);
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

/* Every report Service made that the bench saw: those the stream received,
 * and those the misrouted sender dropped. */
std::vector<ScenarioReport> MadeReports(const ScenarioState& state)
{
    std::vector<ScenarioReport> made = state.Bench.Reports();
    made.insert(made.end(), state.Bench.DroppedReports().begin(), state.Bench.DroppedReports().end());
    return made;
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
    for (const ScenarioReport& report : MadeReports(state))
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
    size_t reports = state.Bench.Reports().size() + state.Bench.DroppedReports().size();
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
    for (const ScenarioReport& report : MadeReports(state))
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

/* The event a report was made of: the last of its kind raised before Service
 * made the report, while the meta SD had handed out fewer sequenceIds than the
 * report took. A slot holds the most recent event of its kind, so an earlier
 * one is not what the report says. */
const ScenarioEvent* EventReportedBy(const ScenarioState& state, const ScenarioReport& report)
{
    const ScenarioEvent* reported = nullptr;
    for (const ScenarioEvent& event : state.Bench.Events())
    {
        bool before = (NameOf(event.Category) == report.Category) && (event.LastSequenceId < report.SequenceId);
        reported = before ? &event : reported;
    }
    return reported;
}

std::string Params(const std::string& source, const std::string& detail, const std::string& native)
{
    return "source=" + source + " detail=" + detail + " native=" + native;
}

std::string EveryReportSaysWhatItsEventSaid(const ScenarioState& state)
{
    std::string reason;
    for (const ScenarioReport& report : MadeReports(state))
    {
        const ScenarioEvent* event = EventReportedBy(state, report);
        bool said = (event != nullptr) && (event->SourceParam == report.SourceParam) &&
                    (event->DetailParam == report.DetailParam) && (event->NativeParam == report.NativeParam);
        if (reason.empty() && !said)
        {
            reason = report.Category + " " + Count("sequenceId", report.SequenceId) + " says " +
                     Params(report.SourceParam, report.DetailParam, report.NativeParam) +
                     ((event != nullptr)
                          ? (", its event said " + Params(event->SourceParam, event->DetailParam, event->NativeParam))
                          : ", with no event before it");
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
 * good, so a freeze reported at any time since is the one for this run. A
 * device of files fails through its file system, which raises FILE_IO_FAILED,
 * so that too follows the failed device.
 * STORE_WRITE_FAILED follows the security policy instead, judged beside it. */
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
        if ((events[i].Category == SOLIDSYSLOG_CAT_FILE_IO_FAILED) && !state.DeviceFailed)
        {
            reason = "FILE_IO_FAILED with the device working";
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

/* The security policy raises only while it fails, and the store's losses to it
 * follow it: a record it would not seal is lost only while the key is withheld
 * or the backend fails, and one is discarded as rejected only while the policy
 * rejects. */
std::string PolicyFaultsFollowThePolicy(const ScenarioState& state)
{
    enum SecurityPolicyFakeFault fault = state.SecurityPolicyFault;
    bool sealing = (fault == SECURITY_POLICY_FAKE_WORKING) || (fault == SECURITY_POLICY_FAKE_REJECTING);
    std::string reason;
    const std::vector<ScenarioEvent>& events = state.Bench.Events();
    for (size_t i = state.EventsJudged; reason.empty() && (i < events.size()); i++)
    {
        uint16_t category = events[i].Category;
        bool backend = (category == SOLIDSYSLOG_CAT_SECURITY_POLICY_SEAL_FAILED) ||
                       (category == SOLIDSYSLOG_CAT_SECURITY_POLICY_OPEN_FAILED);
        bool rejected = (events[i].Source == &SolidSyslogBlockStoreErrorSource) &&
                        (events[i].Detail == SOLIDSYSLOG_BLOCK_STORE_ERROR_RECORD_REJECTED);
        if ((category == SOLIDSYSLOG_CAT_SECURITY_POLICY_KEY_UNAVAILABLE) &&
            (fault != SECURITY_POLICY_FAKE_KEY_WITHHELD))
        {
            reason = "KEY_UNAVAILABLE with the key available";
        }
        if (backend && (fault != SECURITY_POLICY_FAKE_BACKEND_FAILING))
        {
            reason = NameOf(category) + " with the backend working";
        }
        if ((category == SOLIDSYSLOG_CAT_STORE_WRITE_FAILED) && sealing)
        {
            reason = "STORE_WRITE_FAILED with the policy sealing";
        }
        if (rejected && (fault != SECURITY_POLICY_FAKE_REJECTING))
        {
            reason = "a record rejected by a policy that verifies it";
        }
    }
    return reason;
}

bool IsEndedByARecordKept(const ScenarioEvent& event)
{
    return (event.Source == &SolidSyslogBlockStoreErrorSource) &&
           ((event.Detail == SOLIDSYSLOG_BLOCK_STORE_ERROR_RECORD_REFUSED) ||
            (event.Detail == SOLIDSYSLOG_BLOCK_STORE_ERROR_SEAL_FAILED));
}

/* The raises of one report in the pass since its run last ended, and how many
 * records the store had kept at the last of them. */
struct RaisesInRun
{
    size_t Raised;
    size_t KeptAtLast;
};

/* A latched report stands for a run, so the same report twice in one pass
 * is a latch that did not hold - unless the run ended between them. In the
 * buffer a record kept ends a run of refusals and a refusal ends a run of
 * evictions, so each policy change can start one run more before the next
 * pass. In the store a record kept ends a run of refusals or of seal
 * failures, and a pass can keep one between two refusals: a report is a size
 * of its own, so a store with no room for one may still take a logged
 * record. */
std::string ALossIsReportedOncePerRun(const ScenarioState& state)
{
    std::map<std::tuple<const struct SolidSyslogErrorSource*, uint16_t, int32_t>, RaisesInRun> seen;
    std::string reason;
    const std::vector<ScenarioEvent>& events = state.Bench.Events();
    for (size_t i = state.EventsJudged; reason.empty() && (i < events.size()); i++)
    {
        if (IsLoss(events[i]))
        {
            RaisesInRun& run = seen[std::make_tuple(events[i].Source, events[i].Category, events[i].Detail)];
            bool runEnded = IsEndedByARecordKept(events[i]) && (events[i].RecordsKept != run.KeptAtLast);
            run.Raised = runEnded ? 1U : (run.Raised + 1U);
            run.KeptAtLast = events[i].RecordsKept;
            size_t runsAllowed =
                1U +
                ((events[i].Category == SOLIDSYSLOG_CAT_BUFFER_RECORDS_LOST) ? state.PolicyChangesSinceLastPass : 0U);
            if (run.Raised > runsAllowed)
            {
                reason = Count("category", events[i].Category) + " " +
                         Count("detail", static_cast<size_t>(events[i].Detail)) + " " + Count("raised", run.Raised) +
                         " in one pass " + Count("policy changes", state.PolicyChangesSinceLastPass);
            }
        }
    }
    return reason;
}

/* A raise stands for a fault until it clears, so a pass after one in which
 * nothing was logged and nothing changed has no new fault to raise: a category
 * raised in both is one raised again for the same fault - a raise that does
 * not latch, or a report feeding the fault it reports. The one run such a pass
 * can start afresh is the store's refusals or seal failures, which a record
 * kept since the last raise ends. */
std::string QuietPassesGoQuiet(const ScenarioState& state)
{
    std::string reason;
    if (state.QuietSinceLastPass)
    {
        const std::vector<ScenarioEvent>& events = state.Bench.Events();
        size_t pass = state.Bench.Passes();
        std::set<uint16_t> raisedLastPass;
        std::map<std::tuple<const struct SolidSyslogErrorSource*, uint16_t, int32_t>, size_t> keptAtLastRaise;
        for (size_t i = 0U; i < state.EventsJudged; i++)
        {
            if (events[i].Pass == (pass - 1U))
            {
                raisedLastPass.insert(events[i].Category);
                keptAtLastRaise[std::make_tuple(events[i].Source, events[i].Category, events[i].Detail)] =
                    events[i].RecordsKept;
            }
        }
        for (size_t i = state.EventsJudged; reason.empty() && (i < events.size()); i++)
        {
            auto lastRaise =
                keptAtLastRaise.find(std::make_tuple(events[i].Source, events[i].Category, events[i].Detail));
            bool runEnded = IsEndedByARecordKept(events[i]) && (lastRaise != keptAtLastRaise.end()) &&
                            (events[i].RecordsKept != lastRaise->second);
            if ((raisedLastPass.count(events[i].Category) != 0U) && !runEnded)
            {
                reason = NameOf(events[i].Category) + " " + Count("detail", static_cast<size_t>(events[i].Detail)) +
                         " raised again in a quiet " + Count("pass", pass);
            }
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
    {"every report says what its event said", EveryReportSaysWhatItsEventSaid},
    {"every frame is a record", EveryFrameIsARecord},
    {"no universal error", NoUniversalError},
    {"sender edges alternate", SenderEdgesAlternate},
    {"sender edges follow the fault", SenderEdgesFollowTheFault},
    {"store faults follow the device", StoreFaultsFollowTheDevice},
    {"policy faults follow the policy", PolicyFaultsFollowThePolicy},
    {"a loss is reported once per run", ALossIsReportedOncePerRun},
    {"quiet passes go quiet", QuietPassesGoQuiet},
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

/* A report the misrouted sender dropped is one the bench saw dropped; one the
 * store lost raised a loss of its own; any other report made has reached the
 * sender by the end of the drain. Which report a store loss stands for
 * cannot be judged: the store reports a loss once per run, so a report lost
 * inside a run that began before it was made raises nothing of its own. Only
 * whether the store lost anything it held can be, which the rigs with no
 * store make strict. */
std::string EveryReportMadeIsDeliveredOrItsLossReported(const ScenarioState& state)
{
    bool lossReported = false;
    for (const ScenarioEvent& event : state.Bench.Events())
    {
        lossReported = lossReported || CanLoseAReport(event);
    }
    bool shortfall = MadeReports(state).size() != state.Bench.ReportsMade();
    return (shortfall && !lossReported)
               ? (Count("made", state.Bench.ReportsMade()) + " " + Count("delivered", state.Bench.Reports().size()) +
                  " " + Count("dropped", state.Bench.DroppedReports().size()))
               : std::string();
}

/* Service made a report of the kind after the meta SD had handed out
 * @p lastSequenceId, so after the event raised then; it arrived, or the
 * misrouted sender dropped it. */
bool IsReportedAfter(const ScenarioState& state, uint16_t kind, unsigned lastSequenceId)
{
    bool reported = false;
    for (const ScenarioReport& report : MadeReports(state))
    {
        reported = reported || ((report.Category == NameOf(kind)) && (report.SequenceId > lastSequenceId));
    }
    return reported;
}

/* Whether the store could have lost the report of an event raised after the
 * meta SD had handed out @p lastSequenceId. An eviction or a discard reports a
 * run once, so one at any time could have taken it. The freeze loses only what
 * the store held when it froze, so it could only if a sequenceId was handed out
 * between the two: with none, the report took its number after the freeze and
 * went to the sender. */
bool CouldHaveLostTheReport(const std::vector<ScenarioEvent>& events, unsigned lastSequenceId)
{
    bool lost = false;
    for (const ScenarioEvent& event : events)
    {
        bool frozen = event.Category == SOLIDSYSLOG_CAT_STORE_FROZEN;
        bool frozenAfterwards = frozen && (event.LastSequenceId > lastSequenceId);
        lost = lost || frozenAfterwards || (CanLoseAReport(event) && !frozen);
    }
    return lost;
}

/* The drain runs with reporting on and ends idle, so the last event of each
 * reported kind has been taken into a report, which has reached the sender
 * unless the store lost what it held. Which report a loss stands for cannot
 * be judged - the store reports a loss once per run - so any loss that could
 * have taken it excuses. */
std::string TheLastEventOfEachReportedKindIsReported(const ScenarioState& state)
{
    const std::vector<ScenarioEvent>& events = state.Bench.Events();
    std::string reason;
    for (uint16_t kind : REPORTED_KINDS)
    {
        const ScenarioEvent* last = nullptr;
        for (const ScenarioEvent& event : events)
        {
            last = (event.Category == kind) ? &event : last;
        }
        bool unreported = (last != nullptr) && !IsReportedAfter(state, kind, last->LastSequenceId) &&
                          !CouldHaveLostTheReport(events, last->LastSequenceId);
        if (reason.empty() && unreported)
        {
            reason = NameOf(kind) + " raised after " + Count("sequenceId", last->LastSequenceId) + " never reported";
        }
    }
    return reason;
}

const Invariant AT_END[] = {
    {"the drain ends idle", TheDrainEndsIdle},
    {"every record is delivered or its loss reported", EveryRecordIsDeliveredOrItsLossReported},
    {"every report made is delivered or its loss reported", EveryReportMadeIsDeliveredOrItsLossReported},
    {"the last event of each reported kind is reported", TheLastEventOfEachReportedKindIsReported},
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
