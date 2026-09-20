#include "CppUTest/TestHarness.h"
#include "TestUtils.h"

extern "C"
{
#include "LittleFsFake.h"
#include "SolidSyslogFile.h"
#include "SolidSyslogLittleFsFile.h"
#include "lfs.h"
}

using namespace CososoTesting;

static const lfs_size_t TEST_CACHE_SIZE = 64;

// clang-format off
TEST_GROUP(SolidSyslogLittleFsFile)
{
    struct SolidSyslogFile* file = nullptr;
    lfs_t* filesystem = nullptr;
    unsigned char fileBuffer[TEST_CACHE_SIZE] = {};

    void setup() override
    {
        LittleFsFake_Reset();
        filesystem = LittleFsFake_MountedWithCacheSize(TEST_CACHE_SIZE);
        file = SolidSyslogLittleFsFile_Create(filesystem, fileBuffer, sizeof(fileBuffer));
    }

    void teardown() override
    {
        SolidSyslogLittleFsFile_Destroy(file);
    }
};

// clang-format on

TEST(SolidSyslogLittleFsFile, IsOpenIsFalseAfterCreate)
{
    CHECK_FALSE(SolidSyslogFile_IsOpen(file));
}
