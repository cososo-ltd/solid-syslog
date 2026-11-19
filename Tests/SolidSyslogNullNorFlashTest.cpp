#include "CppUTest/TestHarness.h"
#include "SolidSyslogNorFlash.h"
#include "SolidSyslogNullNorFlash.h"

// clang-format off
TEST_GROUP(SolidSyslogNullNorFlash)
{
    struct SolidSyslogNorFlash* flash = nullptr;

    void setup() override
    {
        flash = SolidSyslogNullNorFlash_Get();
    }
};

// clang-format on

TEST(SolidSyslogNullNorFlash, GetReturnsNonNull)
{
    CHECK_TRUE(flash != nullptr);
}

TEST(SolidSyslogNullNorFlash, GetReturnsTheSameInstance)
{
    POINTERS_EQUAL(flash, SolidSyslogNullNorFlash_Get());
}

TEST(SolidSyslogNullNorFlash, ReadReturnsFalse)
{
    char buffer[4] = {};
    CHECK_FALSE(SolidSyslogNorFlash_Read(flash, 0, 0, buffer, sizeof(buffer)));
}

TEST(SolidSyslogNullNorFlash, ProgramReturnsFalse)
{
    CHECK_FALSE(SolidSyslogNorFlash_Program(flash, 0, 0, "x", 1));
}

TEST(SolidSyslogNullNorFlash, EraseReturnsFalse)
{
    CHECK_FALSE(SolidSyslogNorFlash_Erase(flash, 0));
}

TEST(SolidSyslogNullNorFlash, GetBlockSizeReturnsZero)
{
    LONGS_EQUAL(0, SolidSyslogNorFlash_GetBlockSize(flash));
}

TEST(SolidSyslogNullNorFlash, GetBlockCountReturnsZero)
{
    LONGS_EQUAL(0, SolidSyslogNorFlash_GetBlockCount(flash));
}
