#include <map>
#include <set>
#include <vector>
#include <cstddef>
#include <utility>

#include "CppUTest/TestHarness.h"
#include "SolidSyslogBlockStore.h"
#include "SolidSyslogBlockStoreErrors.h"

extern "C"
{
#include "SolidSyslogBlockSequencePrivate.h"
}
#include "SolidSyslogBlockDeviceDefinition.h"

namespace
{
enum class CallType
{
    Acquire,
    Dispose,
    Exists,
    Size
};

struct DeviceCall
{
    CallType type;
    size_t BlockIndex;
};

struct ScanFake
{
    struct SolidSyslogBlockDevice Base;
    std::set<size_t>* existing;
    std::vector<DeviceCall>* calls; /* optional - tests that don't care leave nullptr */
    std::map<size_t, size_t>* sizes; /* optional - tests that need realistic Size readings populate */
    bool failNextDispose;
    bool failNextSize;
};

inline ScanFake& ToFake(struct SolidSyslogBlockDevice* self)
{
    return *reinterpret_cast<ScanFake*>(self);
}

inline void RecordCall(ScanFake& fake, CallType type, size_t blockIndex)
{
    if (fake.calls != nullptr)
    {
        fake.calls->push_back({type, blockIndex});
    }
}

bool FakeExists(struct SolidSyslogBlockDevice* self, size_t blockIndex)
{
    ScanFake& fake = ToFake(self);
    RecordCall(fake, CallType::Exists, blockIndex);
    return fake.existing->count(blockIndex) > 0;
}

bool FakeAcquire(struct SolidSyslogBlockDevice* self, size_t blockIndex)
{
    ScanFake& fake = ToFake(self);
    RecordCall(fake, CallType::Acquire, blockIndex);
    fake.existing->insert(blockIndex);
    if (fake.sizes != nullptr)
    {
        (*fake.sizes)[blockIndex] = 0;
    }
    return true;
}

bool FakeDispose(struct SolidSyslogBlockDevice* self, size_t blockIndex)
{
    ScanFake& fake = ToFake(self);
    RecordCall(fake, CallType::Dispose, blockIndex);

    if (fake.failNextDispose)
    {
        fake.failNextDispose = false;
        return false;
    }

    fake.existing->erase(blockIndex);
    if (fake.sizes != nullptr)
    {
        fake.sizes->erase(blockIndex);
    }
    return true;
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) -- vtable signature: blockIndex / offset are positional, distinct semantics
bool FakeRead(struct SolidSyslogBlockDevice* self, size_t blockIndex, size_t offset, void* buf, size_t count)
{
    (void) self;
    (void) blockIndex;
    (void) offset;
    (void) buf;
    (void) count;
    return false;
}

bool FakeAppend(struct SolidSyslogBlockDevice* self, size_t blockIndex, const void* buf, size_t count)
{
    (void) self;
    (void) blockIndex;
    (void) buf;
    (void) count;
    return true;
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) -- vtable signature: blockIndex / offset are positional, distinct semantics
bool FakeWriteAt(struct SolidSyslogBlockDevice* self, size_t blockIndex, size_t offset, const void* buf, size_t count)
{
    (void) self;
    (void) blockIndex;
    (void) offset;
    (void) buf;
    (void) count;
    return true;
}

bool FakeSize(struct SolidSyslogBlockDevice* self, size_t blockIndex, size_t* reported)
{
    ScanFake& fake = ToFake(self);
    RecordCall(fake, CallType::Size, blockIndex);
    size_t size = 0U;

    if (fake.sizes != nullptr)
    {
        auto it = fake.sizes->find(blockIndex);
        if (it != fake.sizes->end())
        {
            size = it->second;
        }
    }

    *reported = size;
    return !fake.failNextSize;
}
} // namespace

// clang-format off
TEST_GROUP(BlockSequenceScan)
{
    ScanFake fakeDevice = {};
    std::set<size_t> existing;
    struct SolidSyslogBlockSequence* sequence = nullptr;

    void setup() override
    {
        fakeDevice.Base.Acquire = FakeAcquire;
        fakeDevice.Base.Dispose = FakeDispose;
        fakeDevice.Base.Exists  = FakeExists;
        fakeDevice.Base.Read    = FakeRead;
        fakeDevice.Base.Append  = FakeAppend;
        fakeDevice.Base.WriteAt = FakeWriteAt;
        fakeDevice.Base.Size    = FakeSize;
        fakeDevice.existing     = &existing;

        struct SolidSyslogBlockSequenceConfig config = {};
        config.BlockDevice                = &fakeDevice.Base;
        config.MaxBlockSize                = 1000;
        config.MaxBlocks                   = 99;
        config.DiscardPolicy              = SOLIDSYSLOG_DISCARD_POLICY_OLDEST;
        sequence = SolidSyslogBlockSequence_Create(&config);
    }

    void teardown() override
    {
        SolidSyslogBlockSequence_Destroy(sequence);
    }
};

// clang-format on

TEST(BlockSequenceScan, ColdStartAcquiresBlockZero)
{
    CHECK_TRUE(SolidSyslogBlockSequence_Open(sequence));
    LONGS_EQUAL(0, SolidSyslogBlockSequence_ReadSequence(sequence));
    LONGS_EQUAL(0, SolidSyslogBlockSequence_WriteSequence(sequence));
}

TEST(BlockSequenceScan, ResumesContiguousLinearRange)
{
    existing = {2, 3, 4};
    CHECK_TRUE(SolidSyslogBlockSequence_Open(sequence));
    LONGS_EQUAL(2, SolidSyslogBlockSequence_ReadSequence(sequence));
    LONGS_EQUAL(4, SolidSyslogBlockSequence_WriteSequence(sequence));
}

TEST(BlockSequenceScan, ResumesAtZeroWhenOnlyBlockZeroExists)
{
    existing = {0};
    CHECK_TRUE(SolidSyslogBlockSequence_Open(sequence));
    LONGS_EQUAL(0, SolidSyslogBlockSequence_ReadSequence(sequence));
    LONGS_EQUAL(0, SolidSyslogBlockSequence_WriteSequence(sequence));
}

/* After enough rotations, the on-disk block range straddles the 99 -> 00
 * sequence boundary. The naive "lowest = oldest, highest = write" heuristic
 * would mis-identify {98, 99, 0, 1} as oldest=0/write=99, breaking read order
 * and the discard target on the next rotation. Run must be detected as a
 * single circular range. */
TEST(BlockSequenceScan, ResumesWrappedSequenceRangeCorrectly)
{
    existing = {98, 99, 0, 1};
    CHECK_TRUE(SolidSyslogBlockSequence_Open(sequence));
    LONGS_EQUAL(98, SolidSyslogBlockSequence_ReadSequence(sequence));
    LONGS_EQUAL(1, SolidSyslogBlockSequence_WriteSequence(sequence));
}

TEST(BlockSequenceScan, ResumesWrappedSingleBlockAtBoundary)
{
    existing = {99, 0};
    CHECK_TRUE(SolidSyslogBlockSequence_Open(sequence));
    LONGS_EQUAL(99, SolidSyslogBlockSequence_ReadSequence(sequence));
    LONGS_EQUAL(0, SolidSyslogBlockSequence_WriteSequence(sequence));
}

namespace
{
enum
{
    ROTATION_BLOCK_SIZE = 100
};
} // namespace

// clang-format off
TEST_GROUP(BlockSequenceRotation)
{
    static const size_t SIMULATED_RECORD_SIZE = 50;

    ScanFake fakeDevice = {};
    std::set<size_t> existing;
    std::vector<DeviceCall> calls;
    std::map<size_t, size_t> sizes;
    struct SolidSyslogBlockSequence* sequence = nullptr;

    void setup() override
    {
        fakeDevice.Base.Acquire = FakeAcquire;
        fakeDevice.Base.Dispose = FakeDispose;
        fakeDevice.Base.Exists  = FakeExists;
        fakeDevice.Base.Read    = FakeRead;
        fakeDevice.Base.Append  = FakeAppend;
        fakeDevice.Base.WriteAt = FakeWriteAt;
        fakeDevice.Base.Size    = FakeSize;
        fakeDevice.existing     = &existing;
        fakeDevice.calls        = &calls;
        fakeDevice.sizes        = &sizes;

        CreateSequence(99);
    }

    void teardown() override
    {
        SolidSyslogBlockSequence_Destroy(sequence);
    }

    void CreateSequence(size_t maxBlocks)
    {
        struct SolidSyslogBlockSequenceConfig config = {};
        config.BlockDevice                = &fakeDevice.Base;
        config.MaxBlockSize               = ROTATION_BLOCK_SIZE;
        config.MaxBlocks                  = maxBlocks;
        config.DiscardPolicy              = SOLIDSYSLOG_DISCARD_POLICY_OLDEST;
        sequence = SolidSyslogBlockSequence_Create(&config);

        SolidSyslogBlockSequence_Open(sequence); /* cold start: Acquire(0) */
        /* Simulate one record's worth of data in block 0 - production rotation
         * never seals an empty block, and the dispose-on-empty trigger uses
         * device.Size to decide drained-ness. */
        NoteRecordInWriteBlock();
        calls.clear();
    }

    void RecreateWithMaxBlocks(size_t maxBlocks)
    {
        SolidSyslogBlockSequence_Destroy(sequence);
        existing.clear();
        sizes.clear();
        CreateSequence(maxBlocks);
    }

    void NoteRecordInWriteBlock()
    {
        SolidSyslogBlockSequence_NoteRecordWritten(sequence, SIMULATED_RECORD_SIZE);
        sizes[SolidSyslogBlockSequence_WriteSequence(sequence)] = SIMULATED_RECORD_SIZE;
    }

    [[nodiscard]] bool DisposePrecedesAcquire(size_t blockIndex) const
    {
        std::ptrdiff_t disposeAt = -1;
        std::ptrdiff_t acquireAt = -1;
        for (size_t i = 0U; i < calls.size(); i++)
        {
            if ((calls[i].BlockIndex == blockIndex) && (calls[i].type == CallType::Dispose))
            {
                disposeAt = static_cast<std::ptrdiff_t>(i);
            }
            if ((calls[i].BlockIndex == blockIndex) && (calls[i].type == CallType::Acquire))
            {
                acquireAt = static_cast<std::ptrdiff_t>(i);
            }
        }
        return (disposeAt >= 0) && (acquireAt >= 0) && (disposeAt < acquireAt);
    }

    void ForceRotation() const
    {
        bool readBlockChanged = false;
        SolidSyslogBlockSequence_PrepareForWrite(sequence, ROTATION_BLOCK_SIZE + 1, &readBlockChanged);
    }
};

// clang-format on

TEST(BlockSequenceRotation, RotationDisposesStaleBlockBeforeAcquiring)
{
    existing.insert(1); /* simulate stale content left from a previous run */

    ForceRotation();

    CHECK_TRUE(DisposePrecedesAcquire(1));
}

TEST(BlockSequenceRotation, RotationSkipsDisposeWhenTargetBlockEmpty)
{
    /* Baseline: target block isn't on disk; no Dispose call should be emitted.
     * Pins the cost optimisation that distinguishes flash drivers' "verify-and-use"
     * fast path from "erase-and-use". */
    ForceRotation();

    for (const auto& call : calls)
    {
        CHECK_FALSE(call.type == CallType::Dispose);
    }
}

TEST(BlockSequenceRotation, RotationFailsWhenStaleBlockDisposeFails)
{
    /* If the stale block can't be Disposed, we must NOT proceed to Acquire -
     * a flash "verify-and-use" driver would reject the stale block anyway,
     * and surfacing the failure here matches the slice-3 retry contract. */
    existing.insert(1);
    fakeDevice.failNextDispose = true;

    bool readBlockChanged = false;
    bool acquired = SolidSyslogBlockSequence_PrepareForWrite(sequence, ROTATION_BLOCK_SIZE + 1, &readBlockChanged);

    CHECK_FALSE(acquired);
    for (const auto& call : calls)
    {
        if ((call.type == CallType::Acquire) && (call.BlockIndex == 1))
        {
            FAIL("Acquire(1) was called even though Dispose(1) failed");
        }
    }
}

TEST(BlockSequenceRotation, AFailedEvictionIsTheLastDeviceCall)
{
    RecreateWithMaxBlocks(2);
    ForceRotation(); /* block 1 */
    NoteRecordInWriteBlock();
    calls.clear();
    fakeDevice.failNextDispose = true;

    ForceRotation(); /* block 2 - evicting block 0 fails */

    CHECK_TRUE(calls.back().type == CallType::Dispose);
}

TEST(BlockSequenceRotation, AStaleBlockThatWillNotDisposeIsADisposeFailure)
{
    existing.insert(1);
    fakeDevice.failNextDispose = true;

    ForceRotation();

    LONGS_EQUAL(SOLIDSYSLOG_BLOCK_STORE_ERROR_DISPOSE_FAILED, SolidSyslogBlockSequence_FailedOperation(sequence));
}
