#include <cstring>
#include <stdint.h>

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

// Assertion macros at file scope so failures report the test's own
// __FILE__/__LINE__ rather than the helper's.
// NOLINTBEGIN(cppcoreguidelines-macro-usage,cppcoreguidelines-avoid-do-while) -- macros preserve __FILE__/__LINE__ at the call site

// Asserts the last record read equals `expected` bytes of length `size`.
// Depends on `readData` and `readSize` from CircularBufferFixture being in scope.
#define CHECK_LAST_READ_RECORD(expected, size)      \
    do                                              \
    {                                               \
        LONGS_EQUAL((size), readSize);              \
        MEMCMP_EQUAL((expected), readData, (size)); \
    } while (0)

// Asserts buf is a non-null handle that is not one of the slots in pool.
// Used to pin the pool-exhaustion Fallback contract: every legitimate
// _Create returns either a pool slot or the Fallback singleton, never NULL.
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
    TEST_MAX_MESSAGES = 1,
    SMALL_RING_BYTES = 32
};

// Shared fixture for any TEST_GROUP_BASE that drives one CircularBuffer through
// Write/Read. Each derived group supplies its own ring storage and its own
// setup/teardown (the mutex source differs per group). Test bodies reference
// `buffer`, `readData`, `readSize`, `Write(...)`, `Read()` directly because
// they are inherited members.
// clang-format off
TEST_BASE(CircularBufferFixture)
{
    struct SolidSyslogBuffer* buffer = nullptr;
    char                      readData[SOLIDSYSLOG_MAX_MESSAGE_SIZE] = {};
    size_t                    readSize = 0;

    // Write a null-terminated C string. Size is strlen() -- intended for
    // literal-string tests. For binary payloads use the two-arg overload below.
    void Write(const char* text) const
    {
        SolidSyslogBuffer_Write(buffer, text, strlen(text));
    }

    void Write(const void* data, size_t size) const
    {
        SolidSyslogBuffer_Write(buffer, data, size);
    }

    bool Read()
    {
        return SolidSyslogBuffer_Read(buffer, readData, sizeof(readData), &readSize);
    }
};

// clang-format on

// clang-format off
TEST_GROUP_BASE(SolidSyslogCircularBuffer, CircularBufferFixture)
{
    uint8_t ring[SOLIDSYSLOG_CIRCULAR_BUFFER_RING_BYTES(TEST_MAX_MESSAGES)];

    void setup() override
    {
        // cppcheck-suppress unreadVariable -- used across TEST_GROUP methods; cppcheck does not model CppUTest macros
        buffer = SolidSyslogCircularBuffer_Create(SolidSyslogNullMutex_Get(), ring, sizeof(ring));
    }

    void teardown() override
    {
        SolidSyslogCircularBuffer_Destroy(buffer);
    }
};

// clang-format on

TEST(SolidSyslogCircularBuffer, CreateDestroyDoesNotCrash)
{
}

TEST(SolidSyslogCircularBuffer, UseAfterDestroyIsCrashSafeViaNullBufferVtable)
{
    /* After Destroy the slot's abstract-base vtable is the shared NullBuffer's, so
     * Write/Read through the stale handle is a safe no-op rather than a NULL-fn-pointer
     * crash. NullBuffer.Write swallows; NullBuffer.Read returns false with bytesRead=0. */
    SolidSyslogCircularBuffer_Destroy(buffer);

    Write("x");
    CHECK_FALSE(Read());
    LONGS_EQUAL(0, readSize);

    buffer = SolidSyslogCircularBuffer_Create(SolidSyslogNullMutex_Get(), ring, sizeof(ring)); // for teardown
}

TEST(SolidSyslogCircularBuffer, ReadFromEmptyReturnsFalse)
{
    CHECK_FALSE(Read());
}

TEST(SolidSyslogCircularBuffer, WriteThenReadReturnsTrue)
{
    Write("hello");

    CHECK_TRUE(Read());
}

TEST(SolidSyslogCircularBuffer, ReadReturnsWrittenSize)
{
    Write("hello");

    Read();

    LONGS_EQUAL(5, readSize);
}

TEST(SolidSyslogCircularBuffer, ReadReturnsWrittenData)
{
    Write("hello");

    Read();

    MEMCMP_EQUAL("hello", readData, 5);
}

TEST(SolidSyslogCircularBuffer, SecondReadAfterSingleWriteReturnsFalse)
{
    Write("hello");
    Read();

    CHECK_FALSE(Read());
}

TEST(SolidSyslogCircularBuffer, TwoWritesReadInOrder)
{
    Write("first");
    Write("second");

    Read();
    CHECK_LAST_READ_RECORD("first", 5);
    Read();
    CHECK_LAST_READ_RECORD("second", 6);
}

TEST(SolidSyslogCircularBuffer, ThreeWritesReadInOrder)
{
    Write("alpha");
    Write("bravo");
    Write("charlie");

    Read();
    CHECK_LAST_READ_RECORD("alpha", 5);
    Read();
    CHECK_LAST_READ_RECORD("bravo", 5);
    Read();
    CHECK_LAST_READ_RECORD("charlie", 7);
}

TEST(SolidSyslogCircularBuffer, WrapsAroundEndOfStorage)
{
    constexpr size_t payloadSize = 10;
    constexpr int cycles = 400;
    char payload[payloadSize];
    memset(payload, 'X', payloadSize);

    for (int i = 0; i < cycles; i++)
    {
        Write(payload, payloadSize);
        Read();
        CHECK_LAST_READ_RECORD(payload, payloadSize);
    }
}

TEST(SolidSyslogCircularBuffer, OverflowingWriteIsDropped)
{
    char filler[SOLIDSYSLOG_MAX_MESSAGE_SIZE];
    memset(filler, 'A', sizeof(filler));
    char extra[100];
    memset(extra, 'B', sizeof(extra));

    Write(filler, sizeof(filler));
    Write(extra, sizeof(extra));

    Read();
    CHECK_LAST_READ_RECORD(filler, sizeof(filler));
    CHECK_FALSE(Read());
}

TEST(SolidSyslogCircularBuffer, WriteAfterDrainAcceptsRecordTooLargeForRemainingTailSpace)
{
    Write("x");
    Read();

    char big[SOLIDSYSLOG_MAX_MESSAGE_SIZE];
    memset(big, 'B', sizeof(big));
    Write(big, sizeof(big));

    CHECK_TRUE(Read());
    CHECK_LAST_READ_RECORD(big, sizeof(big));
}

TEST(SolidSyslogCircularBuffer, WriteExceedingMaxMessageSizeIsDropped)
{
    char tooBig[SOLIDSYSLOG_MAX_MESSAGE_SIZE + 1];
    memset(tooBig, 'X', sizeof(tooBig));
    Write(tooBig, sizeof(tooBig));

    CHECK_FALSE(Read());
}

// clang-format off
TEST_GROUP_BASE(SolidSyslogCircularBufferMutex, CircularBufferFixture)
{
    uint8_t ring[SOLIDSYSLOG_CIRCULAR_BUFFER_RING_BYTES(TEST_MAX_MESSAGES)];

    void setup() override
    {
        // cppcheck-suppress unreadVariable -- used across TEST_GROUP methods; cppcheck does not model CppUTest macros
        buffer = SolidSyslogCircularBuffer_Create(MutexFake_Create(), ring, sizeof(ring));
    }

    void teardown() override
    {
        SolidSyslogCircularBuffer_Destroy(buffer);
        MutexFake_Destroy();
    }
};

// clang-format on

TEST(SolidSyslogCircularBufferMutex, WriteCallsLockThenUnlockOnce)
{
    Write("hello");

    STRCMP_EQUAL("LU", MutexFake_Sequence());
}

TEST(SolidSyslogCircularBufferMutex, ReadCallsLockThenUnlockOnce)
{
    Read();

    STRCMP_EQUAL("LU", MutexFake_Sequence());
}

// clang-format off
TEST_GROUP_BASE(SolidSyslogCircularBufferSmallRing, CircularBufferFixture)
{
    uint8_t ring[SMALL_RING_BYTES];

    void setup() override
    {
        // cppcheck-suppress unreadVariable -- used across TEST_GROUP methods; cppcheck does not model CppUTest macros
        buffer = SolidSyslogCircularBuffer_Create(SolidSyslogNullMutex_Get(), ring, sizeof(ring));
    }

    void teardown() override
    {
        SolidSyslogCircularBuffer_Destroy(buffer);
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
    Write(rec, sizeof(rec));
    Write(rec, sizeof(rec));
    Read();
    Write(rec, sizeof(rec));

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

    Write(a, sizeof(a));
    Write(b, sizeof(b));
    Read();
    Write(c, sizeof(c));
    Read();
    Write(d, sizeof(d));
    Write(e, sizeof(e));

    CHECK_TRUE(Read());
    CHECK_LAST_READ_RECORD(c, sizeof(c));
}

TEST(SolidSyslogCircularBufferSmallRing, ReadIntoSmallerBufferReturnsFalseAndLeavesRecordQueued)
{
    Write("hello");

    char dest[5];
    size_t got = 0;
    CHECK_FALSE(SolidSyslogBuffer_Read(buffer, dest, 4, &got));
    LONGS_EQUAL(0, got);

    CHECK_TRUE(Read());
    CHECK_LAST_READ_RECORD("hello", 5);
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

    Write(a, sizeof(a));
    Write(b, sizeof(b));
    Read();
    Write(c, sizeof(c));
    Write(d, sizeof(d));

    CHECK_TRUE(Read());
    CHECK_LAST_READ_RECORD(b, sizeof(b));
    CHECK_TRUE(Read());
    CHECK_LAST_READ_RECORD(c, sizeof(c));
    CHECK_TRUE(Read());
    CHECK_LAST_READ_RECORD(d, sizeof(d));
    CHECK_FALSE(Read());
}

// clang-format off
TEST_GROUP(SolidSyslogCircularBufferPool)
{
    uint8_t                   ring[SOLIDSYSLOG_CIRCULAR_BUFFER_RING_BYTES(1)];
    struct SolidSyslogMutex*  mutex                                         = nullptr;
    struct SolidSyslogBuffer* pooled[SOLIDSYSLOG_CIRCULAR_BUFFER_POOL_SIZE] = {};
    struct SolidSyslogBuffer* overflow                                      = nullptr;

    void setup() override
    {
        // cppcheck-suppress unreadVariable -- used across TEST_GROUP methods; cppcheck does not model CppUTest macros
        mutex = SolidSyslogNullMutex_Get();
    }

    void teardown() override
    {
        for (auto* handle : pooled)
        {
            if (handle != nullptr)
            {
                SolidSyslogCircularBuffer_Destroy(handle);
            }
        }
        if (overflow != nullptr)
        {
            SolidSyslogCircularBuffer_Destroy(overflow);
        }
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
