#include <cstring>

#include "CppUTest/TestHarness.h"
#include "MutexFake.h"
#include "SolidSyslogBuffer.h"
#include "SolidSyslogCircularBuffer.h"
#include "SolidSyslogNullMutex.h"
#include "SolidSyslogTunables.h"

enum
{
    TEST_MAX_MESSAGES = 1
};

// clang-format off
TEST_GROUP(SolidSyslogCircularBuffer)
{
    SolidSyslogCircularBufferStorage storage[SOLIDSYSLOG_CIRCULARBUFFER_STORAGE_SIZE(TEST_MAX_MESSAGES)];
    struct SolidSyslogBuffer* buffer = nullptr;
    char                      readData[SOLIDSYSLOG_MAX_MESSAGE_SIZE];
    // cppcheck-suppress variableScope -- member of TEST_GROUP; scope managed by CppUTest macro
    size_t                    readSize;

    void setup() override
    {
        // cppcheck-suppress unreadVariable -- used across TEST_GROUP methods; cppcheck does not model CppUTest macros
        buffer   = SolidSyslogCircularBuffer_Create(storage, sizeof(storage), SolidSyslogNullMutex_Create());
        readSize = 0;
    }

    void teardown() override
    {
        SolidSyslogCircularBuffer_Destroy(buffer);
        SolidSyslogNullMutex_Destroy();
    }

    bool Read()
    {
        return SolidSyslogBuffer_Read(buffer, readData, sizeof(readData), &readSize);
    }
};

// clang-format on

TEST(SolidSyslogCircularBuffer, CreateDestroyDoesNotCrash)
{
}

TEST(SolidSyslogCircularBuffer, HandleEqualsStorageAddress)
{
    POINTERS_EQUAL(&storage, buffer);
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

TEST(SolidSyslogCircularBuffer, WrapsAroundEndOfStorage)
{
    enum
    {
        CYCLES       = 400,
        PAYLOAD_SIZE = 10
    };

    char payload[PAYLOAD_SIZE];
    memset(payload, 'X', PAYLOAD_SIZE);

    for (int i = 0; i < CYCLES; i++)
    {
        SolidSyslogBuffer_Write(buffer, payload, PAYLOAD_SIZE);
        SolidSyslogBuffer_Read(buffer, readData, sizeof(readData), &readSize);
        LONGS_EQUAL(PAYLOAD_SIZE, readSize);
        MEMCMP_EQUAL(payload, readData, PAYLOAD_SIZE);
    }
}

TEST(SolidSyslogCircularBuffer, OverflowingWriteIsDropped)
{
    char filler[SOLIDSYSLOG_MAX_MESSAGE_SIZE];
    memset(filler, 'A', sizeof(filler));
    char overflow[100];
    memset(overflow, 'B', sizeof(overflow));

    SolidSyslogBuffer_Write(buffer, filler, sizeof(filler));
    SolidSyslogBuffer_Write(buffer, overflow, sizeof(overflow));

    SolidSyslogBuffer_Read(buffer, readData, sizeof(readData), &readSize);
    LONGS_EQUAL(sizeof(filler), readSize);
    MEMCMP_EQUAL(filler, readData, sizeof(filler));
    CHECK_FALSE(SolidSyslogBuffer_Read(buffer, readData, sizeof(readData), &readSize));
}

TEST(SolidSyslogCircularBuffer, WriteAfterDrainAcceptsRecordTooLargeForRemainingTailSpace)
{
    SolidSyslogBuffer_Write(buffer, "x", 1);
    SolidSyslogBuffer_Read(buffer, readData, sizeof(readData), &readSize);

    char big[SOLIDSYSLOG_MAX_MESSAGE_SIZE];
    memset(big, 'B', sizeof(big));
    SolidSyslogBuffer_Write(buffer, big, sizeof(big));

    CHECK_TRUE(SolidSyslogBuffer_Read(buffer, readData, sizeof(readData), &readSize));
    LONGS_EQUAL(sizeof(big), readSize);
    MEMCMP_EQUAL(big, readData, sizeof(big));
}

TEST(SolidSyslogCircularBuffer, WriteExceedingMaxMessageSizeIsDropped)
{
    char tooBig[SOLIDSYSLOG_MAX_MESSAGE_SIZE + 1];
    memset(tooBig, 'X', sizeof(tooBig));
    SolidSyslogBuffer_Write(buffer, tooBig, sizeof(tooBig));

    char   bigDest[SOLIDSYSLOG_MAX_MESSAGE_SIZE + 1];
    size_t got = 0;
    CHECK_FALSE(SolidSyslogBuffer_Read(buffer, bigDest, sizeof(bigDest), &got));
}

// clang-format off
TEST_GROUP(SolidSyslogCircularBufferMutex)
{
    SolidSyslogCircularBufferStorage storage[SOLIDSYSLOG_CIRCULARBUFFER_STORAGE_SIZE(TEST_MAX_MESSAGES)];
    struct SolidSyslogBuffer* buffer = nullptr;
    char                      readData[SOLIDSYSLOG_MAX_MESSAGE_SIZE];
    // cppcheck-suppress variableScope -- member of TEST_GROUP; scope managed by CppUTest macro
    size_t                    readSize;

    void setup() override
    {
        // cppcheck-suppress unreadVariable -- used across TEST_GROUP methods; cppcheck does not model CppUTest macros
        buffer   = SolidSyslogCircularBuffer_Create(storage, sizeof(storage), MutexFake_Create());
        readSize = 0;
    }

    void teardown() override
    {
        SolidSyslogCircularBuffer_Destroy(buffer);
        MutexFake_Destroy();
    }

    bool Read()
    {
        return SolidSyslogBuffer_Read(buffer, readData, sizeof(readData), &readSize);
    }
};

// clang-format on

TEST(SolidSyslogCircularBufferMutex, WriteCallsLockThenUnlockOnce)
{
    SolidSyslogBuffer_Write(buffer, "hello", 5);
    STRCMP_EQUAL("LU", MutexFake_Sequence());
}

TEST(SolidSyslogCircularBufferMutex, ReadCallsLockThenUnlockOnce)
{
    SolidSyslogBuffer_Read(buffer, readData, sizeof(readData), &readSize);
    STRCMP_EQUAL("LU", MutexFake_Sequence());
}

enum
{
    SMALL_RING_BYTES = 32
};

// clang-format off
TEST_GROUP(SolidSyslogCircularBufferSmallRing)
{
    SolidSyslogCircularBufferStorage storage[SOLIDSYSLOG_CIRCULARBUFFER_STORAGE_SIZE_BYTES(SMALL_RING_BYTES)];
    struct SolidSyslogBuffer*        buffer = nullptr;
    char                             readData[SMALL_RING_BYTES];
    // cppcheck-suppress variableScope -- member of TEST_GROUP; scope managed by CppUTest macro
    size_t                           readSize;

    void setup() override
    {
        // cppcheck-suppress unreadVariable -- used across TEST_GROUP methods; cppcheck does not model CppUTest macros
        buffer   = SolidSyslogCircularBuffer_Create(storage, sizeof(storage), SolidSyslogNullMutex_Create());
        readSize = 0;
    }

    void teardown() override
    {
        SolidSyslogCircularBuffer_Destroy(buffer);
        SolidSyslogNullMutex_Destroy();
    }

    bool Read()
    {
        return SolidSyslogBuffer_Read(buffer, readData, sizeof(readData), &readSize);
    }
};

// clang-format on

// 32-byte ring, 2-byte header per record.
// recA(12)→ tail=14, recB(12)→ tail=28, read recA → head=14.
// recC(12) recordBytes=14: doesn't fit at tail (28+14=42>32);
// canWrap: recordBytes(14) vs head(14). With `<=` bug: wraps, tail=14=head → IsEmpty.
// With `<` fix: drops; recB still readable.
TEST(SolidSyslogCircularBufferSmallRing, WrapWriteFillingExactlyToHeadDoesNotCollapseToEmpty)
{
    char rec[12];
    memset(rec, 'A', sizeof(rec));
    SolidSyslogBuffer_Write(buffer, rec, sizeof(rec));
    SolidSyslogBuffer_Write(buffer, rec, sizeof(rec));
    Read();
    SolidSyslogBuffer_Write(buffer, rec, sizeof(rec));

    CHECK_TRUE(Read());
    LONGS_EQUAL(sizeof(rec), readSize);
}

// 32-byte ring. Establish wrapped state (head=24, tail=3 after a, b, c, d), then
// write e of size 19 → recordBytes=21 = head-tail. With `<=` bug, fitsAtTail in
// wrapped state lets tail advance to head exactly → IsEmpty collapse.
TEST(SolidSyslogCircularBufferSmallRing, WriteInWrappedStateFillingExactlyToHeadDoesNotCollapseToEmpty)
{
    char a[10];
    char b[10];
    char c[4];
    char d[1];
    char e[19];
    memset(a, 'a', sizeof(a));
    memset(b, 'b', sizeof(b));
    memset(c, 'c', sizeof(c));
    memset(d, 'd', sizeof(d));
    memset(e, 'e', sizeof(e));

    SolidSyslogBuffer_Write(buffer, a, sizeof(a));
    SolidSyslogBuffer_Write(buffer, b, sizeof(b));
    Read();
    SolidSyslogBuffer_Write(buffer, c, sizeof(c));
    Read();
    SolidSyslogBuffer_Write(buffer, d, sizeof(d));
    SolidSyslogBuffer_Write(buffer, e, sizeof(e));

    CHECK_TRUE(Read());
    LONGS_EQUAL(sizeof(c), readSize);
    MEMCMP_EQUAL(c, readData, sizeof(c));
}

TEST(SolidSyslogCircularBufferSmallRing, ReadIntoSmallerBufferReturnsFalseAndLeavesRecordQueued)
{
    SolidSyslogBuffer_Write(buffer, "hello", 5);

    char   dest[5];
    size_t got = 0;
    CHECK_FALSE(SolidSyslogBuffer_Read(buffer, dest, 4, &got));
    LONGS_EQUAL(0, got);

    CHECK_TRUE(Read());
    LONGS_EQUAL(5, readSize);
    MEMCMP_EQUAL("hello", readData, 5);
}

// Exercises the ConsumeWrapMarker branch of Read: write A, B, drain A, write
// C, D (D forces a wrap), then drain in order. The fourth read must cross
// the wrap point — head reaches wrapPoint and jumps to 0 to read D.
TEST(SolidSyslogCircularBufferSmallRing, WrappedBufferReadsAllRecordsInOrder)
{
    char a[10];
    char b[10];
    char c[4];
    char d[1];
    memset(a, 'a', sizeof(a));
    memset(b, 'b', sizeof(b));
    memset(c, 'c', sizeof(c));
    memset(d, 'd', sizeof(d));

    SolidSyslogBuffer_Write(buffer, a, sizeof(a));
    SolidSyslogBuffer_Write(buffer, b, sizeof(b));
    Read();
    SolidSyslogBuffer_Write(buffer, c, sizeof(c));
    SolidSyslogBuffer_Write(buffer, d, sizeof(d));

    CHECK_TRUE(Read());
    MEMCMP_EQUAL(b, readData, sizeof(b));
    CHECK_TRUE(Read());
    MEMCMP_EQUAL(c, readData, sizeof(c));
    CHECK_TRUE(Read());
    MEMCMP_EQUAL(d, readData, sizeof(d));
    CHECK_FALSE(Read());
}
