#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

#include "BlockDeviceFake.h"
#include "Environment.h"
#include "SolidSyslogBlockDevice.h"
#include "SolidSyslogCrc16Policy.h"
#include "StoreBench.h"
#include "StoreSweep.h"
#include "CppUTest/TestHarness.h"

namespace
{
const size_t SMOKE_BUDGET = 50U;
const size_t DRAIN_PASSES = 20U;
/* A tripwire, not a budget: the drain of one block is a few hundred
 * operations, and a call that spins passes this at once. */
const size_t OPERATION_BOUND = 2000U;
const unsigned FRESH_SEQUENCE = 1000U;
const size_t BITS_PER_BYTE = 8U;
const size_t RECORD_SIZE = 32U;
const size_t BLOCK_SPACE = 100U;
const size_t FAILURES_PRINTED = 8U;

/* A populated store the fault is applied to. */
struct Scenario
{
    const char* Name;
    size_t BlockSize;
    size_t MaxBlocks;
    size_t Records; /* written before the fault */
    size_t Delivered; /* of those, delivered and marked sent */
};

const Scenario SCENARIOS[] = {
    {"one-block", 1024U, 2U, 5U, 0U},
    {"two-blocks-first-part-sent", 512U, 3U, 20U, 3U},
    {"three-full-blocks", 512U, 4U, 39U, 0U},
};

struct Policy
{
    const char* Name;
    bool Strong; /* detects every single-bit and stuck-byte fault */
};

const Policy POLICIES[] = {
    {"null", false},
    {"crc16", true},
};

struct SolidSyslogSecurityPolicy* PolicyFor(const Policy& policy)
{
    return policy.Strong ? SolidSyslogCrc16Policy_Create() : nullptr;
}

const enum BlockDeviceFakeAbsentBlockBehaviour ABSENT_BEHAVIOURS[] = {
    BLOCKDEVICEFAKE_ABSENT_FAILS,
    BLOCKDEVICEFAKE_ABSENT_IS_CREATED_ON_TOUCH,
    BLOCKDEVICEFAKE_ABSENT_IS_GHOST,
};

const size_t ABSENT_BEHAVIOUR_COUNT = sizeof(ABSENT_BEHAVIOURS) / sizeof(ABSENT_BEHAVIOURS[0]);

/* The tuple that names a case: what the failure prints, and what the filter
 * matches. Offset and N mean what the fault class says: a byte and a bit, a
 * size, an operation count and a behaviour. */
struct Case
{
    const Scenario* Scene;
    const Policy* Pol;
    const char* Fault;
    size_t Block;
    size_t Offset;
    size_t N;
};

std::string Describe(const Case& c)
{
    return std::string("scenario=") + c.Scene->Name + " policy=" + c.Pol->Name + " fault=" + c.Fault +
           " block=" + std::to_string(c.Block) + " offset=" + std::to_string(c.Offset) + " n=" + std::to_string(c.N);
}

StoreBenchOptions OptionsFor(const Case& c)
{
    StoreBenchOptions options = {c.Scene->BlockSize, c.Scene->MaxBlocks, PolicyFor(*c.Pol), RECORD_SIZE};
    return options;
}

void Populate(StoreBench& bench, const Scenario& scene)
{
    for (unsigned sequence = 1U; sequence <= scene.Records; sequence++)
    {
        bench.Write(sequence);
    }
    bench.Deliver(scene.Delivered);
}

/* A verdict that failed: which, and the detail for this case. */
struct Verdict
{
    const char* Kind; /* nullptr when every verdict held */
    std::string Detail;
};

const char* const KIND_UNBOUNDED_WORK = "unbounded-work";
const char* const KIND_SILENT_LOSS = "silent-loss";
const char* const KIND_LOSS_OUTSIDE_BLOCK = "loss-outside-block";
const char* const KIND_ILLEGAL_WRITE = "illegal-write";
const char* const KIND_UNDETECTED_DAMAGE = "undetected-damage";
const std::vector<const char*> KINDS = {
    KIND_UNBOUNDED_WORK,
    KIND_SILENT_LOSS,
    KIND_LOSS_OUTSIDE_BLOCK,
    KIND_ILLEGAL_WRITE,
    KIND_UNDETECTED_DAMAGE,
};

/* Whether the policy is expected to catch this fault class: one changed byte
 * on the media. A failed read or a vanished block is not damage it can see. */
bool IsSingleByteDamage(const char* fault)
{
    return (std::string(fault) == "flip-bit") || (std::string(fault) == "stuck-ff") ||
           (std::string(fault) == "stuck-00");
}

/* Whether the fault takes the block's own records with it, so their loss is
 * inherent and only loss beyond the block is the store's doing. */
bool BlockIsGone(const char* fault)
{
    return (std::string(fault) == "absent-at-open") || (std::string(fault) == "removed-after-operation");
}

bool IsTornAppend(const char* fault)
{
    return std::string(fault) == "torn-append";
}

/* Two faults no record format of this shape can see, accepted as limits of
 * the design rather than failures of the store (E25, 2026-11-01): a block
 * cut exactly at a record boundary, which leaves nothing to say the records
 * after it ever existed; and a sent-flag byte stuck at the value that means
 * sent, which is a sent record to any reader. */
bool IsUndetectableLoss(const Case& c)
{
    bool truncatedAtBoundary = (std::string(c.Fault) == "truncate") && StoreBench::IsRecordBoundary(c.Block, c.Offset);
    bool flagStuckAtSent = (std::string(c.Fault) == "stuck-00") && StoreBench::IsRecordFlagByte(c.Block, c.Offset);
    return truncatedAtBoundary || flagStuckAtSent;
}

/* A write the store could not have known was wrong, accepted as a limit of
 * the Null policy rather than a failure of the store (E25, 2026-11-01): with
 * nothing to verify a record's length, a damaged length byte places the sent
 * mark wherever it says. Only a policy's verdict makes a length trustworthy
 * (S25.02), and the loss that follows is reported and stays in the block. */
bool IsUnverifiableWrite(const Case& c)
{
    return !c.Pol->Strong && IsSingleByteDamage(c.Fault) && StoreBench::IsRecordLengthByte(c.Block, c.Offset);
}

/* The silent losses the store answers for. A block that is gone takes its
 * own records with it; a torn append loses the one record it was writing and
 * nothing else in the block, since everything before the tear is whole. */
std::vector<unsigned> SilentLossesChargedTo(const StoreBench& bench, const Case& c)
{
    std::vector<unsigned> silent;

    if (BlockIsGone(c.Fault))
    {
        silent = bench.SilentlyLostOutsideBlock(c.Block);
    }
    else if (IsUndetectableLoss(c))
    {
        /* Nothing is charged. */
    }
    else
    {
        silent = bench.SilentlyLost();
        if (IsTornAppend(c.Fault))
        {
            silent.erase(std::remove(silent.begin(), silent.end(), FRESH_SEQUENCE), silent.end());
        }
    }

    return silent;
}

Verdict Judge(const StoreBench& bench, const Case& c)
{
    Verdict verdict = {nullptr, ""};
    std::vector<unsigned> silent = SilentLossesChargedTo(bench, c);

    /* A store that has reported itself frozen has stopped using the media on
     * purpose (E43), and media that fails again at the next start is never
     * used again: failed media is not worked around, accepted as the design
     * 2026-11-01. What the store still holds is then stranded rather than
     * lost, so after a freeze the block bound counts only records that are
     * gone from the media. A record disposed of or overwritten is the store's
     * doing whether it froze afterwards or not. */
    std::vector<unsigned> outside =
        (bench.FreezeReports() > 0U) ? bench.GoneOutsideBlock(c.Block) : bench.LostOutsideBlock(c.Block);

    if (bench.MaxOperationsInOneCall() > OPERATION_BOUND)
    {
        verdict.Kind = KIND_UNBOUNDED_WORK;
        verdict.Detail = "a store call made " + std::to_string(bench.MaxOperationsInOneCall()) + " device operations";
    }
    else if (!silent.empty())
    {
        verdict.Kind = KIND_SILENT_LOSS;
        verdict.Detail =
            "sequence " + std::to_string(silent[0]) + " was kept, never delivered, and nothing was reported";
    }
    else if (!outside.empty())
    {
        verdict.Kind = KIND_LOSS_OUTSIDE_BLOCK;
        verdict.Detail = "sequence " + std::to_string(outside[0]) + " was lost outside the faulted block";
    }
    else if (!StoreBench::IllegalWrites().empty() && !IsUnverifiableWrite(c))
    {
        BlockDeviceFakeWrite write = StoreBench::IllegalWrites()[0];
        verdict.Kind = KIND_ILLEGAL_WRITE;
        verdict.Detail = "the store wrote " + std::to_string(write.Count) + " byte(s) at block " +
                         std::to_string(write.Block) + " offset " + std::to_string(write.Offset) +
                         ", which is not a record's flag";
    }
    else if (c.Pol->Strong && IsSingleByteDamage(c.Fault) && (bench.Garbage() > 0U))
    {
        verdict.Kind = KIND_UNDETECTED_DAMAGE;
        verdict.Detail = "a record the policy should have rejected was delivered";
    }
    else
    {
        /* Every verdict held. */
    }

    return verdict;
}

/* When the fault is applied. Most faults are on media the store left behind
 * and are followed by a restart; power loss cuts the fresh write itself. */
enum When
{
    BEFORE_RESTART,
    ON_FRESH_WRITE
};

void PrintAccount(const StoreBench& bench);

/* Builds the scenario, applies the fault through @p apply, restarts, writes
 * one fresh record, drains, then restarts and drains once more - a frozen
 * store keeps its records for the next start, so it gets one - and judges. */
template <typename Apply> Verdict RunCase(const Case& c, When when, Apply apply)
{
    StoreBench bench(OptionsFor(c));
    Case judged = c; /* the tuple names the case; the fault's block is resolved below */
    Populate(bench, *c.Scene);

    if (when == BEFORE_RESTART)
    {
        apply(c);
        bench.MarkFaultApplied();
        bench.Restart();
        bench.Write(FRESH_SEQUENCE);
    }
    else
    {
        apply(c);
        bench.MarkFaultApplied();
        bench.Write(FRESH_SEQUENCE); /* the write the power loss cuts */
        judged.Block = BlockDeviceFake_Write(BlockDeviceFake_WriteCount() - 1U).Block;
        bench.Restart();
    }

    bench.Drain(DRAIN_PASSES);
    bench.Restart();
    bench.Drain(DRAIN_PASSES);

    if (StoreSweep_CaseFilter() == Describe(c))
    {
        PrintAccount(bench);
    }

    return Judge(bench, judged);
}

std::string Join(const std::vector<unsigned>& sequences)
{
    std::string joined;

    for (unsigned sequence : sequences)
    {
        joined += (joined.empty() ? "" : " ") + std::to_string(sequence);
    }

    return joined;
}

void Say(const std::string& text)
{
    (void) std::fputs(text.c_str(), stdout);
}

/* What the bench recorded, for the one case the filter named. */
void PrintAccount(const StoreBench& bench)
{
    Say("  delivered: " + Join(bench.Delivered()) + "\n");
    Say("  lost: " + Join(bench.Lost()) + "\n");
    Say("  loss reports=" + std::to_string(bench.LossReports()) + " (frozen=" + std::to_string(bench.FreezeReports()) +
        ") garbage=" + std::to_string(bench.Garbage()) + " duplicates=" + std::to_string(bench.Duplicates()) +
        " max operations in one call=" + std::to_string(bench.MaxOperationsInOneCall()) +
        " bytes written after fault=" + std::to_string(bench.BytesWrittenAfterFault()) + "\n");
    for (const BlockDeviceFakeWrite& write : StoreBench::IllegalWrites())
    {
        Say("  illegal write: block=" + std::to_string(write.Block) + " offset=" + std::to_string(write.Offset) +
            " count=" + std::to_string(write.Count) + "\n");
    }
    std::string present = "  blocks present:";
    for (size_t block = 0U; block < BLOCK_SPACE; block++)
    {
        size_t size = 0U;
        if (SolidSyslogBlockDevice_Exists(bench.Device(), block) &&
            SolidSyslogBlockDevice_Size(bench.Device(), block, &size))
        {
            present += " " + std::to_string(block) + "(" + std::to_string(size) + ")";
        }
    }
    Say(present + "\n");
}

/* The positions a fault class is swept over: for each block the scenario
 * populated, how many offsets and how many variants per offset. */
struct Grid
{
    size_t Blocks;
    std::vector<size_t> Offsets;
    size_t Variants;
};

/* How much data each block holds once the scenario is built. */
Grid LayoutOf(const Case& c, size_t variants)
{
    Grid grid = {};
    grid.Offsets.assign(BLOCK_SPACE, 0U);
    grid.Variants = variants;
    StoreBench bench(OptionsFor(c));
    Populate(bench, *c.Scene);

    for (size_t block = 0U; block < BLOCK_SPACE; block++)
    {
        size_t size = 0U;
        if (SolidSyslogBlockDevice_Exists(bench.Device(), block) &&
            SolidSyslogBlockDevice_Size(bench.Device(), block, &size))
        {
            grid.Offsets[block] = size;
            grid.Blocks = block + 1U;
        }
    }

    return grid;
}

/* One position per block. */
Grid BlocksOf(const Case& c, size_t variants)
{
    Grid grid = LayoutOf(c, variants);

    for (size_t block = 0U; block < grid.Blocks; block++)
    {
        grid.Offsets[block] = 1U;
    }

    return grid;
}

/* How many media operations a clean run of the case makes after the fault
 * point, so a removal can be placed after each one. The operations that
 * populate the scenario come before that point and are not positions. */
Grid OperationsOf(const Case& c, size_t variants)
{
    Grid grid = BlocksOf(c, variants);
    size_t populating = 0U;
    {
        StoreBench bench(OptionsFor(c));
        Populate(bench, *c.Scene);
        populating = BlockDeviceFake_OperationCount();
    }
    RunCase(c, BEFORE_RESTART, [](const Case&) {});
    size_t operations = BlockDeviceFake_OperationCount() - populating;

    for (size_t block = 0U; block < grid.Blocks; block++)
    {
        grid.Offsets[block] = operations;
    }

    return grid;
}

size_t GridTotal(const Grid& grid)
{
    size_t total = 0U;

    for (size_t block = 0U; block < grid.Blocks; block++)
    {
        total += grid.Offsets[block] * grid.Variants;
    }

    return total;
}

void PlaceInGrid(const Grid& grid, size_t index, Case& c)
{
    size_t remaining = index;
    c.Block = 0U;

    while (remaining >= (grid.Offsets[c.Block] * grid.Variants))
    {
        remaining -= grid.Offsets[c.Block] * grid.Variants;
        c.Block++;
    }

    c.Offset = remaining / grid.Variants;
    c.N = remaining % grid.Variants;
}

size_t casesRun = 0U;

/* Collects failing cases and fails the test once at the end, so a known flaw
 * early in a class does not hide what follows it. */
class Failures
{
  public:
    void Add(const Case& c, const Verdict& verdict)
    {
        if (count < FAILURES_PRINTED)
        {
            message += "\n\t" + Describe(c) + ": " + verdict.Kind + ": " + verdict.Detail;
        }
        for (size_t kind = 0U; kind < KINDS.size(); kind++)
        {
            if (KINDS[kind] == verdict.Kind)
            {
                byKind[kind]++;
            }
        }
        count++;
    }

    void Report() const
    {
        if (count > 0U)
        {
            std::string summary = std::to_string(count) + " of " + std::to_string(casesRun) + " cases failed:";
            for (size_t kind = 0U; kind < KINDS.size(); kind++)
            {
                if (byKind[kind] > 0U)
                {
                    summary += std::string(" ") + KINDS[kind] + "=" + std::to_string(byKind[kind]);
                }
            }
            summary += message;
            FAIL(summary.c_str());
        }
    }

  private:
    size_t count = 0U;
    std::vector<size_t> byKind = std::vector<size_t>(KINDS.size(), 0U);
    std::string message;
};

/* Runs @p apply at every grid position the budget admits, or the one the
 * filter names, for every scenario and policy. */
template <typename GridOf, typename Apply>
void Sweep(const char* fault, size_t variants, GridOf gridOf, When when, Apply apply)
{
    std::string filter = StoreSweep_CaseFilter();
    size_t budget = StoreSweep_Budget();
    Failures failures;

    for (const Scenario& scene : SCENARIOS)
    {
        for (const Policy& pol : POLICIES)
        {
            Case c = {&scene, &pol, fault, 0U, 0U, 0U};
            Grid grid = gridOf(c, variants);
            size_t total = GridTotal(grid);
            size_t stride = ((budget == 0U) || !filter.empty()) ? 1U : ((total + budget - 1U) / budget);

            for (size_t index = 0U; index < total; index += ((stride == 0U) ? 1U : stride))
            {
                PlaceInGrid(grid, index, c);

                if (filter.empty() || (filter == Describe(c)))
                {
                    casesRun++;
                    Verdict verdict = RunCase(c, when, apply);
                    if (verdict.Kind != nullptr)
                    {
                        failures.Add(c, verdict);
                    }
                }
            }
        }
    }

    failures.Report();
}
} // namespace

size_t StoreSweep_Budget()
{
    const char* value = Environment_Get("SOLIDSYSLOG_STORE_SWEEP_BUDGET");
    return (value != nullptr) ? static_cast<size_t>(std::strtoul(value, nullptr, 10)) : SMOKE_BUDGET;
}

std::string StoreSweep_CaseFilter()
{
    const char* value = Environment_Get("SOLIDSYSLOG_STORE_SWEEP_CASE");
    return (value != nullptr) ? std::string(value) : std::string();
}

// clang-format off
TEST_GROUP(StoreSweep)
{
    void teardown() override
    {
        Say(std::string("  ") + UtestShell::getCurrent()->getName().asCharString() + ": " + std::to_string(casesRun) + " cases\n");
        casesRun = 0U;
    }
};

// clang-format on

/* offset is the byte, n the bit. */
TEST(StoreSweep, ASingleFlippedBit)
{
    Sweep(
        "flip-bit",
        BITS_PER_BYTE,
        LayoutOf,
        BEFORE_RESTART,
        [](const Case& c) { BlockDeviceFake_FlipBit(c.Block, c.Offset, static_cast<unsigned>(c.N)); }
    );
}

/* offset is the byte. */
TEST(StoreSweep, AByteStuckAtFF)
{
    Sweep(
        "stuck-ff",
        1U,
        LayoutOf,
        BEFORE_RESTART,
        [](const Case& c) { BlockDeviceFake_StickByte(c.Block, c.Offset, 0xFFU); }
    );
}

TEST(StoreSweep, AByteStuckAtZero)
{
    Sweep(
        "stuck-00",
        1U,
        LayoutOf,
        BEFORE_RESTART,
        [](const Case& c) { BlockDeviceFake_StickByte(c.Block, c.Offset, 0x00U); }
    );
}

/* offset is the byte no read may touch. */
TEST(StoreSweep, AReadThatFailsAtOneOffset)
{
    Sweep(
        "read-fail",
        1U,
        LayoutOf,
        BEFORE_RESTART,
        [](const Case& c) { BlockDeviceFake_FailReadsTouching(c.Block, c.Offset); }
    );
}

/* offset is the size the block is cut to. */
TEST(StoreSweep, ABlockTruncated)
{
    Sweep(
        "truncate",
        1U,
        LayoutOf,
        BEFORE_RESTART,
        [](const Case& c) { BlockDeviceFake_TruncateTo(c.Block, c.Offset); }
    );
}

/* n is the absent-block behaviour. */
TEST(StoreSweep, ABlockAbsentAtOpen)
{
    Sweep(
        "absent-at-open",
        ABSENT_BEHAVIOUR_COUNT,
        BlocksOf,
        BEFORE_RESTART,
        [](const Case& c)
        {
            BlockDeviceFake_SetAbsentBlockBehaviour(ABSENT_BEHAVIOURS[c.N]);
            BlockDeviceFake_RemoveBlock(c.Block);
        }
    );
}

TEST(StoreSweep, ABlockThatFailsEveryOperation)
{
    Sweep(
        "fails-every-operation",
        1U,
        BlocksOf,
        BEFORE_RESTART,
        [](const Case& c) { BlockDeviceFake_FailEveryOperationOn(c.Block); }
    );
}

/* offset is the operation after which the block goes, n the absent-block
 * behaviour. */
TEST(StoreSweep, ABlockRemovedWhileRunning)
{
    Sweep(
        "removed-after-operation",
        ABSENT_BEHAVIOUR_COUNT,
        OperationsOf,
        BEFORE_RESTART,
        [](const Case& c)
        {
            BlockDeviceFake_SetAbsentBlockBehaviour(ABSENT_BEHAVIOURS[c.N]);
            BlockDeviceFake_RemoveBlockAfterOperation(c.Block, BlockDeviceFake_OperationCount() + c.Offset + 1U);
        }
    );
}

/* offset is how many bytes of the fresh record land; the tuple's block is the
 * layout's last, and the case is judged in the block the write landed in,
 * which is the next one when the layout's last is full. */
TEST(StoreSweep, PowerLossDuringTheWrite)
{
    Sweep(
        "torn-append",
        1U,
        [](const Case& c, size_t variants)
        {
            Grid grid = LayoutOf(c, variants);
            size_t writeBlock = grid.Blocks - 1U;
            for (size_t block = 0U; block < grid.Blocks; block++)
            {
                grid.Offsets[block] = (block == writeBlock) ? (RECORD_SIZE + 8U) : 0U;
            }
            return grid;
        },
        ON_FRESH_WRITE,
        [](const Case& c) { BlockDeviceFake_TearNextAppend(c.Offset); }
    );
}
