#include "CppUTest/TestHarness.h"
#include "SolidSyslogBuffer.h"
#include "SolidSyslogCircularBuffer.h"

// clang-format off
TEST_GROUP(SolidSyslogCircularBuffer)
{
    struct SolidSyslogBuffer* buffer = nullptr;
    char                      readData[512];
    // cppcheck-suppress variableScope -- member of TEST_GROUP; scope managed by CppUTest macro
    size_t                    readSize;

    void setup() override
    {
        // cppcheck-suppress unreadVariable -- used across TEST_GROUP methods; cppcheck does not model CppUTest macros
        buffer   = SolidSyslogCircularBuffer_Create();
        readSize = 0;
    }

    void teardown() override
    {
        SolidSyslogCircularBuffer_Destroy();
    }
};

// clang-format on

TEST(SolidSyslogCircularBuffer, CreateDestroyDoesNotCrash)
{
}

TEST(SolidSyslogCircularBuffer, ReadFromEmptyReturnsFalse)
{
    CHECK_FALSE(SolidSyslogBuffer_Read(buffer, readData, sizeof(readData), &readSize));
}

TEST(SolidSyslogCircularBuffer, WriteThenReadReturnsTrue)
{
    SolidSyslogBuffer_Write(buffer, "hello", 5);
    CHECK_TRUE(SolidSyslogBuffer_Read(buffer, readData, sizeof(readData), &readSize));
}

TEST(SolidSyslogCircularBuffer, ReadReturnsWrittenSize)
{
    SolidSyslogBuffer_Write(buffer, "hello", 5);
    SolidSyslogBuffer_Read(buffer, readData, sizeof(readData), &readSize);
    LONGS_EQUAL(5, readSize);
}

TEST(SolidSyslogCircularBuffer, ReadReturnsWrittenData)
{
    SolidSyslogBuffer_Write(buffer, "hello", 5);
    SolidSyslogBuffer_Read(buffer, readData, sizeof(readData), &readSize);
    MEMCMP_EQUAL("hello", readData, 5);
}

TEST(SolidSyslogCircularBuffer, SecondReadAfterSingleWriteReturnsFalse)
{
    SolidSyslogBuffer_Write(buffer, "hello", 5);
    SolidSyslogBuffer_Read(buffer, readData, sizeof(readData), &readSize);
    CHECK_FALSE(SolidSyslogBuffer_Read(buffer, readData, sizeof(readData), &readSize));
}

TEST(SolidSyslogCircularBuffer, TwoWritesReadInOrder)
{
    SolidSyslogBuffer_Write(buffer, "first", 5);
    SolidSyslogBuffer_Write(buffer, "second", 6);
    SolidSyslogBuffer_Read(buffer, readData, sizeof(readData), &readSize);
    MEMCMP_EQUAL("first", readData, 5);
    SolidSyslogBuffer_Read(buffer, readData, sizeof(readData), &readSize);
    MEMCMP_EQUAL("second", readData, 6);
}

TEST(SolidSyslogCircularBuffer, ThreeWritesReadInOrder)
{
    SolidSyslogBuffer_Write(buffer, "alpha", 5);
    SolidSyslogBuffer_Write(buffer, "bravo", 5);
    SolidSyslogBuffer_Write(buffer, "charlie", 7);
    SolidSyslogBuffer_Read(buffer, readData, sizeof(readData), &readSize);
    MEMCMP_EQUAL("alpha", readData, 5);
    SolidSyslogBuffer_Read(buffer, readData, sizeof(readData), &readSize);
    MEMCMP_EQUAL("bravo", readData, 5);
    SolidSyslogBuffer_Read(buffer, readData, sizeof(readData), &readSize);
    MEMCMP_EQUAL("charlie", readData, 7);
}
