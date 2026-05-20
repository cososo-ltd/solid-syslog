#include <cstdlib>

#include "TestUtils.h"
#include "CppUTest/TestHarness.h"

using namespace CososoTesting; // NOLINT(google-build-using-namespace) -- test-file scope only; brings NEVER/ONCE/TWICE/THRICE into scope for the CALLED_*
    // macros
#include "ConfigLockFake.h"
#include "ErrorHandlerFake.h"
#include "SolidSyslogBuffer.h"
#include "SolidSyslogBufferDefinition.h"
#include "SolidSyslogErrorMessages.h"
#include "SolidSyslogPosixMessageQueueBuffer.h"
#include "SolidSyslog.h"
#include "SolidSyslogConfig.h"
#include "SolidSyslogTunables.h"
#include "SolidSyslogNullStore.h"
#include "SenderFake.h"
#include "SolidSyslogPrival.h"

struct SolidSyslogStore;

static const char* const TEST_MESSAGE = "hello";
static const size_t TEST_MESSAGE_LEN = 5;

// clang-format off
TEST_GROUP(SolidSyslogPosixMessageQueueBuffer)
{
    struct SolidSyslogBuffer* buffer = nullptr;
    char   readData[SOLIDSYSLOG_MAX_MESSAGE_SIZE];
    // cppcheck-suppress variableScope -- member of TEST_GROUP; scope managed by CppUTest macro
    size_t readSize;

    void setup() override
    {
        // cppcheck-suppress unreadVariable -- used across TEST_GROUP methods; cppcheck does not model CppUTest macros
        buffer = SolidSyslogPosixMessageQueueBuffer_Create(SOLIDSYSLOG_MAX_MESSAGE_SIZE, 10);
        readSize = 0;
    }

    void teardown() override
    {
        SolidSyslogPosixMessageQueueBuffer_Destroy(buffer);
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

TEST(SolidSyslogPosixMessageQueueBuffer, CreateDestroyWorksWithoutCrashing)
{
}

TEST(SolidSyslogPosixMessageQueueBuffer, WriteAndReadReturnsTrue)
{
    Write();
    CHECK_TRUE(Read());
}

TEST(SolidSyslogPosixMessageQueueBuffer, ReadReturnsWrittenData)
{
    Write();
    Read();
    MEMCMP_EQUAL(TEST_MESSAGE, readData, TEST_MESSAGE_LEN);
}

TEST(SolidSyslogPosixMessageQueueBuffer, ReadReturnsWrittenSize)
{
    Write();
    Read();
    LONGS_EQUAL(TEST_MESSAGE_LEN, readSize);
}

TEST(SolidSyslogPosixMessageQueueBuffer, ReadFromEmptyReturnsFalse)
{
    CHECK_FALSE(Read());
}

TEST(SolidSyslogPosixMessageQueueBuffer, MultipleWritesReadInOrder)
{
    SolidSyslogBuffer_Write(buffer, "first", 5);
    SolidSyslogBuffer_Write(buffer, "second", 6);
    Read();
    MEMCMP_EQUAL("first", readData, 5);
    Read();
    MEMCMP_EQUAL("second", readData, 6);
}

TEST(SolidSyslogPosixMessageQueueBuffer, SecondReadAfterSingleWriteReturnsFalse)
{
    Write();
    Read();
    CHECK_FALSE(Read());
}

TEST(SolidSyslogPosixMessageQueueBuffer, ServiceSendsMessageWrittenViaLog)
{
    struct SolidSyslogSender* fakeSender = SenderFake_Create();
    SolidSyslogStore* nullStore = SolidSyslogNullStore_Get();
    SolidSyslogConfig config = {buffer, fakeSender, nullptr, nullptr, nullptr, nullptr, nullStore, nullptr, 0};
    struct SolidSyslog* solidSyslog = SolidSyslog_Create(&config);

    SolidSyslogMessage message = {SOLIDSYSLOG_FACILITY_LOCAL0, SOLIDSYSLOG_SEVERITY_INFORMATIONAL, nullptr, nullptr};
    SolidSyslog_Log(solidSyslog, &message);
    SolidSyslog_Service(solidSyslog);
    CALLED_FAKE_ON(SenderFake_Send, fakeSender, ONCE);

    SolidSyslog_Destroy(solidSyslog);
    SenderFake_Destroy(fakeSender);
}

IGNORE_TEST(SolidSyslogPosixMessageQueueBuffer, HappyPathOnly)

{
    // Error handling not yet implemented — see Epic #31
    //   Create with zero maxMessageSize or maxMessages
    //   Create when mq_open fails returns NULL
    //   Write with NULL buffer does not crash
    //   Write with NULL data does not crash
    //   Read with NULL buffer does not crash
    //   Read with NULL data does not crash
    //   Read with NULL bytesRead does not crash
    //   Destroy with NULL buffer does not crash
    //   Write when queue is full (back-pressure / overflow)
    //
    // Blocking mode not yet implemented — see S4.5 or later
    //   Read blocks waiting for a message (O_NONBLOCK removed)
}

// NOLINTBEGIN(cppcoreguidelines-macro-usage,cppcoreguidelines-avoid-do-while)
#define CHECK_IS_FALLBACK(handle, pool)                                                \
    do                                                                                 \
    {                                                                                  \
        CHECK_TEXT((handle) != nullptr, "Fallback handle was nullptr");                \
        for (auto* slot : (pool))                                                      \
        {                                                                              \
            CHECK_TEXT(slot != nullptr, "pool slot was nullptr (FillPool failed?)");   \
            CHECK_TEXT((handle) != slot, "Fallback handle collided with a pool slot"); \
        }                                                                              \
    } while (0)

// NOLINTEND(cppcoreguidelines-macro-usage,cppcoreguidelines-avoid-do-while)

// clang-format off
TEST_GROUP(SolidSyslogPosixMessageQueueBufferPool)
{
    // cppcheck-suppress constVariable -- assigned in test bodies; cppcheck does not model CppUTest lifecycle
    struct SolidSyslogBuffer* pooled[SOLIDSYSLOG_POSIX_MESSAGE_QUEUE_BUFFER_POOL_SIZE] = {};
    struct SolidSyslogBuffer* overflow                                                 = nullptr;

    void teardown() override
    {
        for (auto* handle : pooled)
        {
            if (handle != nullptr)
            {
                SolidSyslogPosixMessageQueueBuffer_Destroy(handle);
            }
        }
        // cppcheck-suppress knownConditionTrueFalse -- assigned in test bodies; cppcheck does not model CppUTest lifecycle
        if (overflow != nullptr)
        {
            SolidSyslogPosixMessageQueueBuffer_Destroy(overflow);
        }
        ConfigLockFake_Uninstall();
        ErrorHandlerFake_Uninstall();
    }

    static struct SolidSyslogBuffer* MakeBuffer()
    {
        return SolidSyslogPosixMessageQueueBuffer_Create(SOLIDSYSLOG_MAX_MESSAGE_SIZE, 10);
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

TEST(SolidSyslogPosixMessageQueueBufferPool, FillingPoolThenOverflowReturnsDistinctFallback)
{
    FillPool();

    overflow = MakeBuffer();

    CHECK_IS_FALLBACK(overflow, pooled);
}

TEST(SolidSyslogPosixMessageQueueBufferPool, ExhaustedCreateReportsError)
{
    ErrorHandlerFake_Install(nullptr);
    FillPool();

    overflow = MakeBuffer();

    CALLED_FAKE(ErrorHandlerFake_Handle, ONCE);
    LONGS_EQUAL(SOLIDSYSLOG_SEVERITY_ERROR, ErrorHandlerFake_LastSeverity());
    STRCMP_EQUAL(SOLIDSYSLOG_ERROR_MSG_POSIXMESSAGEQUEUEBUFFER_POOL_EXHAUSTED, ErrorHandlerFake_LastMessage());
}

TEST(SolidSyslogPosixMessageQueueBufferPool, FallbackReadAndWriteAreNoOps)
{
    FillPool();
    overflow = MakeBuffer();

    SolidSyslogBuffer_Write(overflow, "hello", 5);

    char readBuffer[16] = {};
    size_t bytesRead = 99;
    CHECK_FALSE(SolidSyslogBuffer_Read(overflow, readBuffer, sizeof(readBuffer), &bytesRead));
    LONGS_EQUAL(0, bytesRead);
}

TEST(SolidSyslogPosixMessageQueueBufferPool, CreateAcquiresAndReleasesConfigLockOnFirstFreeSlot)
{
    ConfigLockFake_Install();

    pooled[0] = MakeBuffer();

    CALLED_FAKE(ConfigLockFake_Lock, ONCE);
    CALLED_FAKE(ConfigLockFake_Unlock, ONCE);
}

TEST(SolidSyslogPosixMessageQueueBufferPool, CreateLocksOncePerSlotProbedWhenPoolIsFull)
{
    FillPool();
    ConfigLockFake_Install();

    overflow = MakeBuffer();

    LONGS_EQUAL(SOLIDSYSLOG_POSIX_MESSAGE_QUEUE_BUFFER_POOL_SIZE, ConfigLockFake_LockCallCount());
    LONGS_EQUAL(SOLIDSYSLOG_POSIX_MESSAGE_QUEUE_BUFFER_POOL_SIZE, ConfigLockFake_UnlockCallCount());
}

TEST(SolidSyslogPosixMessageQueueBufferPool, DestroyOfPooledHandleLocksOnce)
{
    pooled[0] = MakeBuffer();
    ConfigLockFake_Install();

    SolidSyslogPosixMessageQueueBuffer_Destroy(pooled[0]);
    pooled[0] = nullptr;

    CALLED_FAKE(ConfigLockFake_Lock, ONCE);
    CALLED_FAKE(ConfigLockFake_Unlock, ONCE);
}

TEST(SolidSyslogPosixMessageQueueBufferPool, DestroyOfUnknownHandleDoesNotLock)
{
    ConfigLockFake_Install();
    struct SolidSyslogBuffer stranger = {};

    SolidSyslogPosixMessageQueueBuffer_Destroy(&stranger);

    CALLED_FAKE(ConfigLockFake_Lock, NEVER);
    CALLED_FAKE(ConfigLockFake_Unlock, NEVER);
}

TEST(SolidSyslogPosixMessageQueueBufferPool, DestroyOfUnknownHandleReportsWarning)
{
    ErrorHandlerFake_Install(nullptr);
    struct SolidSyslogBuffer stranger = {};

    SolidSyslogPosixMessageQueueBuffer_Destroy(&stranger);

    CALLED_FAKE(ErrorHandlerFake_Handle, ONCE);
    LONGS_EQUAL(SOLIDSYSLOG_SEVERITY_WARNING, ErrorHandlerFake_LastSeverity());
    STRCMP_EQUAL(SOLIDSYSLOG_ERROR_MSG_POSIXMESSAGEQUEUEBUFFER_UNKNOWN_DESTROY, ErrorHandlerFake_LastMessage());
}

TEST(SolidSyslogPosixMessageQueueBufferPool, DestroyOfStaleHandleReportsWarning)
{
    pooled[0] = MakeBuffer();
    SolidSyslogPosixMessageQueueBuffer_Destroy(pooled[0]);
    ErrorHandlerFake_Install(nullptr);

    SolidSyslogPosixMessageQueueBuffer_Destroy(pooled[0]);
    pooled[0] = nullptr;

    CALLED_FAKE(ErrorHandlerFake_Handle, ONCE);
    LONGS_EQUAL(SOLIDSYSLOG_SEVERITY_WARNING, ErrorHandlerFake_LastSeverity());
    STRCMP_EQUAL(SOLIDSYSLOG_ERROR_MSG_POSIXMESSAGEQUEUEBUFFER_UNKNOWN_DESTROY, ErrorHandlerFake_LastMessage());
}

/* Slot-indexed queue names are only observable with at least two pool
 * slots. The default build runs at pool size 1; the
 * `tunable-override-debug` preset bumps it to 2 (see
 * Tests/Fixtures/SmallMessageSizeTunables.h). When the runtime pool
 * size can't host a second slot, print a notice and exit cleanly via
 * TEST_EXIT so the test is honestly accounted for in the run rather
 * than compiled out. Local pointers (not fixture's `pooled[]`) so the
 * second handle has compile-time storage even at pool size 1. */
TEST(SolidSyslogPosixMessageQueueBufferPool, EachPooledHandleHasIsolatedQueue)
{
    if (SOLIDSYSLOG_POSIX_MESSAGE_QUEUE_BUFFER_POOL_SIZE < 2U)
    {
        UT_PRINT("Pool size < 2 — slot isolation only observable under tunable-override-debug");
        TEST_EXIT;
    }

    struct SolidSyslogBuffer* slotZero = MakeBuffer();
    struct SolidSyslogBuffer* slotOne = MakeBuffer();

    SolidSyslogBuffer_Write(slotZero, "slot0", 5U);

    char readBuffer[SOLIDSYSLOG_MAX_MESSAGE_SIZE] = {};
    size_t bytesRead = 99U;
    bool readSucceeded = SolidSyslogBuffer_Read(slotOne, readBuffer, sizeof(readBuffer), &bytesRead);

    SolidSyslogPosixMessageQueueBuffer_Destroy(slotZero);
    SolidSyslogPosixMessageQueueBuffer_Destroy(slotOne);

    CHECK_FALSE(readSucceeded);
    LONGS_EQUAL(0, bytesRead);
}
