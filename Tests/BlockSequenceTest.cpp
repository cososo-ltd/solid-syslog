#include <map>
#include <set>
#include <vector>
#include <cstddef>
#include <utility>

#include "CppUTest/TestHarness.h"
#include "SolidSyslogBlockStore.h"

extern "C"
{
#include "BlockSequencePrivate.h"
}
#include "SolidSyslogBlockDeviceDefinition.h"

namespace
{
enum class CallType
{
    Acquire,
    Dispose,
    Exists
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
    std::vector<DeviceCall>* calls; /* optional — tests that don't care leave nullptr */
    std::map<size_t, size_t>* sizes; /* optional — tests that need realistic Size readings populate */
    bool failNextDispose;
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

size_t FakeSize(struct SolidSyslogBlockDevice* self, size_t blockIndex)
{
    const ScanFake& fake = ToFake(self);
    size_t size = 0;

    if (fake.sizes != nullptr)
    {
        auto it = fake.sizes->find(blockIndex);
        if (it != fake.sizes->end())
        {
            size = it->second;
        }
    }

    return size;
}
} // namespace

// clang-format off
TEST_GROUP(BlockSequenceScan)
{
    ScanFake fakeDevice = {};
    std::set<size_t> existing;
    struct BlockSequence* sequence = nullptr;

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

        struct BlockSequenceConfig config = {};
        config.BlockDevice                = &fakeDevice.Base;
        config.MaxBlockSize                = 1000;
        config.MaxBlocks                   = 99;
        config.DiscardPolicy              = SOLIDSYSLOG_DISCARD_POLICY_OLDEST;
        sequence = BlockSequence_Create(&config);
    }

    void teardown() override
    {
        BlockSequence_Destroy(sequence);
    }
};

// clang-format on

TEST(BlockSequenceScan, ColdStartAcquiresBlockZero)
{
    CHECK_TRUE(BlockSequence_Open(sequence));
    LONGS_EQUAL(0, BlockSequence_ReadSequence(sequence));
    LONGS_EQUAL(0, BlockSequence_WriteSequence(sequence));
}

TEST(BlockSequenceScan, ResumesContiguousLinearRange)
{
    existing = {2, 3, 4};
    CHECK_TRUE(BlockSequence_Open(sequence));
    LONGS_EQUAL(2, BlockSequence_ReadSequence(sequence));
    LONGS_EQUAL(4, BlockSequence_WriteSequence(sequence));
}

TEST(BlockSequenceScan, ResumesAtZeroWhenOnlyBlockZeroExists)
{
    existing = {0};
    CHECK_TRUE(BlockSequence_Open(sequence));
    LONGS_EQUAL(0, BlockSequence_ReadSequence(sequence));
    LONGS_EQUAL(0, BlockSequence_WriteSequence(sequence));
}

/* After enough rotations, the on-disk block range straddles the 99 -> 00
 * sequence boundary. The naive "lowest = oldest, highest = write" heuristic
 * would mis-identify {98, 99, 0, 1} as oldest=0/write=99, breaking read order
 * and the discard target on the next rotation. Run must be detected as a
 * single circular range. */
TEST(BlockSequenceScan, ResumesWrappedSequenceRangeCorrectly)
{
    existing = {98, 99, 0, 1};
    CHECK_TRUE(BlockSequence_Open(sequence));
    LONGS_EQUAL(98, BlockSequence_ReadSequence(sequence));
    LONGS_EQUAL(1, BlockSequence_WriteSequence(sequence));
}

TEST(BlockSequenceScan, ResumesWrappedSingleBlockAtBoundary)
{
    existing = {99, 0};
    CHECK_TRUE(BlockSequence_Open(sequence));
    LONGS_EQUAL(99, BlockSequence_ReadSequence(sequence));
    LONGS_EQUAL(0, BlockSequence_WriteSequence(sequence));
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
    struct BlockSequence* sequence = nullptr;

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

        struct BlockSequenceConfig config = {};
        config.BlockDevice                = &fakeDevice.Base;
        config.MaxBlockSize               = ROTATION_BLOCK_SIZE;
        config.MaxBlocks                  = 99;
        config.DiscardPolicy              = SOLIDSYSLOG_DISCARD_POLICY_OLDEST;
        sequence = BlockSequence_Create(&config);

        BlockSequence_Open(sequence); /* cold start: Acquire(0) */
        /* Simulate one record's worth of data in block 0 — production rotation
         * never seals an empty block, and the dispose-on-empty trigger uses
         * device.Size to decide drained-ness. */
        BlockSequence_NoteRecordWritten(sequence, SIMULATED_RECORD_SIZE);
        sizes[BlockSequence_WriteSequence(sequence)] = SIMULATED_RECORD_SIZE;
        calls.clear();
    }

    void teardown() override
    {
        BlockSequence_Destroy(sequence);
    }

    [[nodiscard]] bool DisposePrecedesAcquire(size_t blockIndex) const
    {
        std::ptrdiff_t disposeAt = -1;
        std::ptrdiff_t acquireAt = -1;
        for (size_t i = 0; i < calls.size(); i++)
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
        BlockSequence_PrepareForWrite(sequence, ROTATION_BLOCK_SIZE + 1, &readBlockChanged);
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
    /* If the stale block can't be Disposed, we must NOT proceed to Acquire —
     * a flash "verify-and-use" driver would reject the stale block anyway,
     * and surfacing the failure here matches the slice-3 retry contract. */
    existing.insert(1);
    fakeDevice.failNextDispose = true;

    bool readBlockChanged = false;
    bool acquired = BlockSequence_PrepareForWrite(sequence, ROTATION_BLOCK_SIZE + 1, &readBlockChanged);

    CHECK_FALSE(acquired);
    for (const auto& call : calls)
    {
        if ((call.type == CallType::Acquire) && (call.BlockIndex == 1))
        {
            FAIL("Acquire(1) was called even though Dispose(1) failed");
        }
    }
}
