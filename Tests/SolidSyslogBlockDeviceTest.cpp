#include "CppUTest/TestHarness.h"
#include "SolidSyslogBlockDevice.h"
#include "SolidSyslogBlockDeviceDefinition.h"

namespace
{
struct FakeBlockDevice
{
    struct SolidSyslogBlockDevice base;
    size_t                        lastBlockIndex      = 0;
    size_t                        lastOffset          = 0;
    size_t                        lastCount           = 0;
    const void*                   lastReadDestination = nullptr;
    const void*                   lastWriteSource     = nullptr;
    bool                          acquireReturn       = true;
    bool                          disposeReturn       = true;
    bool                          existsReturn        = true;
    bool                          readReturn          = true;
    bool                          appendReturn        = true;
    bool                          writeAtReturn       = true;
    size_t                        sizeReturn          = 0;
};

inline FakeBlockDevice& ToFake(struct SolidSyslogBlockDevice* self)
{
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast) -- vtable downcast: base is the first member of FakeBlockDevice
    return *reinterpret_cast<FakeBlockDevice*>(self);
}

bool FakeAcquire(struct SolidSyslogBlockDevice* self, size_t blockIndex)
{
    FakeBlockDevice& fake = ToFake(self);
    fake.lastBlockIndex   = blockIndex;
    return fake.acquireReturn;
}

bool FakeDispose(struct SolidSyslogBlockDevice* self, size_t blockIndex)
{
    FakeBlockDevice& fake = ToFake(self);
    fake.lastBlockIndex   = blockIndex;
    return fake.disposeReturn;
}

bool FakeExists(struct SolidSyslogBlockDevice* self, size_t blockIndex)
{
    FakeBlockDevice& fake = ToFake(self);
    fake.lastBlockIndex   = blockIndex;
    return fake.existsReturn;
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) -- vtable signature: blockIndex / offset are positional, distinct semantics
bool FakeRead(struct SolidSyslogBlockDevice* self, size_t blockIndex, size_t offset, void* buf, size_t count)
{
    FakeBlockDevice& fake    = ToFake(self);
    fake.lastBlockIndex      = blockIndex;
    fake.lastOffset          = offset;
    fake.lastReadDestination = buf;
    fake.lastCount           = count;
    return fake.readReturn;
}

bool FakeAppend(struct SolidSyslogBlockDevice* self, size_t blockIndex, const void* buf, size_t count)
{
    FakeBlockDevice& fake = ToFake(self);
    fake.lastBlockIndex   = blockIndex;
    fake.lastWriteSource  = buf;
    fake.lastCount        = count;
    return fake.appendReturn;
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) -- vtable signature: blockIndex / offset are positional, distinct semantics
bool FakeWriteAt(struct SolidSyslogBlockDevice* self, size_t blockIndex, size_t offset, const void* buf, size_t count)
{
    FakeBlockDevice& fake = ToFake(self);
    fake.lastBlockIndex   = blockIndex;
    fake.lastOffset       = offset;
    fake.lastWriteSource  = buf;
    fake.lastCount        = count;
    return fake.writeAtReturn;
}

size_t FakeSize(struct SolidSyslogBlockDevice* self, size_t blockIndex)
{
    FakeBlockDevice& fake = ToFake(self);
    fake.lastBlockIndex   = blockIndex;
    return fake.sizeReturn;
}
} // namespace

// clang-format off
TEST_GROUP(SolidSyslogBlockDevice)
{
    FakeBlockDevice fake = {};
    struct SolidSyslogBlockDevice* device = nullptr;

    void setup() override
    {
        fake.base.Acquire = FakeAcquire;
        fake.base.Dispose = FakeDispose;
        fake.base.Exists  = FakeExists;
        fake.base.Read    = FakeRead;
        fake.base.Append  = FakeAppend;
        fake.base.WriteAt = FakeWriteAt;
        fake.base.Size    = FakeSize;
        // cppcheck-suppress unreadVariable -- used across TEST_GROUP methods; cppcheck does not model CppUTest macros
        device = &fake.base;
    }
};

// clang-format on

TEST(SolidSyslogBlockDevice, AcquireForwardsBlockIndexToVtable)
{
    SolidSyslogBlockDevice_Acquire(device, 7);
    LONGS_EQUAL(7, fake.lastBlockIndex);
}

TEST(SolidSyslogBlockDevice, AcquireReturnsVtableResult)
{
    fake.acquireReturn = false;
    CHECK_FALSE(SolidSyslogBlockDevice_Acquire(device, 0));
}

TEST(SolidSyslogBlockDevice, DisposeForwardsBlockIndexToVtable)
{
    SolidSyslogBlockDevice_Dispose(device, 9);
    LONGS_EQUAL(9, fake.lastBlockIndex);
}

TEST(SolidSyslogBlockDevice, DisposeReturnsVtableResult)
{
    fake.disposeReturn = false;
    CHECK_FALSE(SolidSyslogBlockDevice_Dispose(device, 0));
}

TEST(SolidSyslogBlockDevice, ExistsForwardsBlockIndexToVtable)
{
    SolidSyslogBlockDevice_Exists(device, 4);
    LONGS_EQUAL(4, fake.lastBlockIndex);
}

TEST(SolidSyslogBlockDevice, ExistsReturnsVtableResult)
{
    fake.existsReturn = false;
    CHECK_FALSE(SolidSyslogBlockDevice_Exists(device, 0));
}

TEST(SolidSyslogBlockDevice, ReadForwardsAllArgumentsToVtable)
{
    char buf[8] = {};
    SolidSyslogBlockDevice_Read(device, 3, 12, buf, sizeof(buf));
    LONGS_EQUAL(3, fake.lastBlockIndex);
    LONGS_EQUAL(12, fake.lastOffset);
    LONGS_EQUAL(sizeof(buf), fake.lastCount);
    POINTERS_EQUAL(buf, fake.lastReadDestination);
}

TEST(SolidSyslogBlockDevice, ReadReturnsVtableResult)
{
    fake.readReturn = false;
    char buf[1]     = {};
    CHECK_FALSE(SolidSyslogBlockDevice_Read(device, 0, 0, buf, 1));
}

TEST(SolidSyslogBlockDevice, AppendForwardsAllArgumentsToVtable)
{
    const char data[] = "abc";
    SolidSyslogBlockDevice_Append(device, 5, data, sizeof(data));
    LONGS_EQUAL(5, fake.lastBlockIndex);
    LONGS_EQUAL(sizeof(data), fake.lastCount);
    POINTERS_EQUAL(data, fake.lastWriteSource);
}

TEST(SolidSyslogBlockDevice, AppendReturnsVtableResult)
{
    fake.appendReturn = false;
    CHECK_FALSE(SolidSyslogBlockDevice_Append(device, 0, "x", 1));
}

TEST(SolidSyslogBlockDevice, WriteAtForwardsAllArgumentsToVtable)
{
    const char data[] = "yz";
    SolidSyslogBlockDevice_WriteAt(device, 2, 17, data, sizeof(data));
    LONGS_EQUAL(2, fake.lastBlockIndex);
    LONGS_EQUAL(17, fake.lastOffset);
    LONGS_EQUAL(sizeof(data), fake.lastCount);
    POINTERS_EQUAL(data, fake.lastWriteSource);
}

TEST(SolidSyslogBlockDevice, WriteAtReturnsVtableResult)
{
    fake.writeAtReturn = false;
    CHECK_FALSE(SolidSyslogBlockDevice_WriteAt(device, 0, 0, "x", 1));
}

TEST(SolidSyslogBlockDevice, SizeForwardsBlockIndexToVtable)
{
    SolidSyslogBlockDevice_Size(device, 6);
    LONGS_EQUAL(6, fake.lastBlockIndex);
}

TEST(SolidSyslogBlockDevice, SizeReturnsVtableResult)
{
    fake.sizeReturn = 42;
    LONGS_EQUAL(42, SolidSyslogBlockDevice_Size(device, 0));
}
