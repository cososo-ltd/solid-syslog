#include "CppUTest/TestHarness.h"
#include "SolidSyslogFile.h"
#include "SolidSyslogNullFile.h"

// clang-format off
TEST_GROUP(SolidSyslogNullFile)
{
    struct SolidSyslogFile* file = nullptr;

    void setup() override
    {
        file = SolidSyslogNullFile_Get();
    }
};

// clang-format on

TEST(SolidSyslogNullFile, OpenReturnsFalse)
{
    CHECK_FALSE(SolidSyslogFile_Open(file, "anywhere"));
}

TEST(SolidSyslogNullFile, CloseDoesNotCrash)
{
    SolidSyslogFile_Close(file);
}

TEST(SolidSyslogNullFile, IsOpenReturnsFalse)
{
    CHECK_FALSE(SolidSyslogFile_IsOpen(file));
}

TEST(SolidSyslogNullFile, ReadReturnsFalse)
{
    char buffer[1] = {0};
    CHECK_FALSE(SolidSyslogFile_Read(file, buffer, sizeof(buffer)));
}

TEST(SolidSyslogNullFile, WriteReturnsTrueToDropOnTheFloor)
{
    CHECK_TRUE(SolidSyslogFile_Write(file, "x", 1));
}

TEST(SolidSyslogNullFile, SeekToDoesNotCrash)
{
    SolidSyslogFile_SeekTo(file, 0U);
}

TEST(SolidSyslogNullFile, SizeReturnsZero)
{
    UNSIGNED_LONGS_EQUAL(0U, SolidSyslogFile_Size(file));
}

TEST(SolidSyslogNullFile, TruncateDoesNotCrash)
{
    SolidSyslogFile_Truncate(file);
}

TEST(SolidSyslogNullFile, ExistsReturnsFalse)
{
    CHECK_FALSE(SolidSyslogFile_Exists(file, "anywhere"));
}

TEST(SolidSyslogNullFile, DeleteReturnsTrueAsVacuouslySucceeded)
{
    CHECK_TRUE(SolidSyslogFile_Delete(file, "anywhere"));
}
