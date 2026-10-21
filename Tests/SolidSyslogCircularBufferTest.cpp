#include <cstring>
#include <stdint.h>

#include "CppUTest/TestHarness.h"

#include "ConfigLockFake.h"
#include "ErrorHandlerFake.h"
#include "MutexFake.h"
#include "SolidSyslogBuffer.h"
#include "SolidSyslogBufferCategories.h"
#include "SolidSyslogBufferDefinition.h"
#include "SolidSyslogCircularBuffer.h"
#include "SolidSyslogCircularBufferErrors.h"
#include "SolidSyslogDiscardPolicy.h"
#include "SolidSyslogError.h"
#include "SolidSyslogErrorCategory.h"
#include "SolidSyslogNullMutex.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogTunables.h"
#include "TestUtils.h"

using namespace CososoTesting;

// Assertion macros at file scope so failures report the test's own
// __FILE__/__LINE__ rather than the helper's.

// Asserts the last record read equals `expected` bytes of length `size`.
// Depends on `readData` and `readSize` from CircularBufferFixture being in scope.
#define CHECK_LAST_READ_RECORD(expected, size)      \
    {                                               \
        LONGS_EQUAL((size), readSize);              \
        MEMCMP_EQUAL((expected), readData, (size)); \
    }

// Asserts buf is a non-null handle that is not one of the slots in pool.
// Used to pin the pool-exhaustion Fallback contract: every legitimate
// SolidSyslogCircularBuffer_Create returns either a pool slot or the Fallback
// singleton, never NULL.
#define CHECK_IS_FALLBACK(buf, pool)                                                 \
    {                                                                                \
        CHECK_TEXT((buf) != nullptr, "Fallback handle was nullptr");                 \
        for (auto* slot : (pool))                                                    \
        {                                                                            \
            CHECK_TEXT(slot != nullptr, "pool slot was nullptr (FillPool failed?)"); \
            CHECK_TEXT((buf) != slot, "Fallback handle collided with a pool slot");  \
        }                                                                            \
    }

enum
{
    TEST_MAX_MESSAGES = 1,
    SMALL_RING_BYTES = 32
};

/* The policy every ring in this file is created with. Each group's setup
 * resets it to Newest, so the refusal tests mean what they say; a test that
 * wants another policy sets it before writing. */
static enum SolidSyslogDiscardPolicy testPolicy;

static enum SolidSyslogDiscardPolicy AnswersTestPolicy(void* context)
{
    (void) context;
    return testPolicy;
}

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
    size_t                    readSize = 0U;

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
        testPolicy = SOLIDSYSLOG_DISCARD_POLICY_NEWEST;
        buffer = SolidSyslogCircularBuffer_Create(SolidSyslogNullMutex_Get(), ring, sizeof(ring), AnswersTestPolicy, nullptr);
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

    buffer = SolidSyslogCircularBuffer_Create(
        SolidSyslogNullMutex_Get(),
        ring,
        sizeof(ring),
        AnswersTestPolicy,
        nullptr
    ); // for teardown
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

TEST(SolidSyslogCircularBuffer, AFullRingReportsTheRecordLost)
{
    char filler[SOLIDSYSLOG_MAX_MESSAGE_SIZE];
    memset(filler, 'A', sizeof(filler));
    Write(filler, sizeof(filler));
    ErrorHandlerFake_Install(nullptr);

    Write("extra");

    CHECK_ERROR_REPORTED_ONCE(
        SOLIDSYSLOG_SEVERITY_WARNING,
        &SolidSyslogCircularBufferErrorSource,
        SOLIDSYSLOG_CAT_BUFFER_RECORDS_LOST,
        SOLIDSYSLOG_CIRCULAR_BUFFER_ERROR_RECORD_REFUSED
    );
}

TEST(SolidSyslogCircularBuffer, ARunOfRefusedRecordsIsReportedOnce)
{
    char filler[SOLIDSYSLOG_MAX_MESSAGE_SIZE];
    memset(filler, 'A', sizeof(filler));
    Write(filler, sizeof(filler));
    ErrorHandlerFake_Install(nullptr);

    Write("extra");
    Write("extra");

    CALLED_FAKE(ErrorHandlerFake_Handle, ONCE);
}

TEST(SolidSyslogCircularBuffer, ARecordTheRingKeepsEndsTheRunSoTheNextRefusalIsReportedAgain)
{
    char filler[SOLIDSYSLOG_MAX_MESSAGE_SIZE];
    memset(filler, 'A', sizeof(filler));
    Write(filler, sizeof(filler));
    ErrorHandlerFake_Install(nullptr);
    Write("extra"); /* refused */
    Read();
    Write(filler, sizeof(filler)); /* kept */

    Write("extra"); /* refused */

    CALLED_FAKE(ErrorHandlerFake_Handle, TWICE);
}

TEST(SolidSyslogCircularBuffer, AnEmptyRingIsNotFull)
{
    CHECK_FALSE(SolidSyslogBuffer_IsFull(buffer));
}

TEST(SolidSyslogCircularBuffer, ARingThatRefusedTheLastRecordIsFull)
{
    char filler[SOLIDSYSLOG_MAX_MESSAGE_SIZE];
    memset(filler, 'A', sizeof(filler));
    Write(filler, sizeof(filler));

    Write("extra");

    CHECK_TRUE(SolidSyslogBuffer_IsFull(buffer));
}

TEST(SolidSyslogCircularBuffer, AReadThatTakesARecordClearsFull)
{
    char filler[SOLIDSYSLOG_MAX_MESSAGE_SIZE];
    memset(filler, 'A', sizeof(filler));
    Write(filler, sizeof(filler));
    Write("extra");

    Read();

    CHECK_FALSE(SolidSyslogBuffer_IsFull(buffer));
}

TEST(SolidSyslogCircularBuffer, AWriteThatFitsClearsFull)
{
    char filler[SOLIDSYSLOG_MAX_MESSAGE_SIZE - 8];
    memset(filler, 'A', sizeof(filler));
    Write(filler, sizeof(filler));
    Write(filler, sizeof(filler)); /* refused */

    Write("fits");

    CHECK_FALSE(SolidSyslogBuffer_IsFull(buffer));
}

TEST(SolidSyslogCircularBuffer, UnderOldestAFullRingDropsItsOldestRecordToMakeRoom)
{
    testPolicy = SOLIDSYSLOG_DISCARD_POLICY_OLDEST;
    char older[SOLIDSYSLOG_MAX_MESSAGE_SIZE];
    memset(older, 'A', sizeof(older));
    char newer[SOLIDSYSLOG_MAX_MESSAGE_SIZE];
    memset(newer, 'B', sizeof(newer));
    Write(older, sizeof(older));

    Write(newer, sizeof(newer));

    CHECK_TRUE(Read());
    CHECK_LAST_READ_RECORD(newer, sizeof(newer));
    CHECK_FALSE(Read());
}

TEST(SolidSyslogCircularBuffer, UnderOldestARingThatMadeRoomIsFull)
{
    testPolicy = SOLIDSYSLOG_DISCARD_POLICY_OLDEST;
    char filler[SOLIDSYSLOG_MAX_MESSAGE_SIZE];
    memset(filler, 'A', sizeof(filler));
    Write(filler, sizeof(filler));

    Write(filler, sizeof(filler));

    CHECK_TRUE(SolidSyslogBuffer_IsFull(buffer));
}

TEST(SolidSyslogCircularBuffer, UnderOldestEvictingIsReportedAsRecordsLost)
{
    testPolicy = SOLIDSYSLOG_DISCARD_POLICY_OLDEST;
    char filler[SOLIDSYSLOG_MAX_MESSAGE_SIZE];
    memset(filler, 'A', sizeof(filler));
    Write(filler, sizeof(filler));
    ErrorHandlerFake_Install(nullptr);

    Write(filler, sizeof(filler));

    CHECK_ERROR_REPORTED_ONCE(
        SOLIDSYSLOG_SEVERITY_WARNING,
        &SolidSyslogCircularBufferErrorSource,
        SOLIDSYSLOG_CAT_BUFFER_RECORDS_LOST,
        SOLIDSYSLOG_CIRCULAR_BUFFER_ERROR_RECORDS_EVICTED
    );
}

TEST(SolidSyslogCircularBuffer, ARunOfEvictionsIsReportedOnce)
{
    testPolicy = SOLIDSYSLOG_DISCARD_POLICY_OLDEST;
    char filler[SOLIDSYSLOG_MAX_MESSAGE_SIZE];
    memset(filler, 'A', sizeof(filler));
    Write(filler, sizeof(filler));
    ErrorHandlerFake_Install(nullptr);

    Write(filler, sizeof(filler));
    Write(filler, sizeof(filler));

    CALLED_FAKE(ErrorHandlerFake_Handle, ONCE);
}

TEST(SolidSyslogCircularBuffer, AWriteThatEvictsNothingEndsTheRunSoTheNextEvictionIsReportedAgain)
{
    testPolicy = SOLIDSYSLOG_DISCARD_POLICY_OLDEST;
    char filler[SOLIDSYSLOG_MAX_MESSAGE_SIZE];
    memset(filler, 'A', sizeof(filler));
    Write(filler, sizeof(filler));
    ErrorHandlerFake_Install(nullptr);
    Write(filler, sizeof(filler)); /* evicts */
    Read();
    Write(filler, sizeof(filler)); /* fits */

    Write(filler, sizeof(filler)); /* evicts */

    CALLED_FAKE(ErrorHandlerFake_Handle, TWICE);
}

TEST(SolidSyslogCircularBuffer, UnderHaltAFullRingRefusesTheRecordAsUnderNewest)
{
    testPolicy = SOLIDSYSLOG_DISCARD_POLICY_HALT;
    char filler[SOLIDSYSLOG_MAX_MESSAGE_SIZE];
    memset(filler, 'A', sizeof(filler));
    Write(filler, sizeof(filler));

    Write("extra");

    CHECK_TRUE(Read());
    CHECK_LAST_READ_RECORD(filler, sizeof(filler));
    CHECK_FALSE(Read());
}

TEST(SolidSyslogCircularBuffer, NoPolicyInjectedAppliesOldest)
{
    SolidSyslogCircularBuffer_Destroy(buffer);
    buffer = SolidSyslogCircularBuffer_Create(SolidSyslogNullMutex_Get(), ring, sizeof(ring), nullptr, nullptr);
    char older[SOLIDSYSLOG_MAX_MESSAGE_SIZE];
    memset(older, 'A', sizeof(older));
    char newer[SOLIDSYSLOG_MAX_MESSAGE_SIZE];
    memset(newer, 'B', sizeof(newer));
    Write(older, sizeof(older));

    Write(newer, sizeof(newer));

    CHECK_TRUE(Read());
    CHECK_LAST_READ_RECORD(newer, sizeof(newer));
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

TEST(SolidSyslogCircularBuffer, AWriteLargerThanAnyMessageIsReportedAsABadArgument)
{
    char tooBig[SOLIDSYSLOG_MAX_MESSAGE_SIZE + 1];
    memset(tooBig, 'X', sizeof(tooBig));
    ErrorHandlerFake_Install(nullptr);

    Write(tooBig, sizeof(tooBig));

    CHECK_ERROR_REPORTED_ONCE(
        SOLIDSYSLOG_BAD_ARGUMENT_SEVERITY,
        &SolidSyslogCircularBufferErrorSource,
        SOLIDSYSLOG_CAT_BAD_ARGUMENT,
        SOLIDSYSLOG_CIRCULAR_BUFFER_ERROR_RECORD_EXCEEDS_MAX_MESSAGE_SIZE
    );
}

TEST(SolidSyslogCircularBuffer, ReadWithBufferTooSmallForHeadRecordReportsError)
{
    ErrorHandlerFake_Install(nullptr);
    Write("hello");

    char small[4] = {};
    size_t got = 0U;
    bool delivered = SolidSyslogBuffer_Read(buffer, small, sizeof(small), &got);

    CHECK_FALSE(delivered);
    LONGS_EQUAL(0, got);
    CHECK_ERROR_REPORTED_ONCE(
        SOLIDSYSLOG_SEVERITY_ERROR,
        &SolidSyslogCircularBufferErrorSource,
        SOLIDSYSLOG_CAT_BUFFER_BACKEND_FAILED,
        SOLIDSYSLOG_CIRCULAR_BUFFER_ERROR_RECORD_TOO_LARGE
    );
}

TEST(SolidSyslogCircularBuffer, ReadWithBufferTooSmallLeavesRecordQueued)
{
    Write("hello");

    char small[4] = {};
    size_t got = 0U;
    (void) SolidSyslogBuffer_Read(buffer, small, sizeof(small), &got);

    CHECK_TRUE(Read()); // fixture Read() uses a MAX-sized buffer
    CHECK_LAST_READ_RECORD("hello", 5);
}

// clang-format off
TEST_GROUP_BASE(SolidSyslogCircularBufferMutex, CircularBufferFixture)
{
    uint8_t ring[SOLIDSYSLOG_CIRCULAR_BUFFER_RING_BYTES(TEST_MAX_MESSAGES)];

    void setup() override
    {
        testPolicy = SOLIDSYSLOG_DISCARD_POLICY_NEWEST;
        buffer = SolidSyslogCircularBuffer_Create(MutexFake_Create(), ring, sizeof(ring), AnswersTestPolicy, nullptr);
    }

    void teardown() override
    {
        SolidSyslog_SetErrorHandler(nullptr, nullptr);
        SolidSyslogCircularBuffer_Destroy(buffer);
        MutexFake_Destroy();
    }
};

// clang-format on

static int LogIntoTheBufferCallCount;

/* An integrator's handler that logs the report, which lands in the same full
 * ring. */
static void LogIntoTheBuffer(void* context, const struct SolidSyslogErrorEvent* event)
{
    (void) event;
    LogIntoTheBufferCallCount++;
    SolidSyslogBuffer_Write(static_cast<struct SolidSyslogBuffer*>(context), "nested", 6);
}

TEST(SolidSyslogCircularBufferMutex, AHandlerThatLogsTheLossIntoTheFullRingIsCalledOnceAndNotUnderTheLock)
{
    char filler[SOLIDSYSLOG_MAX_MESSAGE_SIZE];
    memset(filler, 'A', sizeof(filler));
    Write(filler, sizeof(filler));
    LogIntoTheBufferCallCount = 0;
    SolidSyslog_SetErrorHandler(LogIntoTheBuffer, buffer);

    Write("extra");

    CALLED_FUNCTION(LogIntoTheBuffer, ONCE);
    STRCMP_EQUAL("LULULU", MutexFake_Sequence());
}

/* A record the handler logs that does fit is a record kept, so it ends the run
 * and the next refusal is reported again. Each such report costs one handler
 * record, so the reports stop once those records have filled the room left. */
TEST(SolidSyslogCircularBufferMutex, HandlerRecordsThatFitEndTheRunOnlyUntilTheyFillTheRing)
{
    enum
    {
        ROOM_LEFT = 100,
        ATTEMPTS = 50
    };

    char partFiller[SOLIDSYSLOG_MAX_MESSAGE_SIZE - ROOM_LEFT];
    memset(partFiller, 'A', sizeof(partFiller));
    Write(partFiller, sizeof(partFiller));
    char big[SOLIDSYSLOG_MAX_MESSAGE_SIZE];
    memset(big, 'B', sizeof(big));
    LogIntoTheBufferCallCount = 0;
    SolidSyslog_SetErrorHandler(LogIntoTheBuffer, buffer);

    for (int i = 0; i < ATTEMPTS; i++)
    {
        Write(big, sizeof(big));
    }

    CHECK(LogIntoTheBufferCallCount > 1);
    CHECK(LogIntoTheBufferCallCount < ATTEMPTS);
}

/* An integrator's policy function that logs once, which lands in the same
 * buffer that asked. */
static struct SolidSyslogBuffer* policyLogTarget;

static enum SolidSyslogDiscardPolicy PolicyThatLogs(void* context)
{
    (void) context;
    struct SolidSyslogBuffer* target = policyLogTarget;
    policyLogTarget = nullptr;
    if (target != nullptr)
    {
        SolidSyslogBuffer_Write(target, "nested", 6);
    }
    return SOLIDSYSLOG_DISCARD_POLICY_NEWEST;
}

TEST(SolidSyslogCircularBufferMutex, APolicyFunctionThatLogsIsNotAskedUnderTheLock)
{
    SolidSyslogCircularBuffer_Destroy(buffer);
    buffer = SolidSyslogCircularBuffer_Create(MutexFake_Create(), ring, sizeof(ring), PolicyThatLogs, nullptr);
    policyLogTarget = buffer;

    Write("x");

    CHECK_TEXT(strstr(MutexFake_Sequence(), "LL") == nullptr, MutexFake_Sequence());
}

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
        testPolicy = SOLIDSYSLOG_DISCARD_POLICY_NEWEST;
        buffer = SolidSyslogCircularBuffer_Create(SolidSyslogNullMutex_Get(), ring, sizeof(ring), AnswersTestPolicy, nullptr);
    }

    void teardown() override
    {
        SolidSyslogCircularBuffer_Destroy(buffer);
    }
};

// clang-format on

// 32-byte ring, 2-byte header per record.
// recA(12)-> tail=14, recB(12)-> tail=28, read recA -> head=14.
// recC(12) recordBytes=14: doesn't fit at tail (28+14=42>32);
// canWrap: recordBytes(14) vs head(14). With `<=` bug: wraps, tail=14=head -> IsEmpty.
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
// write e of size 19 -> recordBytes=21 = head-tail. With `<=` bug, fitsAtTail in
// wrapped state lets tail advance to head exactly -> IsEmpty collapse.
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
    size_t got = 0U;
    CHECK_FALSE(SolidSyslogBuffer_Read(buffer, dest, 4, &got));
    LONGS_EQUAL(0, got);

    CHECK_TRUE(Read());
    CHECK_LAST_READ_RECORD("hello", 5);
}

// Exercises the ConsumeWrapMarker branch of Read: write A, B, drain A, write
// C, D (D forces a wrap), then drain in order. The fourth read must cross
// the wrap point - head reaches wrapPoint and jumps to 0 to read D.
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

// 32-byte ring: three 6-byte records take 24 bytes, and a 20-byte record needs
// 22, which only the room of all three gives.
TEST(SolidSyslogCircularBufferSmallRing, UnderOldestAsManyRecordsGoAsTheNewOneNeedsRoomFor)
{
    testPolicy = SOLIDSYSLOG_DISCARD_POLICY_OLDEST;
    char small[6];
    memset(small, 's', sizeof(small));
    char large[20];
    memset(large, 'L', sizeof(large));
    Write(small, sizeof(small));
    Write(small, sizeof(small));
    Write(small, sizeof(small));

    Write(large, sizeof(large));

    CHECK_TRUE(Read());
    CHECK_LAST_READ_RECORD(large, sizeof(large));
    CHECK_FALSE(Read());
}

// Evicting the head crosses the wrap marker as a Read does: after A, B, read A,
// C (wraps; tail 8, head 14, marker 28), read B (head 28), D fits at the tail
// (tail 22), E (8 bytes) does not - under Oldest it costs C, at the marker.
TEST(SolidSyslogCircularBufferSmallRing, UnderOldestEvictionCrossesTheWrapMarker)
{
    testPolicy = SOLIDSYSLOG_DISCARD_POLICY_OLDEST;
    char a[12];
    char b[12];
    char c[6];
    char d[12];
    char e[6];
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
    CHECK_LAST_READ_RECORD(d, sizeof(d));
    CHECK_TRUE(Read());
    CHECK_LAST_READ_RECORD(e, sizeof(e));
    CHECK_FALSE(Read());
}

TEST(SolidSyslogCircularBufferSmallRing, UnderOldestARecordNoEmptyRingCouldHoldIsRefusedWithoutEvicting)
{
    testPolicy = SOLIDSYSLOG_DISCARD_POLICY_OLDEST;
    char rec[12];
    memset(rec, 'r', sizeof(rec));
    char tooLarge[SMALL_RING_BYTES];
    memset(tooLarge, 'L', sizeof(tooLarge));
    Write(rec, sizeof(rec));

    Write(tooLarge, sizeof(tooLarge));

    CHECK_TRUE(Read());
    CHECK_LAST_READ_RECORD(rec, sizeof(rec));
    CHECK_FALSE(Read());
}

TEST(SolidSyslogCircularBufferSmallRing, ARecordNoEmptyRingCouldHoldDoesNotMakeTheRingFull)
{
    char tooLarge[SMALL_RING_BYTES];
    memset(tooLarge, 'L', sizeof(tooLarge));

    Write(tooLarge, sizeof(tooLarge));

    CHECK_FALSE(SolidSyslogBuffer_IsFull(buffer));
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
        testPolicy = SOLIDSYSLOG_DISCARD_POLICY_NEWEST;
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
    }

    struct SolidSyslogBuffer* MakeBuffer()
    {
        return SolidSyslogCircularBuffer_Create(mutex, ring, sizeof(ring), AnswersTestPolicy, nullptr);
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

    CHECK_ERROR_REPORTED_ONCE(
        SOLIDSYSLOG_SEVERITY_CRITICAL,
        &SolidSyslogCircularBufferErrorSource,
        SOLIDSYSLOG_CAT_POOL_EXHAUSTED,
        SOLIDSYSLOG_CIRCULAR_BUFFER_ERROR_POOL_EXHAUSTED
    );
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

    CHECK_ERROR_REPORTED_ONCE(
        SOLIDSYSLOG_SEVERITY_WARNING,
        &SolidSyslogCircularBufferErrorSource,
        SOLIDSYSLOG_CAT_UNKNOWN_DESTROY,
        SOLIDSYSLOG_CIRCULAR_BUFFER_ERROR_UNKNOWN_DESTROY
    );
}

TEST(SolidSyslogCircularBufferPool, DestroyOfStaleHandleReportsWarning)
{
    pooled[0] = MakeBuffer();
    SolidSyslogCircularBuffer_Destroy(pooled[0]);
    ErrorHandlerFake_Install(nullptr);

    SolidSyslogCircularBuffer_Destroy(pooled[0]);
    pooled[0] = nullptr;

    CHECK_ERROR_REPORTED_ONCE(
        SOLIDSYSLOG_SEVERITY_WARNING,
        &SolidSyslogCircularBufferErrorSource,
        SOLIDSYSLOG_CAT_UNKNOWN_DESTROY,
        SOLIDSYSLOG_CIRCULAR_BUFFER_ERROR_UNKNOWN_DESTROY
    );
}

TEST(SolidSyslogCircularBuffer, TheErrorSourceNamesItsDetailsThroughCircularBufferErrors)
{
    CHECK(SolidSyslogCircularBufferErrors_Name == SolidSyslogCircularBufferErrorSource.DetailName);
}
