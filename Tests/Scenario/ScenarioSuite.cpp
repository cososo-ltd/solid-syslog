#include "ScenarioSuite.h"

#include <cstdint>
#include <cstdlib>
#include <array>
#include <random>
#include <string>

#include "Environment.h"
#include "ScenarioInvariants.h"
#include "SolidSyslogDiscardPolicy.h"
#include "SolidSyslogServiceStatus.h"

namespace
{
const size_t SMOKE_RUNS = 25U;
const size_t WALK_STEPS = 300U;
/* Passes the drain allows beyond one per record before it is a spin. */
const size_t DRAIN_SLACK = 50U;

const std::array<ScenarioRig, 6> RIGS = {
    {SCENARIO_RIG_OLDEST,
     SCENARIO_RIG_NEWEST,
     SCENARIO_RIG_HALT,
     SCENARIO_RIG_OLDEST_NO_STORE,
     SCENARIO_RIG_NEWEST_NO_STORE,
     SCENARIO_RIG_HALT_NO_STORE}
};

/* A step's letter and its weight in a thousand draws. Logging outruns
 * Service, outages are short and recover, a device failure is rare enough
 * that most walks never see one, a misrouted sender and a failing security
 * policy are rare and soon put right, and reporting is mostly on. */
struct Step
{
    char Letter;
    unsigned Weight;
};

const Step STEPS[] = {
    {'L', 396U},
    {'M', 8U},
    {'B', 12U},
    {'K', 8U},
    {'P', 4U},
    {'T', 2U},
    {'W', 10U},
    {'S', 335U},
    {'D', 40U},
    {'U', 60U},
    {'F', 1U},
    {'O', 33U},
    {'N', 33U},
    {'H', 33U},
    {'Q', 10U},
    {'R', 15U},
};
const unsigned WEIGHT_TOTAL = 1000U;

/* The script driver: applies steps to the bench and keeps the state the
 * invariants judge. */
class Driver
{
  public:
    explicit Driver(const ScenarioRig& rig)
        : bench(rig.Options), state{
                                  bench,
                                  false,
                                  false,
                                  rig.Options.DiscardPolicy,
                                  (rig.Options.DiscardPolicy == SOLIDSYSLOG_DISCARD_POLICY_HALT) ? 1U : 0U,
                                  0U,
                                  SOLIDSYSLOG_SERVICE_IDLE,
                                  SOLIDSYSLOG_SERVICE_IDLE,
                                  0U,
                                  0U,
                                  false,
                                  SECURITY_POLICY_FAKE_WORKING
                              }
    {
    }

    ScenarioVerdict Run(const std::string& script)
    {
        ScenarioVerdict verdict = {true, "", "", 0U};
        for (size_t i = 0U; verdict.Passed && (i < script.size()); i++)
        {
            verdict = Apply(script[i]);
        }
        return verdict.Passed ? Drain() : verdict;
    }

  private:
    ScenarioBench bench;
    ScenarioState state;

    ScenarioVerdict Apply(char step)
    {
        ScenarioVerdict verdict = {true, "", "", bench.Passes()};
        state.QuietSinceLastPass = state.QuietSinceLastPass && (step == 'S');
        switch (step)
        {
            case 'L':
                bench.Log();
                break;
            case 'S':
                verdict = Pass();
                break;
            case 'D':
                SetSender(true);
                break;
            case 'U':
                SetSender(false);
                break;
            case 'F':
                bench.FailDevice();
                state.DeviceFailed = true;
                break;
            case 'C':
                /* Scripts only: the ring never gets past a corrupt record, so
                 * a walk would strand everything logged after it. */
                bench.CorruptTheRing();
                break;
            case 'M':
                bench.SetMisrouted(true);
                break;
            case 'B':
                bench.SetMisrouted(false);
                break;
            case 'K':
                SetSecurityPolicyFault(SECURITY_POLICY_FAKE_KEY_WITHHELD);
                break;
            case 'P':
                SetSecurityPolicyFault(SECURITY_POLICY_FAKE_BACKEND_FAILING);
                break;
            case 'T':
                SetSecurityPolicyFault(SECURITY_POLICY_FAKE_REJECTING);
                break;
            case 'W':
                SetSecurityPolicyFault(SECURITY_POLICY_FAKE_WORKING);
                break;
            case 'O':
                SetPolicy(SOLIDSYSLOG_DISCARD_POLICY_OLDEST);
                break;
            case 'N':
                SetPolicy(SOLIDSYSLOG_DISCARD_POLICY_NEWEST);
                break;
            case 'H':
                SetPolicy(SOLIDSYSLOG_DISCARD_POLICY_HALT);
                break;
            case 'Q':
                bench.SetReporting(false);
                break;
            case 'R':
                bench.SetReporting(true);
                break;
            default:
                verdict.Passed = false;
                verdict.Invariant = "the script is well formed";
                verdict.Reason = std::string("unknown step '") + step + "'";
                break;
        }
        return verdict;
    }

    void SetSender(bool down)
    {
        bench.SetSenderDown(down);
        state.SenderDown = down;
    }

    void SetSecurityPolicyFault(enum SecurityPolicyFakeFault fault)
    {
        bench.SetSecurityPolicyFault(fault);
        state.SecurityPolicyFault = fault;
    }

    void SetPolicy(enum SolidSyslogDiscardPolicy policy)
    {
        bench.SetDiscardPolicy(policy);
        state.PolicyChangesSinceLastPass += (policy != state.Policy) ? 1U : 0U;
        state.HaltPeriods += ((policy == SOLIDSYSLOG_DISCARD_POLICY_HALT) && (policy != state.Policy)) ? 1U : 0U;
        state.Policy = policy;
    }

    ScenarioVerdict Pass()
    {
        state.StatusBeforePass = state.Status;
        state.DeliveredBeforePass = bench.Delivered().size();
        state.Status = bench.Service();
        ScenarioVerdict verdict = ScenarioInvariants_CheckAfterPass(state);
        state.EventsJudged = bench.Events().size();
        state.PolicyChangesSinceLastPass = 0U;
        state.QuietSinceLastPass = true;
        return verdict;
    }

    /* What the script left queued goes out under the kindest conditions,
     * so the end-of-run accounting can be judged. */
    ScenarioVerdict Drain()
    {
        SetSender(false);
        SetPolicy(SOLIDSYSLOG_DISCARD_POLICY_OLDEST);
        bench.SetReporting(true);
        bench.SetMisrouted(false);
        SetSecurityPolicyFault(SECURITY_POLICY_FAKE_WORKING);
        state.QuietSinceLastPass = false;
        ScenarioVerdict verdict = {true, "", "", bench.Passes()};
        size_t allowance = bench.Logged() + DRAIN_SLACK;
        bool idle = false; /* the script's last status says nothing about what it logged since */
        for (size_t i = 0U; verdict.Passed && !idle && (i < allowance); i++)
        {
            verdict = Pass();
            idle = state.Status == SOLIDSYSLOG_SERVICE_IDLE;
        }
        return verdict.Passed ? ScenarioInvariants_CheckAtEnd(state) : verdict;
    }
};
} // namespace

ScenarioVerdict ScenarioSuite_Run(const ScenarioRig& rig, const std::string& script)
{
    Driver driver(rig);
    return driver.Run(script);
}

const ScenarioRig& ScenarioSuite_RigForSeed(uint32_t seed)
{
    return RIGS.at(seed % RIGS.size());
}

std::string ScenarioSuite_ScriptForSeed(uint32_t seed)
{
    std::mt19937 generator(seed);
    std::string script;
    for (size_t i = 0U; i < WALK_STEPS; i++)
    {
        auto draw = static_cast<unsigned>(generator() % WEIGHT_TOTAL);
        for (const Step& step : STEPS)
        {
            if (draw < step.Weight)
            {
                script += step.Letter;
                draw = WEIGHT_TOTAL; /* chosen; no later step can match */
            }
            else
            {
                draw -= step.Weight;
            }
        }
    }
    return script;
}

std::string ScenarioSuite_Describe(const ScenarioRig& rig, const std::string& script, const ScenarioVerdict& verdict)
{
    return "rig=" + std::string(rig.Name) + " pass=" + std::to_string(verdict.Pass) + " invariant=\"" +
           verdict.Invariant + "\" reason=\"" + verdict.Reason + "\" script=" + script;
}

/* A seed is the decimal a failure printed, whole and within uint32_t;
 * anything else is not one, so a typo cannot narrow a lane to one walk. */
bool ScenarioSuite_SeedGiven(uint32_t* seed)
{
    const char* value = Environment_Get("SOLIDSYSLOG_SCENARIO_SEED");
    bool digits = (value != nullptr) && (*value != '\0');
    for (const char* c = value; digits && (*c != '\0'); c++)
    {
        digits = (*c >= '0') && (*c <= '9');
    }
    unsigned long long parsed = digits ? std::strtoull(value, nullptr, 10) : 0ULL;
    bool given = digits && (parsed <= UINT32_MAX);
    *seed = given ? static_cast<uint32_t>(parsed) : 0U;
    return given;
}

/* A count that is unset, malformed or zero is the smoke count: a typo in a
 * lane's environment must not switch the walks off. */
size_t ScenarioSuite_Runs()
{
    const char* value = Environment_Get("SOLIDSYSLOG_SCENARIO_RUNS");
    char* end = nullptr;
    size_t runs = (value != nullptr) ? static_cast<size_t>(std::strtoul(value, &end, 10)) : 0U;
    bool whole = (value != nullptr) && (*value != '\0') && (*end == '\0');
    return (whole && (runs > 0U)) ? runs : SMOKE_RUNS;
}
