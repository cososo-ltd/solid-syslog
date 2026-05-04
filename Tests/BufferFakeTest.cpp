#include <stddef.h>

#include "BufferFake.h"
#include "SolidSyslogBuffer.h"
#include "CppUTest/TestHarness.h"

static const char* const TEST_MESSAGE     = "hello";
static const size_t      TEST_MESSAGE_LEN = 5;

// clang-format off
TEST_GROUP(BufferFake)
{
    struct SolidSyslogBuffer* buffer = nullptr;
    char   readData[512];
    // cppcheck-suppress variableScope -- member of TEST_GROUP; scope managed by CppUTest macro
    size_t readSize;

    void setup() override
    {
        // cppcheck-suppress unreadVariable -- used across TEST_GROUP methods; cppcheck does not model CppUTest macros
        buffer = BufferFake_Create();
        readSize = 0;
    }

    void teardown() override
    {
        BufferFake_Destroy();
    }

    void Write() const
    {
        SolidSyslogBuffer_Write(buffer, TEST_MESSAGE, TEST_MESSAGE_LEN);
    }

    bool Read()
    {
        return SolidSyslogBuffer_Read(buffer, readData, sizeof(readData), &readSize);
    }
};

// clang-format on

TEST(BufferFake, CreateDestroyWorksWithoutCrashing)
{
}

TEST(BufferFake, ReadFromEmptyReturnsFalse)
{
    CHECK_FALSE(Read());
}

TEST(BufferFake, WriteAndReadReturnsTrue)
{
    Write();
    CHECK_TRUE(Read());
}

TEST(BufferFake, ReadReturnsWrittenData)
{
    Write();
    Read();
    MEMCMP_EQUAL(TEST_MESSAGE, readData, TEST_MESSAGE_LEN);
}

TEST(BufferFake, ReadReturnsWrittenSize)
{
    Write();
    Read();
    LONGS_EQUAL(TEST_MESSAGE_LEN, readSize);
}

TEST(BufferFake, SecondReadReturnsFalse)
{
    Write();
    Read();
    CHECK_FALSE(Read());
}
