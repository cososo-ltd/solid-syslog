#include "CppUTest/TestHarness.h"
#include "SolidSyslogBlockDevice.h"
#include "SolidSyslogNullBlockDevice.h"

// clang-format off
TEST_GROUP(SolidSyslogNullBlockDevice)
{
    struct SolidSyslogBlockDevice* device = nullptr;

    void setup() override
    {
        device = SolidSyslogNullBlockDevice_Get();
    }
};

// clang-format on

TEST(SolidSyslogNullBlockDevice, GetReturnsNonNull)
{
    CHECK_TRUE(device != nullptr);
}

TEST(SolidSyslogNullBlockDevice, AcquireReturnsFalse)
{
    CHECK_FALSE(SolidSyslogBlockDevice_Acquire(device, 0));
}

TEST(SolidSyslogNullBlockDevice, DisposeReturnsFalse)
{
    CHECK_FALSE(SolidSyslogBlockDevice_Dispose(device, 0));
}

TEST(SolidSyslogNullBlockDevice, ExistsReturnsFalse)
{
    CHECK_FALSE(SolidSyslogBlockDevice_Exists(device, 0));
}

TEST(SolidSyslogNullBlockDevice, ReadReturnsFalse)
{
    char buf[4] = {};
    CHECK_FALSE(SolidSyslogBlockDevice_Read(device, 0, 0, buf, sizeof(buf)));
}

TEST(SolidSyslogNullBlockDevice, AppendReturnsFalse)
{
    CHECK_FALSE(SolidSyslogBlockDevice_Append(device, 0, "x", 1));
}

TEST(SolidSyslogNullBlockDevice, WriteAtReturnsFalse)
{
    CHECK_FALSE(SolidSyslogBlockDevice_WriteAt(device, 0, 0, "x", 1));
}

TEST(SolidSyslogNullBlockDevice, SizeReturnsZero)
{
    LONGS_EQUAL(0, SolidSyslogBlockDevice_Size(device, 0));
}

TEST(SolidSyslogNullBlockDevice, GetBlockSizeReturnsZero)
{
    LONGS_EQUAL(0, SolidSyslogBlockDevice_GetBlockSize(device));
}

TEST(SolidSyslogNullBlockDevice, GetIsIdempotentAndReturnsSameInstance)
{
    POINTERS_EQUAL(device, SolidSyslogNullBlockDevice_Get());
}
