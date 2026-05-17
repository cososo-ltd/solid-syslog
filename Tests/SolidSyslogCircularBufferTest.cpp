#include <cstring>

#include "CppUTest/TestHarness.h"

#include "ConfigLockFake.h"
#include "ErrorHandlerFake.h"
#include "MutexFake.h"
#include "SolidSyslogBuffer.h"
#include "SolidSyslogBufferDefinition.h"
#include "SolidSyslogCircularBuffer.h"
#include "SolidSyslogErrorMessages.h"
#include "SolidSyslogNullMutex.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogTunables.h"
#include "TestUtils.h"

using namespace CososoTesting; // NOLINT(google-build-using-namespace) -- test-file scope only; brings ONCE/NEVER into scope for CALLED_FAKE

// Asserts buf is a non-null handle that is not one of the slots in pool.
// Used to pin the pool-exhaustion Fallback contract: every legitimate
// _Create returns either a pool slot or the Fallback singleton, never NULL.
// NOLINTBEGIN(cppcoreguidelines-macro-usage,cppcoreguidelines-avoid-do-while) -- macros preserve __FILE__/__LINE__ at the call site
#define CHECK_IS_FALLBACK(buf, pool)                                                 \
    do                                                                               \
    {                                                                                \
        CHECK_TEXT((buf) != nullptr, "Fallback handle was nullptr");                 \
        for (auto* slot : (pool))                                                    \
        {                                                                            \
            CHECK_TEXT(slot != nullptr, "pool slot was nullptr (FillPool failed?)"); \
            CHECK_TEXT((buf) != slot, "Fallback handle collided with a pool slot");  \
        }                                                                            \
    } while (0)

// NOLINTEND(cppcoreguidelines-macro-usage,cppcoreguidelines-avoid-do-while)

enum
{
    TEST_MAX_MESSAGES = 1
};

// clang-format off
TEST_GROUP(SolidSyslogCircularBuffer)
{
    uint8_t                   ring[SOLIDSYSLOG_CIRCULAR_BUFFER_RING_BYTES(TEST_MAX_MESSAGES)];
    struct SolidSyslogBuffer* buffer = nullptr;
    char                      readData[SOLIDSYSLOG_MAX_MESSAGE_SIZE];
    // cppcheck-suppress variableScope -- member of TEST_GROUP; scope managed by CppUTest macro
    size_t                    readSize;

    void setup() override
    {
        // cppcheck-suppress unreadVariable -- used across TEST_GROUP methods; cppcheck does not model CppUTest macros
        buffer   = SolidSyslogCircularBuffer_Create(SolidSyslogNullMutex_Create(), ring, sizeof(ring));
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
        CYCLES = 400,
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

    char bigDest[SOLIDSYSLOG_MAX_MESSAGE_SIZE + 1];
    size_t got = 0;
    CHECK_FALSE(SolidSyslogBuffer_Read(buffer, bigDest, sizeof(bigDest), &got));
}

// clang-format off
TEST_GROUP(SolidSyslogCircularBufferMutex)
{
    uint8_t                   ring[SOLIDSYSLOG_CIRCULAR_BUFFER_RING_BYTES(TEST_MAX_MESSAGES)];
    struct SolidSyslogBuffer* buffer = nullptr;
    char                      readData[SOLIDSYSLOG_MAX_MESSAGE_SIZE];
    // cppcheck-suppress variableScope -- member of TEST_GROUP; scope managed by CppUTest macro
    size_t                    readSize;

    void setup() override
    {
        // cppcheck-suppress unreadVariable -- used across TEST_GROUP methods; cppcheck does not model CppUTest macros
        buffer   = SolidSyslogCircularBuffer_Create(MutexFake_Create(), ring, sizeof(ring));
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
    uint8_t                          ring[SMALL_RING_BYTES];
    struct SolidSyslogBuffer*        buffer = nullptr;
    char                             readData[SMALL_RING_BYTES];
    // cppcheck-suppress variableScope -- member of TEST_GROUP; scope managed by CppUTest macro
    size_t                           readSize;

    void setup() override
    {
        // cppcheck-suppress unreadVariable -- used across TEST_GROUP methods; cppcheck does not model CppUTest macros
        buffer   = SolidSyslogCircularBuffer_Create(SolidSyslogNullMutex_Create(), ring, sizeof(ring));
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

    char dest[5];
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

// clang-format off
TEST_GROUP(SolidSyslogCircularBufferPool)
{
    uint8_t                   ring[SOLIDSYSLOG_CIRCULAR_BUFFER_RING_BYTES(1)];
    struct SolidSyslogMutex*  mutex                                           = nullptr;
    struct SolidSyslogBuffer* pooled[SOLIDSYSLOG_CIRCULAR_BUFFER_POOL_SIZE]   = {};
    struct SolidSyslogBuffer* overflow                                        = nullptr;

    void setup() override
    {
        // cppcheck-suppress unreadVariable -- used across TEST_GROUP methods; cppcheck does not model CppUTest macros
        mutex = SolidSyslogNullMutex_Create();
    }

    void teardown() override
    {
        for (auto* buffer : pooled)
        {
            SolidSyslogCircularBuffer_Destroy(buffer);
        }
        SolidSyslogCircularBuffer_Destroy(overflow);
        SolidSyslogNullMutex_Destroy();
        ConfigLockFake_Uninstall();
        ErrorHandlerFake_Uninstall();
    }

    struct SolidSyslogBuffer* MakeBuffer()
    {
        return SolidSyslogCircularBuffer_Create(mutex, ring, sizeof(ring));
    }

    void FillPool()
    {
        for (auto*& slot : pooled)
        {
            slot = MakeBuffer();
        }
    }
};

// clang-format on

TEST(SolidSyslogCircularBufferPool, FillingPoolThenOverflowReturnsDistinctFallback)
{
    FillPool();
    overflow = MakeBuffer();

    CHECK_IS_FALLBACK(overflow, pooled);
}

TEST(SolidSyslogCircularBufferPool, ExhaustedCreateReportsError)
{
    ErrorHandlerFake_Install(nullptr);
    FillPool();

    overflow = MakeBuffer();

    CALLED_FAKE(ErrorHandlerFake_Handle, ONCE);
    LONGS_EQUAL(SOLIDSYSLOG_SEVERITY_ERROR, ErrorHandlerFake_LastSeverity());
}

TEST(SolidSyslogCircularBufferPool, FallbackWriteAndReadAreNoOps)
{
    FillPool();
    overflow = MakeBuffer();

    SolidSyslogBuffer_Write(overflow, "hello", 5);

    char readBuffer[16] = {};
    size_t bytesRead = 99;
    CHECK_FALSE(SolidSyslogBuffer_Read(overflow, readBuffer, sizeof(readBuffer), &bytesRead));
    LONGS_EQUAL(0, bytesRead);
}

TEST(SolidSyslogCircularBufferPool, CreateAcquiresAndReleasesConfigLockOnFirstFreeSlot)
{
    ConfigLockFake_Install();

    pooled[0] = MakeBuffer();

    CALLED_FAKE(ConfigLockFake_Lock, ONCE);
    CALLED_FAKE(ConfigLockFake_Unlock, ONCE);
}

TEST(SolidSyslogCircularBufferPool, CreateLocksOncePerSlotProbedWhenPoolIsFull)
{
    FillPool();
    ConfigLockFake_Install();

    overflow = MakeBuffer();

    LONGS_EQUAL(SOLIDSYSLOG_CIRCULAR_BUFFER_POOL_SIZE, ConfigLockFake_LockCallCount());
    LONGS_EQUAL(SOLIDSYSLOG_CIRCULAR_BUFFER_POOL_SIZE, ConfigLockFake_UnlockCallCount());
}

TEST(SolidSyslogCircularBufferPool, DestroyOfPooledHandleLocksOnce)
{
    pooled[0] = MakeBuffer();
    ConfigLockFake_Install();

    SolidSyslogCircularBuffer_Destroy(pooled[0]);
    pooled[0] = nullptr;

    CALLED_FAKE(ConfigLockFake_Lock, ONCE);
    CALLED_FAKE(ConfigLockFake_Unlock, ONCE);
}

TEST(SolidSyslogCircularBufferPool, DestroyOfUnknownHandleDoesNotLock)
{
    ConfigLockFake_Install();
    struct SolidSyslogBuffer stranger = {};

    SolidSyslogCircularBuffer_Destroy(&stranger);

    CALLED_FAKE(ConfigLockFake_Lock, NEVER);
    CALLED_FAKE(ConfigLockFake_Unlock, NEVER);
}

TEST(SolidSyslogCircularBufferPool, DestroyOfUnknownHandleReportsWarning)
{
    ErrorHandlerFake_Install(nullptr);
    struct SolidSyslogBuffer stranger = {};

    SolidSyslogCircularBuffer_Destroy(&stranger);

    CALLED_FAKE(ErrorHandlerFake_Handle, ONCE);
    LONGS_EQUAL(SOLIDSYSLOG_SEVERITY_WARNING, ErrorHandlerFake_LastSeverity());
    STRCMP_EQUAL(SOLIDSYSLOG_ERROR_MSG_CIRCULARBUFFER_UNKNOWN_DESTROY, ErrorHandlerFake_LastMessage());
}

TEST(SolidSyslogCircularBufferPool, DestroyOfStaleHandleReportsWarning)
{
    pooled[0] = MakeBuffer();
    SolidSyslogCircularBuffer_Destroy(pooled[0]);
    ErrorHandlerFake_Install(nullptr);

    SolidSyslogCircularBuffer_Destroy(pooled[0]);
    pooled[0] = nullptr;

    CALLED_FAKE(ErrorHandlerFake_Handle, ONCE);
    LONGS_EQUAL(SOLIDSYSLOG_SEVERITY_WARNING, ErrorHandlerFake_LastSeverity());
    STRCMP_EQUAL(SOLIDSYSLOG_ERROR_MSG_CIRCULARBUFFER_UNKNOWN_DESTROY, ErrorHandlerFake_LastMessage());
}
