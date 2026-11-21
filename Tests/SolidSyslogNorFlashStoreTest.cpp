#include <algorithm>
#include <cstddef>
#include <string>
#include <vector>

#include "ErrorHandlerFake.h"
#include "NorFlashFake.h"
#include "SolidSyslogBlockStore.h"
#include "SolidSyslogCrc16Policy.h"
#include "SolidSyslogDiscardPolicy.h"
#include "SolidSyslogNorFlashBlockDevice.h"
#include "SolidSyslogNullStore.h"
#include "SolidSyslogStore.h"
#include "SolidSyslogStoreCategories.h"
#include "SolidSyslogStoreDefinition.h"
#include "TestUtils.h"
#include "SolidSyslogTunables.h"
#include "CppUTest/TestHarness.h"

using namespace CososoTesting;

/* The block store over the flash block device over a flash in memory: what a
 * record costs the part, and what a power cut at any program or erase leaves. */

namespace
{
enum SolidSyslogDiscardPolicy AlwaysOldest(void* context)
{
    (void) context;
    return SOLIDSYSLOG_DISCARD_POLICY_OLDEST;
}

const size_t NUMBER_DIGITS = 5;

/* A record of `bytes` bytes that carries its own number. */
std::string RecordNumbered(unsigned number, size_t bytes)
{
    std::string digits = std::to_string(number);
    std::string record = std::string(NUMBER_DIGITS - digits.size(), '0') + digits;
    record.resize(bytes, 'x');
    return record;
}

unsigned NumberOf(const std::string& record)
{
    return static_cast<unsigned>(std::stoul(record.substr(0, NUMBER_DIGITS)));
}

/* The whole store stack over one flash. Restart discards the library's
 * state and builds it again over the same bytes, as a reset does. */
struct NorFlashStoreStack
{
    size_t flashBlockSize = NORFLASHFAKE_MAX_BLOCK_SIZE;
    size_t flashBlockCount = 16;
    size_t storeMaxBlocks = 8;
    size_t deviceBlockSize = 0; /* all a flash block holds */
    SolidSyslogDiscardPolicyFunction discardPolicy = nullptr;
    SolidSyslogNorFlash* flash = nullptr;
    SolidSyslogSecurityPolicy* policy = nullptr;
    SolidSyslogBlockDevice* device = nullptr;
    SolidSyslogStore* store = nullptr;

    void StartOnABlankPart()
    {
        flash = NorFlashFake_Create(flashBlockSize, flashBlockCount);
        Start();
    }

    void Start()
    {
        policy = SolidSyslogCrc16Policy_Create();
        SolidSyslogNorFlashBlockDeviceConfig deviceConfig = {};
        deviceConfig.Flash = flash;
        deviceConfig.FirstBlock = 0;
        deviceConfig.BlockCount = flashBlockCount;
        deviceConfig.BlockSize = deviceBlockSize;
        device = SolidSyslogNorFlashBlockDevice_Create(&deviceConfig);
        SolidSyslogBlockStoreConfig storeConfig = {};
        storeConfig.BlockDevice = device;
        storeConfig.MaxBlocks = storeMaxBlocks;
        storeConfig.SecurityPolicy = policy;
        storeConfig.GetDiscardPolicy = discardPolicy;
        store = SolidSyslogBlockStore_Create(&storeConfig);
    }

    void Stop()
    {
        if (store != nullptr)
        {
            SolidSyslogBlockStore_Destroy(store);
            SolidSyslogNorFlashBlockDevice_Destroy(device);
            SolidSyslogCrc16Policy_Destroy();
            store = nullptr;
            device = nullptr;
            policy = nullptr;
        }
    }

    void Restart()
    {
        Stop();
        NorFlashFake_PowerOn();
        Start();
    }

    [[nodiscard]] bool Write(const std::string& record) const
    {
        return SolidSyslogStore_Write(store, record.data(), record.size()) == SOLIDSYSLOG_STORE_WRITE_KEPT;
    }

    /* Reads the next record without marking it. Empty when there is none. */
    [[nodiscard]] std::string ReadNext() const
    {
        char buffer[SOLIDSYSLOG_MAX_MESSAGE_SIZE] = {};
        size_t bytesRead = 0U;
        bool read = SolidSyslogStore_ReadNextUnsent(store, buffer, sizeof(buffer), &bytesRead);
        return read ? std::string(buffer, bytesRead) : std::string();
    }

    void MarkSent() const
    {
        SolidSyslogStore_MarkSent(store);
    }

    /* Reads and marks until nothing is left to send. A record read twice
         * running is one whose sent mark did not take, and ends the drain as a
         * failure where it would otherwise never end. */
    [[nodiscard]] std::vector<unsigned> Drain() const
    {
        std::vector<unsigned> numbers;
        std::string record = ReadNext();
        while (!record.empty())
        {
            if (!numbers.empty() && (numbers.back() == NumberOf(record)))
            {
                FAIL("the same record was read twice: its sent mark did not take");
            }
            numbers.push_back(NumberOf(record));
            MarkSent();
            record = ReadNext();
        }
        return numbers;
    }
};

NorFlashStoreStack stack;

std::string Listed(const std::vector<unsigned>& numbers)
{
    std::string text;
    for (unsigned number : numbers)
    {
        text += std::to_string(number) + " ";
    }
    return text;
}
} // namespace

enum
{
    RECORD_BYTES = 200,
    /* A 200-byte message is 207 bytes stored: 4 of header, a 2-byte CRC and
     * the sent flag. A 4 KiB flash block holds 4092 after its state. */
    RECORDS_A_BLOCK = 19,
    FLASH_BLOCKS = 16
};

/* `count` of something for `records` records came to between `least` and
 * `most` hundredths each. A macro, so a failure is reported at the test's own
 * line. */
#define CHECK_HUNDREDTHS_A_RECORD(least, most, count, records)                                \
    CHECK_TEXT(                                                                               \
        ((static_cast<size_t>(count) * 100U) >= (static_cast<size_t>(records) * (least))) &&  \
            ((static_cast<size_t>(count) * 100U) <= (static_cast<size_t>(records) * (most))), \
        (std::to_string(count) + " for " + std::to_string(records) + " records").c_str()      \
    )

// clang-format off
TEST_GROUP(SolidSyslogNorFlashStoreCost)
{
    unsigned nextNumber = 0;

    void setup() override
    {
        stack = NorFlashStoreStack();
        stack.flashBlockCount = FLASH_BLOCKS;
        nextNumber = 0;
    }

    void teardown() override
    {
        stack.Stop();
    }

    void Store(unsigned records)
    {
        for (unsigned i = 0; i < records; i++)
        {
            CHECK_TRUE(stack.Write(RecordNumbered(nextNumber, RECORD_BYTES)));
            nextNumber++;
        }
    }

    static void Send(unsigned records)
    {
        for (unsigned i = 0; i < records; i++)
        {
            CHECK_FALSE(stack.ReadNext().empty());
            stack.MarkSent();
        }
    }

    /* Each record sent as soon as it is stored, as with the collector reachable. */
    void StoreAndSendAtOnce(unsigned records)
    {
        for (unsigned i = 0; i < records; i++)
        {
            Store(1);
            Send(1);
        }
    }
};

// clang-format on

TEST(SolidSyslogNorFlashStoreCost, AStoreOnABlankPartReadsEachBlockOnceAndErasesOnlyItsFirst)
{
    stack.StartOnABlankPart();

    /* One read of each block's state, then the check of the program that takes
     * the first. */
    LONGS_EQUAL(FLASH_BLOCKS + 1, NorFlashFake_ReadCount());
    LONGS_EQUAL(1, NorFlashFake_EraseCount());
}

TEST(SolidSyslogNorFlashStoreCost, AStoredRecordCostsOneProgramAndNoErase)
{
    stack.StartOnABlankPart();
    NorFlashFake_ResetCounts();

    Store(10);

    LONGS_EQUAL(10, NorFlashFake_ProgramCount());
    LONGS_EQUAL(0, NorFlashFake_EraseCount());
}

TEST(SolidSyslogNorFlashStoreCost, MarkingARecordSentCostsOneProgramAndNoErase)
{
    stack.StartOnABlankPart();
    Store(10);
    NorFlashFake_ResetCounts();

    Send(10);

    LONGS_EQUAL(10, NorFlashFake_ProgramCount());
    LONGS_EQUAL(0, NorFlashFake_EraseCount());
}

TEST(SolidSyslogNorFlashStoreCost, ABlockHoldsNineteenRecordsOfTwoHundredBytes)
{
    stack.StartOnABlankPart();
    Store(RECORDS_A_BLOCK);
    NorFlashFake_ResetCounts();

    Store(1); /* the twentieth takes the next block, which on a blank part is erased first */

    LONGS_EQUAL(1, NorFlashFake_EraseCount());
}

TEST(SolidSyslogNorFlashStoreCost, ARecordStoredAndSentAtOnceCostsAboutATwentiethOfAnErase)
{
    const unsigned records = RECORDS_A_BLOCK * FLASH_BLOCKS * 4U;
    stack.StartOnABlankPart();
    StoreAndSendAtOnce(RECORDS_A_BLOCK * FLASH_BLOCKS); /* the first lap, which also erases each blank block */
    NorFlashFake_ResetCounts();

    StoreAndSendAtOnce(records);

    CHECK_HUNDREDTHS_A_RECORD(5U, 6U, NorFlashFake_EraseCount(), records);
}

/* The record, its sent mark, and a nineteenth of the three that walk a block
 * through in use, disposed and erased. */
TEST(SolidSyslogNorFlashStoreCost, ARecordStoredAndSentAtOnceCostsALittleOverTwoPrograms)
{
    const unsigned records = RECORDS_A_BLOCK * FLASH_BLOCKS * 4U;
    stack.StartOnABlankPart();
    StoreAndSendAtOnce(RECORDS_A_BLOCK * FLASH_BLOCKS);
    NorFlashFake_ResetCounts();

    StoreAndSendAtOnce(records);

    CHECK_HUNDREDTHS_A_RECORD(210U, 220U, NorFlashFake_ProgramCount(), records);
}

TEST(SolidSyslogNorFlashStoreCost, AfterTheFirstLapEachLapOfTheRingErasesEveryBlockOnce)
{
    stack.StartOnABlankPart();
    StoreAndSendAtOnce(RECORDS_A_BLOCK * FLASH_BLOCKS * 2U);
    std::vector<size_t> before;
    before.reserve(FLASH_BLOCKS);
    for (size_t block = 0; block < FLASH_BLOCKS; block++)
    {
        before.push_back(NorFlashFake_EraseCountOf(block));
    }

    StoreAndSendAtOnce(RECORDS_A_BLOCK * FLASH_BLOCKS);

    for (size_t block = 0; block < FLASH_BLOCKS; block++)
    {
        LONGS_EQUAL(before[block] + 1U, NorFlashFake_EraseCountOf(block));
    }
}

/* The erase is spent on the block being taken, as late as it can be. */
TEST(SolidSyslogNorFlashStoreCost, TakingTheNextBlockCostsOneEraseAndDisposingOfOneCostsNone)
{
    stack.StartOnABlankPart();
    Store(RECORDS_A_BLOCK);
    Send(RECORDS_A_BLOCK); /* drained, and still the write block */
    NorFlashFake_ResetCounts();

    Store(1); /* takes the next block, then disposes of the drained one */

    LONGS_EQUAL(1, NorFlashFake_EraseCount());
    LONGS_EQUAL(3, NorFlashFake_ProgramCount()); /* the new block's state, the record, the old block's state */
}

/* A flash that will not erase the block the store needs fails the store's
 * device: the store freezes, and what is logged from then on is left to the
 * sender. */
TEST(SolidSyslogNorFlashStoreCost, AnEraseThatFailsFreezesTheStoreAndLeavesRecordsToTheSender)
{
    const std::string record = RecordNumbered(RECORDS_A_BLOCK, RECORD_BYTES);
    stack.StartOnABlankPart();
    Store(RECORDS_A_BLOCK); /* the next record needs the next block */
    ErrorHandlerFake_Install(nullptr);
    NorFlashFake_FailNextErase();

    LONGS_EQUAL(
        SOLIDSYSLOG_STORE_WRITE_NOT_RETAINED,
        SolidSyslogStore_Write(stack.store, record.data(), record.size())
    );

    CHECK_TRUE(SolidSyslogStore_IsTransient(stack.store));
    LONGS_EQUAL(
        SOLIDSYSLOG_STORE_WRITE_NOT_RETAINED,
        SolidSyslogStore_Write(stack.store, record.data(), record.size())
    );
    CALLED_FAKE(ErrorHandlerFake_Handle, ONCE); /* the freeze, and nothing for the write after it */
    LONGS_EQUAL(SOLIDSYSLOG_CAT_STORE_FROZEN, ErrorHandlerFake_LastCategory());
}

TEST(SolidSyslogNorFlashStoreCost, AFrozenStoreKeepsWhatItHeldForTheNextStart)
{
    stack.StartOnABlankPart();
    Store(RECORDS_A_BLOCK);
    ErrorHandlerFake_Install(nullptr);
    NorFlashFake_FailNextErase();
    CHECK_FALSE(stack.Write(RecordNumbered(RECORDS_A_BLOCK, RECORD_BYTES)));

    stack.Restart();

    LONGS_EQUAL(RECORDS_A_BLOCK, stack.Drain().size());
}

TEST(SolidSyslogNorFlashStoreCost, AFullStoreEvictingItsOldestCostsAboutATwentiethOfAnEraseARecord)
{
    const unsigned records = RECORDS_A_BLOCK * FLASH_BLOCKS * 4U;
    stack.discardPolicy = AlwaysOldest;
    stack.StartOnABlankPart();
    ErrorHandlerFake_Install(nullptr);
    Store(RECORDS_A_BLOCK * FLASH_BLOCKS * 2U);
    NorFlashFake_ResetCounts();

    Store(records);

    CHECK_HUNDREDTHS_A_RECORD(5U, 6U, NorFlashFake_EraseCount(), records);
}

/* The store grows a block size smaller than its largest record to fit one,
 * and the device takes what the flash block has room for. */
TEST(SolidSyslogNorFlashStoreCost, ABlockSizeSmallerThanTheLargestRecordStillStores)
{
    stack.deviceBlockSize = 300;
    stack.StartOnABlankPart();
    ErrorHandlerFake_Install(nullptr);

    Store(10);

    LONGS_EQUAL(10, stack.Drain().size());
    CHECK_TRUE(NorFlashFake_EraseCount() > 1U); /* it moved on from each block sooner than a whole one fills */
}

TEST(SolidSyslogNorFlashStoreCost, ARestartFindsWhatWasStoredAndNotSent)
{
    stack.StartOnABlankPart();
    Store(RECORDS_A_BLOCK + 5U);
    Send(3);

    stack.Restart();

    std::vector<unsigned> drained = stack.Drain();
    LONGS_EQUAL(RECORDS_A_BLOCK + 2U, drained.size());
    LONGS_EQUAL(3, drained.front());
    LONGS_EQUAL(RECORDS_A_BLOCK + 4U, drained.back());
}

/* What one run of the store had done when the power went. */
struct RunToTheCut
{
    std::vector<unsigned> kept; /* records Write answered as kept, in order */
    size_t markedSent = 0; /* how many of them a MarkSent finished marking */
    size_t markAttempts = 0; /* and how many a MarkSent was called for */
    bool writeInFlight = false; /* a Write was cut short, or answered that it had not kept */
    unsigned inFlight = 0;
};

enum
{
    /* A block size the store grows to one record of the largest message, so
     * that these records go four to a block whatever that largest is. */
    SMALLEST_BLOCK_SIZE = 1,
    SMALL_RECORD_BYTES = (SOLIDSYSLOG_MAX_MESSAGE_SIZE / 4) - 20
};

// clang-format off
TEST_GROUP(SolidSyslogNorFlashStorePowerCut)
{
    RunToTheCut run;
    unsigned nextNumber = 0;

    void setup() override
    {
        stack = NorFlashStoreStack();
        stack.deviceBlockSize = SMALLEST_BLOCK_SIZE;
    }

    void teardown() override
    {
        stack.Stop();
    }

    void StartARun()
    {
        stack.Stop();
        run = RunToTheCut();
        nextNumber = 0;
        stack.StartOnABlankPart();
        ErrorHandlerFake_Install(nullptr);
    }

    void Store(unsigned records)
    {
        for (unsigned i = 0; (i < records) && !NorFlashFake_PowerIsLost(); i++)
        {
            if (stack.Write(RecordNumbered(nextNumber, SMALL_RECORD_BYTES)))
            {
                run.kept.push_back(nextNumber);
            }
            else
            {
                run.writeInFlight = true;
                run.inFlight = nextNumber;
            }
            nextNumber++;
        }
    }

    void Send(unsigned records)
    {
        for (unsigned i = 0; (i < records) && !NorFlashFake_PowerIsLost(); i++)
        {
            std::string record = stack.ReadNext();
            if (!record.empty() && !NorFlashFake_PowerIsLost())
            {
                run.markAttempts++;
                stack.MarkSent();
                if (!NorFlashFake_PowerIsLost())
                {
                    run.markedSent++;
                }
            }
        }
    }

    /* Stores and sends by turns, through several rotations and several
     * drained blocks, never filling the store. */
    void StoreAndSendByTurns()
    {
        Store(6);
        Send(3);
        Store(6);
        Send(5);
        Store(4);
        Send(6);
        Store(5);
    }

    [[nodiscard]] bool WasKeptAndNotMarked(unsigned number) const
    {
        return std::find(run.kept.begin() + static_cast<std::ptrdiff_t>(run.markedSent), run.kept.end(), number) !=
               run.kept.end();
    }

    [[nodiscard]] bool MayBeOnThePart(unsigned number) const
    {
        return WasKeptAndNotMarked(number) || (run.writeInFlight && (number == run.inFlight));
    }

    [[nodiscard]] std::string Describe(size_t cutAt, enum NorFlashFakeCut cut, const std::vector<unsigned>& drained) const
    {
        return "cut " + std::to_string(cutAt) + ((cut == NORFLASHFAKE_CUT_TORN) ? " torn" : " clean") + ": kept " +
               std::to_string(run.kept.size()) + ", marked " + std::to_string(run.markedSent) + ", drained " +
               Listed(drained);
    }

    /* After the restart: nothing arrives that was not stored, nothing arrives
     * twice or out of order, and nothing kept and not yet sent is missing. */
    void CheckNothingKeptWasLost(const std::vector<unsigned>& drained, const std::string& where) const
    {
        CHECK_TEXT(std::is_sorted(drained.begin(), drained.end()), where.c_str());
        CHECK_TEXT(std::adjacent_find(drained.begin(), drained.end()) == drained.end(), where.c_str());
        for (unsigned number : drained)
        {
            CHECK_TEXT(MayBeOnThePart(number), where.c_str());
        }
        for (size_t i = run.markAttempts; i < run.kept.size(); i++)
        {
            CHECK_TEXT(std::find(drained.begin(), drained.end(), run.kept[i]) != drained.end(), where.c_str());
        }
    }

    void CheckTheStoreStillStores(const std::string& where) const
    {
        CHECK_TEXT(stack.Write(RecordNumbered(nextNumber, SMALL_RECORD_BYTES)), where.c_str());
        std::vector<unsigned> drained = stack.Drain();
        CHECK_TEXT((drained.size() == 1U) && (drained.front() == nextNumber), where.c_str());
    }

    /* How many programs and erases the run makes with the power left on. */
    template <typename Run>
    size_t WriteOperationsIn(Run runIt)
    {
        StartARun();
        size_t before = NorFlashFake_WriteOperationCount();
        runIt();
        return NorFlashFake_WriteOperationCount() - before;
    }
};

// clang-format on

TEST(SolidSyslogNorFlashStorePowerCut, TheRunStoresAndSendsThroughSeveralBlocks)
{
    StartARun();

    StoreAndSendByTurns();

    LONGS_EQUAL(21, run.kept.size());
    LONGS_EQUAL(14, run.markedSent);
    CHECK_TRUE(NorFlashFake_EraseCount() >= 4U);
}

TEST(SolidSyslogNorFlashStorePowerCut, ACutAtAnyProgramOrEraseLosesNothingTheStoreHadKept)
{
    const size_t operations = WriteOperationsIn([this] { StoreAndSendByTurns(); });

    for (size_t cutAt = 1; cutAt <= operations; cutAt++)
    {
        for (enum NorFlashFakeCut cut : {NORFLASHFAKE_CUT_CLEAN, NORFLASHFAKE_CUT_TORN})
        {
            StartARun();
            NorFlashFake_CutPowerAt(cutAt, cut);
            StoreAndSendByTurns();
            CHECK_TRUE(NorFlashFake_PowerIsLost());

            stack.Restart();

            std::vector<unsigned> drained = stack.Drain();
            std::string where = Describe(cutAt, cut, drained);
            CheckNothingKeptWasLost(drained, where);
            CheckTheStoreStillStores(where);
        }
    }
}

/* A store of three blocks under the Oldest policy, written to without a
 * send: every fourth record takes a new block, and from the third block on
 * each of those evicts the oldest. */
TEST(SolidSyslogNorFlashStorePowerCut, ACutWhileAFullStoreEvictsNeverCostsTheNewestRecord)
{
    stack.storeMaxBlocks = 3;
    stack.discardPolicy = AlwaysOldest;
    const size_t operations = WriteOperationsIn([this] { Store(30); });

    for (size_t cutAt = 1; cutAt <= operations; cutAt++)
    {
        for (enum NorFlashFakeCut cut : {NORFLASHFAKE_CUT_CLEAN, NORFLASHFAKE_CUT_TORN})
        {
            StartARun();
            NorFlashFake_CutPowerAt(cutAt, cut);
            Store(30);
            CHECK_TRUE(NorFlashFake_PowerIsLost());

            stack.Restart();

            std::vector<unsigned> drained = stack.Drain();
            std::string where = Describe(cutAt, cut, drained);
            for (size_t i = 1; i < drained.size(); i++)
            {
                CHECK_TEXT(drained[i] == (drained[i - 1U] + 1U), where.c_str());
            }
            if (!run.kept.empty())
            {
                CHECK_TEXT(std::find(drained.begin(), drained.end(), run.kept.back()) != drained.end(), where.c_str());
            }
            CheckTheStoreStillStores(where);
        }
    }
}

/* The record that needs the oldest block's place is appended before that block
 * is touched: where a cut has cost the oldest block, the record that evicted
 * it is on the part. */
TEST(SolidSyslogNorFlashStorePowerCut, WhereACutCostTheOldestBlockTheRecordThatEvictedItWasStored)
{
    enum
    {
        RECORDS = 30
    };

    stack.storeMaxBlocks = 3;
    stack.discardPolicy = AlwaysOldest;
    /* The oldest record on the part once `n` records have been stored. */
    std::vector<unsigned> oldestAfter(RECORDS + 1U, 0U);
    for (unsigned stored = 1; stored <= RECORDS; stored++)
    {
        StartARun();
        Store(stored);
        stack.Restart();
        oldestAfter[stored] = stack.Drain().front();
    }
    const size_t operations = WriteOperationsIn([this] { Store(RECORDS); });

    for (size_t cutAt = 1; cutAt <= operations; cutAt++)
    {
        for (enum NorFlashFakeCut cut : {NORFLASHFAKE_CUT_CLEAN, NORFLASHFAKE_CUT_TORN})
        {
            StartARun();
            NorFlashFake_CutPowerAt(cutAt, cut);
            Store(RECORDS);

            stack.Restart();

            std::vector<unsigned> drained = stack.Drain();
            std::string where = Describe(cutAt, cut, drained);
            bool oldestBlockWent =
                !run.kept.empty() && !drained.empty() && (drained.front() > oldestAfter[run.kept.size()]);
            if (run.writeInFlight && oldestBlockWent)
            {
                CHECK_TEXT(std::find(drained.begin(), drained.end(), run.inFlight) != drained.end(), where.c_str());
            }
        }
    }
}

/* A store at the clamp of its ceiling, two blocks short of the part, written
 * to under the Oldest policy without a send. A cut in a rotation can leave the
 * run one over its ceiling, and a second cut in a later run then finds it so.
 * Delivery stays in order, and what is lost is only ever the oldest records,
 * which is what an eviction loses. */
TEST(SolidSyslogNorFlashStorePowerCut, TwoCutsWhileAFullStoreAtItsClampEvictsDeliverInOrder)
{
    enum
    {
        FIRST_RUN = 16,
        SECOND_RUN = 12
    };

    stack.flashBlockCount = 5;
    stack.storeMaxBlocks = 3;
    stack.discardPolicy = AlwaysOldest;
    const size_t firstOperations = WriteOperationsIn([this] { Store(FIRST_RUN); });
    stack.Restart();
    Store(SECOND_RUN);
    const size_t secondOperations = NorFlashFake_WriteOperationCount();

    for (size_t firstCut = 1; firstCut <= firstOperations; firstCut++)
    {
        for (size_t secondCut = 1; secondCut <= secondOperations; secondCut++)
        {
            StartARun();
            NorFlashFake_CutPowerAt(firstCut, NORFLASHFAKE_CUT_CLEAN);
            Store(FIRST_RUN);
            stack.Restart();
            NorFlashFake_CutPowerAt(secondCut, NORFLASHFAKE_CUT_CLEAN);
            Store(SECOND_RUN);

            stack.Restart();

            std::vector<unsigned> drained = stack.Drain();
            std::string where = "cuts " + std::to_string(firstCut) + " and " + std::to_string(secondCut) +
                                ": drained " + Listed(drained);
            CHECK_TEXT(std::is_sorted(drained.begin(), drained.end()), where.c_str());
            CHECK_TEXT(std::adjacent_find(drained.begin(), drained.end()) == drained.end(), where.c_str());
            CHECK_TEXT(run.kept.empty() || !drained.empty(), where.c_str());
            for (unsigned number : run.kept)
            {
                bool evicted = drained.empty() || (number < drained.front());
                CHECK_TEXT(
                    evicted || (std::find(drained.begin(), drained.end(), number) != drained.end()),
                    where.c_str()
                );
            }
            CheckTheStoreStillStores(where);
        }
    }
}
