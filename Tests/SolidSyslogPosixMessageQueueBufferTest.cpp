#include <cerrno>
#include <cstdlib>
#include <cstring>

#include "TestUtils.h"
#include "CppUTest/TestHarness.h"

using namespace CososoTesting;
#include "ConfigLockFake.h"
#include "ErrorHandlerFake.h"
#include "MqFake.h"
#include "MutexFake.h"
#include "SenderFake.h"
#include "SolidSyslog.h"
#include "SolidSyslogBuffer.h"
#include "SolidSyslogBufferCategories.h"
#include "SolidSyslogBufferDefinition.h"
#include "SolidSyslogConfig.h"
#include "SolidSyslogDiscardPolicy.h"
#include "SolidSyslogError.h"
#include "SolidSyslogErrorCategory.h"
#include "SolidSyslogNullMutex.h"
#include "SolidSyslogNullStore.h"
#include "SolidSyslogPosixMessageQueueBuffer.h"
#include "SolidSyslogPosixMessageQueueBufferErrors.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogTunables.h"

struct SolidSyslogStore;

static const char* const TEST_MESSAGE = "hello";
static const size_t TEST_MESSAGE_LEN = 5;
static const size_t TEST_MAX_MESSAGES = 10;

/* The policy every queue in this file is created with. Each group's setup
 * resets it to Newest, so the refusal tests mean what they say; a test that
 * wants another policy sets it before writing. */
static enum SolidSyslogDiscardPolicy testPolicy;

static enum SolidSyslogDiscardPolicy AnswersTestPolicy(void* context)
{
    (void) context;
    return testPolicy;
}

// clang-format off
TEST_GROUP(SolidSyslogPosixMessageQueueBuffer)
{
    struct SolidSyslogBuffer* buffer = nullptr;
    char   readData[SOLIDSYSLOG_MAX_MESSAGE_SIZE];
    size_t readSize;

    void setup() override
    {
        MqFake_Reset();
        testPolicy = SOLIDSYSLOG_DISCARD_POLICY_NEWEST;
        buffer = SolidSyslogPosixMessageQueueBuffer_Create(SolidSyslogNullMutex_Get(), SOLIDSYSLOG_MAX_MESSAGE_SIZE, TEST_MAX_MESSAGES, AnswersTestPolicy, nullptr);
        readSize = 0;
    }

    void teardown() override
    {
        SolidSyslog_SetErrorHandler(nullptr, nullptr);
        SolidSyslogPosixMessageQueueBuffer_Destroy(buffer);
    }

    void Write() const
    {
        SolidSyslogBuffer_Write(buffer, TEST_MESSAGE, TEST_MESSAGE_LEN);
    }

    void Write(const char* text) const
    {
        SolidSyslogBuffer_Write(buffer, text, strlen(text));
    }

    void FillTheQueue() const
    {
        for (size_t i = 0; i < TEST_MAX_MESSAGES; i++)
        {
            Write();
        }
    }

    bool Read()
    {
        return SolidSyslogBuffer_Read(buffer, readData, sizeof(readData), &readSize);
    }

    /* Drains the queue, answering the last record it held. */
    const char* LastRecordQueued()
    {
        readData[0] = '\0';
        while (Read())
        {
            readData[readSize] = '\0';
        }
        return readData;
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

TEST(SolidSyslogPosixMessageQueueBuffer, AFullQueueReportsTheRecordLost)
{
    ErrorHandlerFake_Install(nullptr);
    MqFake_FailNextSend(EAGAIN);

    Write();

    CHECK_ERROR_REPORTED_ONCE(
        SOLIDSYSLOG_SEVERITY_WARNING,
        &SolidSyslogPosixMessageQueueBufferErrorSource,
        SOLIDSYSLOG_CAT_BUFFER_RECORDS_LOST,
        SOLIDSYSLOG_POSIX_MESSAGE_QUEUE_BUFFER_ERROR_RECORD_REFUSED
    );
}

TEST(SolidSyslogPosixMessageQueueBuffer, ARunOfRefusedRecordsIsReportedOnce)
{
    ErrorHandlerFake_Install(nullptr);
    MqFake_FailNextSend(EAGAIN);
    Write();
    MqFake_FailNextSend(EAGAIN);

    Write();

    CALLED_FAKE(ErrorHandlerFake_Handle, ONCE);
}

TEST(SolidSyslogPosixMessageQueueBuffer, ARecordTheQueueTakesEndsTheRunSoTheNextRefusalIsReportedAgain)
{
    ErrorHandlerFake_Install(nullptr);
    MqFake_FailNextSend(EAGAIN);
    Write();
    Write(); /* taken */
    MqFake_FailNextSend(EAGAIN);

    Write();

    CALLED_FAKE(ErrorHandlerFake_Handle, TWICE);
}

TEST(SolidSyslogPosixMessageQueueBuffer, ARunOfFailedSendsIsReportedOnce)
{
    ErrorHandlerFake_Install(nullptr);
    MqFake_FailNextSend(EBADF);
    Write();
    MqFake_FailNextSend(EBADF);

    Write();

    CALLED_FAKE(ErrorHandlerFake_Handle, ONCE);
}

TEST(SolidSyslogPosixMessageQueueBuffer, AnEmptyQueueIsNotFull)
{
    CHECK_FALSE(SolidSyslogBuffer_IsFull(buffer));
}

TEST(SolidSyslogPosixMessageQueueBuffer, AQueueThatRefusedTheLastRecordIsFull)
{
    MqFake_FailNextSend(EAGAIN);

    Write();

    CHECK_TRUE(SolidSyslogBuffer_IsFull(buffer));
}

TEST(SolidSyslogPosixMessageQueueBuffer, AQueueThatFailedToSendIsNotFull)
{
    MqFake_FailNextSend(EBADF);

    Write();

    CHECK_FALSE(SolidSyslogBuffer_IsFull(buffer));
}

TEST(SolidSyslogPosixMessageQueueBuffer, AReadThatTakesARecordClearsFull)
{
    Write();
    MqFake_FailNextSend(EAGAIN);
    Write();

    Read();

    CHECK_FALSE(SolidSyslogBuffer_IsFull(buffer));
}

TEST(SolidSyslogPosixMessageQueueBuffer, AWriteTheQueueTakesClearsFull)
{
    MqFake_FailNextSend(EAGAIN);
    Write();

    Write();

    CHECK_FALSE(SolidSyslogBuffer_IsFull(buffer));
}

TEST(SolidSyslogPosixMessageQueueBuffer, UnderOldestAFullQueueDropsItsOldestRecordToMakeRoom)
{
    testPolicy = SOLIDSYSLOG_DISCARD_POLICY_OLDEST;
    FillTheQueue();

    Write("newest");

    STRCMP_EQUAL("newest", LastRecordQueued());
}

TEST(SolidSyslogPosixMessageQueueBuffer, UnderOldestOneRecordGoesForOneWritten)
{
    testPolicy = SOLIDSYSLOG_DISCARD_POLICY_OLDEST;
    FillTheQueue();
    Write("newest");

    size_t queued = 0;
    while (Read())
    {
        queued++;
    }

    LONGS_EQUAL(TEST_MAX_MESSAGES, queued);
}

TEST(SolidSyslogPosixMessageQueueBuffer, UnderOldestTheSendIsRetriedOnceAfterEvicting)
{
    testPolicy = SOLIDSYSLOG_DISCARD_POLICY_OLDEST;
    FillTheQueue();

    Write("newest");

    LONGS_EQUAL(TEST_MAX_MESSAGES + 2, MqFake_SendCallCount());
    LONGS_EQUAL(1, MqFake_ReceiveCallCount());
}

TEST(SolidSyslogPosixMessageQueueBuffer, UnderOldestAQueueThatMadeRoomIsFull)
{
    testPolicy = SOLIDSYSLOG_DISCARD_POLICY_OLDEST;
    FillTheQueue();

    Write("newest");

    CHECK_TRUE(SolidSyslogBuffer_IsFull(buffer));
}

TEST(SolidSyslogPosixMessageQueueBuffer, UnderOldestEvictingIsReportedAsRecordsLost)
{
    testPolicy = SOLIDSYSLOG_DISCARD_POLICY_OLDEST;
    FillTheQueue();
    ErrorHandlerFake_Install(nullptr);

    Write("newest");

    CHECK_ERROR_REPORTED_ONCE(
        SOLIDSYSLOG_SEVERITY_WARNING,
        &SolidSyslogPosixMessageQueueBufferErrorSource,
        SOLIDSYSLOG_CAT_BUFFER_RECORDS_LOST,
        SOLIDSYSLOG_POSIX_MESSAGE_QUEUE_BUFFER_ERROR_RECORDS_EVICTED
    );
}

TEST(SolidSyslogPosixMessageQueueBuffer, ARunOfEvictionsIsReportedOnce)
{
    testPolicy = SOLIDSYSLOG_DISCARD_POLICY_OLDEST;
    FillTheQueue();
    ErrorHandlerFake_Install(nullptr);

    Write("newest");
    Write("newer still");

    CALLED_FAKE(ErrorHandlerFake_Handle, ONCE);
}

TEST(SolidSyslogPosixMessageQueueBuffer, ASendThatEvictsNothingEndsTheRunSoTheNextEvictionIsReportedAgain)
{
    testPolicy = SOLIDSYSLOG_DISCARD_POLICY_OLDEST;
    FillTheQueue();
    ErrorHandlerFake_Install(nullptr);
    Write("newest"); /* evicts */
    Read();
    Write("fits");

    Write("evicts again");

    CALLED_FAKE(ErrorHandlerFake_Handle, TWICE);
}

/* The one send of each run that is reported says why the record went: the
 * refusal says the policy refused it, so a run of evictions reports as such. */
TEST(SolidSyslogPosixMessageQueueBuffer, UnderOldestAQueueWithNothingToEvictRefusesTheRecord)
{
    testPolicy = SOLIDSYSLOG_DISCARD_POLICY_OLDEST;
    ErrorHandlerFake_Install(nullptr);
    MqFake_FailNextSend(EAGAIN);

    Write();

    CHECK_ERROR_REPORTED_ONCE(
        SOLIDSYSLOG_SEVERITY_WARNING,
        &SolidSyslogPosixMessageQueueBufferErrorSource,
        SOLIDSYSLOG_CAT_BUFFER_RECORDS_LOST,
        SOLIDSYSLOG_POSIX_MESSAGE_QUEUE_BUFFER_ERROR_RECORD_REFUSED
    );
    CHECK_FALSE(Read());
}

/* The eviction is a loss of its own, reported whatever becomes of the send
 * it made room for. */
TEST(SolidSyslogPosixMessageQueueBuffer, UnderOldestAnEvictionIsReportedEvenWhenTheRetriedSendFails)
{
    testPolicy = SOLIDSYSLOG_DISCARD_POLICY_OLDEST;
    FillTheQueue();
    ErrorHandlerFake_Install(nullptr);
    MqFake_FailSecondSend(EBADF);

    Write("newest");

    CALLED_FAKE(ErrorHandlerFake_Handle, TWICE);
    CHECK_ERROR_EVENT_AT(
        0,
        SOLIDSYSLOG_SEVERITY_WARNING,
        &SolidSyslogPosixMessageQueueBufferErrorSource,
        SOLIDSYSLOG_CAT_BUFFER_RECORDS_LOST,
        SOLIDSYSLOG_POSIX_MESSAGE_QUEUE_BUFFER_ERROR_RECORDS_EVICTED
    );
    CHECK_ERROR_EVENT_AT(
        1,
        SOLIDSYSLOG_SEVERITY_ERROR,
        &SolidSyslogPosixMessageQueueBufferErrorSource,
        SOLIDSYSLOG_CAT_BUFFER_BACKEND_FAILED,
        SOLIDSYSLOG_POSIX_MESSAGE_QUEUE_BUFFER_ERROR_SEND_FAILED
    );
}

TEST(SolidSyslogPosixMessageQueueBuffer, UnderOldestAQueueThatMadeRoomIsFullEvenWhenTheRetriedSendFails)
{
    testPolicy = SOLIDSYSLOG_DISCARD_POLICY_OLDEST;
    FillTheQueue();
    MqFake_FailSecondSend(EBADF);

    Write("newest");

    CHECK_TRUE(SolidSyslogBuffer_IsFull(buffer));
}

/* Another task filled the room between the eviction and the send: both losses
 * are reported. */
TEST(SolidSyslogPosixMessageQueueBuffer, UnderOldestARecordRefusedAfterEvictingReportsBothLosses)
{
    testPolicy = SOLIDSYSLOG_DISCARD_POLICY_OLDEST;
    FillTheQueue();
    ErrorHandlerFake_Install(nullptr);
    MqFake_FailSecondSend(EAGAIN);

    Write("newest");

    CALLED_FAKE(ErrorHandlerFake_Handle, TWICE);
    CHECK_ERROR_EVENT(
        SOLIDSYSLOG_SEVERITY_WARNING,
        &SolidSyslogPosixMessageQueueBufferErrorSource,
        SOLIDSYSLOG_CAT_BUFFER_RECORDS_LOST,
        SOLIDSYSLOG_POSIX_MESSAGE_QUEUE_BUFFER_ERROR_RECORD_REFUSED
    );
    CHECK_TRUE(SolidSyslogBuffer_IsFull(buffer));
}

TEST(SolidSyslogPosixMessageQueueBuffer, UnderHaltAFullQueueRefusesTheRecordAsUnderNewest)
{
    testPolicy = SOLIDSYSLOG_DISCARD_POLICY_HALT;
    FillTheQueue();

    Write("newest");

    STRCMP_EQUAL(TEST_MESSAGE, LastRecordQueued());
}

TEST(SolidSyslogPosixMessageQueueBuffer, NoPolicyInjectedAppliesOldest)
{
    SolidSyslogPosixMessageQueueBuffer_Destroy(buffer);
    buffer = SolidSyslogPosixMessageQueueBuffer_Create(
        SolidSyslogNullMutex_Get(),
        SOLIDSYSLOG_MAX_MESSAGE_SIZE,
        TEST_MAX_MESSAGES,
        nullptr,
        nullptr
    );
    FillTheQueue();

    Write("newest");

    STRCMP_EQUAL("newest", LastRecordQueued());
}

/* A queue that fills and then breaks must still say it broke: the failure is
 * an ERROR a handler may be filtering for. */
TEST(SolidSyslogPosixMessageQueueBuffer, AFailedSendDuringARunOfRefusalsIsReportedToo)
{
    ErrorHandlerFake_Install(nullptr);
    MqFake_FailNextSend(EAGAIN);
    Write();
    MqFake_FailNextSend(EBADF);

    Write();

    CALLED_FAKE(ErrorHandlerFake_Handle, TWICE);
    CHECK_ERROR_EVENT(
        SOLIDSYSLOG_SEVERITY_ERROR,
        &SolidSyslogPosixMessageQueueBufferErrorSource,
        SOLIDSYSLOG_CAT_BUFFER_BACKEND_FAILED,
        SOLIDSYSLOG_POSIX_MESSAGE_QUEUE_BUFFER_ERROR_SEND_FAILED
    );
}

/* Only a full queue is a record lost to capacity; any other refusal is the
 * queue failing. */
TEST(SolidSyslogPosixMessageQueueBuffer, WriteWithOversizedMessageReportsError)
{
    ErrorHandlerFake_Install(nullptr);
    MqFake_FailNextSend(EMSGSIZE);

    Write();

    CHECK_ERROR_REPORTED_ONCE(
        SOLIDSYSLOG_SEVERITY_ERROR,
        &SolidSyslogPosixMessageQueueBufferErrorSource,
        SOLIDSYSLOG_CAT_BUFFER_BACKEND_FAILED,
        SOLIDSYSLOG_POSIX_MESSAGE_QUEUE_BUFFER_ERROR_SEND_FAILED
    );
}

TEST(SolidSyslogPosixMessageQueueBuffer, ReadWhenMqReceiveFailsReportsError)
{
    ErrorHandlerFake_Install(nullptr);
    MqFake_FailNextReceive(EMSGSIZE);

    Read();

    CHECK_ERROR_REPORTED_ONCE(
        SOLIDSYSLOG_SEVERITY_ERROR,
        &SolidSyslogPosixMessageQueueBufferErrorSource,
        SOLIDSYSLOG_CAT_BUFFER_BACKEND_FAILED,
        SOLIDSYSLOG_POSIX_MESSAGE_QUEUE_BUFFER_ERROR_RECEIVE_FAILED
    );
}

/* Regression guard: empty queue (EAGAIN) is part of the happy poll loop
 * and must NOT emit. The error handler must remain untouched. */
TEST(SolidSyslogPosixMessageQueueBuffer, ReadFromEmptyQueueDoesNotEmitError)
{
    ErrorHandlerFake_Install(nullptr);

    CHECK_FALSE(Read());

    CALLED_FAKE(ErrorHandlerFake_Handle, NEVER);
}

/* A NULL bytesRead* would crash on `*bytesRead = 0`. Guard at the
 * Read entry - invalid caller usage, not a runtime failure, so no
 * error code is emitted; just a defensive false return. */
TEST(SolidSyslogPosixMessageQueueBuffer, ReadWithNullBytesReadDoesNotCrash)
{
    char data[16] = {};
    CHECK_FALSE(SolidSyslogBuffer_Read(buffer, data, sizeof(data), nullptr));
}

/* After Destroy the slot's abstract-base vtable is the shared NullBuffer's, so
 * Write/Read through the stale handle is a safe no-op rather than a NULL-fn-pointer
 * crash. NullBuffer.Write swallows; NullBuffer.Read returns false with bytesRead=0. */
TEST(SolidSyslogPosixMessageQueueBuffer, UseAfterDestroyIsCrashSafeViaNullBufferVtable)
{
    SolidSyslogPosixMessageQueueBuffer_Destroy(buffer);

    SolidSyslogBuffer_Write(buffer, "x", 1);
    char data[16] = {};
    size_t bytesRead = 99;
    CHECK_FALSE(SolidSyslogBuffer_Read(buffer, data, sizeof(data), &bytesRead));
    LONGS_EQUAL(0, bytesRead);

    buffer = SolidSyslogPosixMessageQueueBuffer_Create(
        SolidSyslogNullMutex_Get(),
        SOLIDSYSLOG_MAX_MESSAGE_SIZE,
        10,
        AnswersTestPolicy,
        nullptr
    ); // for teardown
}

TEST(SolidSyslogPosixMessageQueueBuffer, ServiceSendsMessageWrittenViaLog)
{
    struct SolidSyslogSender* fakeSender = SenderFake_Create();
    SolidSyslogStore* nullStore = SolidSyslogNullStore_Get();
    SolidSyslogConfig config = {
        buffer,
        fakeSender,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullStore,
        nullptr,
        0,
        nullptr,
        nullptr,
        nullptr,
        nullptr
    };
    struct SolidSyslog* solidSyslog = SolidSyslog_Create(&config);

    SolidSyslogMessage message = {SOLIDSYSLOG_FACILITY_LOCAL0, SOLIDSYSLOG_SEVERITY_INFORMATIONAL, nullptr, nullptr};
    SolidSyslog_Log(solidSyslog, &message);
    SolidSyslog_Service(solidSyslog);
    CALLED_FAKE_ON(SenderFake_Send, fakeSender, ONCE);

    SolidSyslog_Destroy(solidSyslog);
    SenderFake_Destroy(fakeSender);
}

#define CHECK_IS_FALLBACK(handle, pool)                                                \
    {                                                                                  \
        CHECK_TEXT((handle) != nullptr, "Fallback handle was nullptr");                \
        for (auto* slot : (pool))                                                      \
        {                                                                              \
            CHECK_TEXT(slot != nullptr, "pool slot was nullptr (FillPool failed?)");   \
            CHECK_TEXT((handle) != slot, "Fallback handle collided with a pool slot"); \
        }                                                                              \
    }

// clang-format off
TEST_GROUP(SolidSyslogPosixMessageQueueBufferMutex)
{
    struct SolidSyslogBuffer* buffer = nullptr;

    void setup() override
    {
        MqFake_Reset();
        testPolicy = SOLIDSYSLOG_DISCARD_POLICY_NEWEST;
        buffer = SolidSyslogPosixMessageQueueBuffer_Create(MutexFake_Create(), SOLIDSYSLOG_MAX_MESSAGE_SIZE, TEST_MAX_MESSAGES, AnswersTestPolicy, nullptr);
    }

    void teardown() override
    {
        SolidSyslog_SetErrorHandler(nullptr, nullptr);
        SolidSyslogPosixMessageQueueBuffer_Destroy(buffer);
        MutexFake_Destroy();
    }

    void Write() const
    {
        SolidSyslogBuffer_Write(buffer, TEST_MESSAGE, TEST_MESSAGE_LEN);
    }
};

// clang-format on

/* An integrator's policy function that logs once, which lands in the same
 * queue that asked. */
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

TEST(SolidSyslogPosixMessageQueueBufferMutex, APolicyFunctionThatLogsIsNotAskedUnderTheLock)
{
    SolidSyslogPosixMessageQueueBuffer_Destroy(buffer);
    buffer = SolidSyslogPosixMessageQueueBuffer_Create(
        MutexFake_Create(),
        SOLIDSYSLOG_MAX_MESSAGE_SIZE,
        TEST_MAX_MESSAGES,
        PolicyThatLogs,
        nullptr
    );
    policyLogTarget = buffer;

    Write();

    CHECK_TEXT(strstr(MutexFake_Sequence(), "LL") == nullptr, MutexFake_Sequence());
}

TEST(SolidSyslogPosixMessageQueueBufferMutex, WriteCallsLockThenUnlockOnce)
{
    Write();

    STRCMP_EQUAL("LU", MutexFake_Sequence());
}

static int LogIntoTheQueueCallCount;

/* An integrator's handler that logs the report, which lands in the same full
 * queue. */
static void LogIntoTheQueue(void* context, const struct SolidSyslogErrorEvent* event)
{
    (void) event;
    LogIntoTheQueueCallCount++;
    SolidSyslogBuffer_Write(static_cast<struct SolidSyslogBuffer*>(context), "nested", 6);
}

TEST(SolidSyslogPosixMessageQueueBufferMutex, AHandlerThatLogsTheLossIntoTheFullQueueIsCalledOnceAndNotUnderTheLock)
{
    for (size_t i = 0; i < TEST_MAX_MESSAGES; i++)
    {
        Write();
    }
    LogIntoTheQueueCallCount = 0;
    SolidSyslog_SetErrorHandler(LogIntoTheQueue, buffer);

    Write();

    CALLED_FUNCTION(LogIntoTheQueue, ONCE);
    CHECK_TEXT(strstr(MutexFake_Sequence(), "LL") == nullptr, MutexFake_Sequence());
}

// clang-format off
TEST_GROUP(SolidSyslogPosixMessageQueueBufferPool)
{
    struct SolidSyslogBuffer* pooled[SOLIDSYSLOG_POSIX_MESSAGE_QUEUE_BUFFER_POOL_SIZE] = {};
    struct SolidSyslogBuffer* overflow                                                 = nullptr;

    void setup() override
    {
        MqFake_Reset();
    }

    void teardown() override
    {
        for (auto* handle : pooled)
        {
            if (handle != nullptr)
            {
                SolidSyslogPosixMessageQueueBuffer_Destroy(handle);
            }
        }
        if (overflow != nullptr)
        {
            SolidSyslogPosixMessageQueueBuffer_Destroy(overflow);
        }
        ConfigLockFake_Uninstall();
    }

    static struct SolidSyslogBuffer* MakeBuffer()
    {
        return SolidSyslogPosixMessageQueueBuffer_Create(SolidSyslogNullMutex_Get(), SOLIDSYSLOG_MAX_MESSAGE_SIZE, 10, nullptr, nullptr);
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

    CHECK_ERROR_REPORTED_ONCE(
        SOLIDSYSLOG_SEVERITY_CRITICAL,
        &SolidSyslogPosixMessageQueueBufferErrorSource,
        SOLIDSYSLOG_CAT_POOL_EXHAUSTED,
        SOLIDSYSLOG_POSIX_MESSAGE_QUEUE_BUFFER_ERROR_POOL_EXHAUSTED
    );
}

TEST(SolidSyslogPosixMessageQueueBufferPool, CreateOnMqOpenFailureReportsError)
{
    ErrorHandlerFake_Install(nullptr);
    MqFake_FailNextOpen(EINVAL);

    overflow = MakeBuffer();

    CHECK_ERROR_REPORTED_ONCE(
        SOLIDSYSLOG_SEVERITY_ERROR,
        &SolidSyslogPosixMessageQueueBufferErrorSource,
        SOLIDSYSLOG_CAT_BUFFER_BACKEND_FAILED,
        SOLIDSYSLOG_POSIX_MESSAGE_QUEUE_BUFFER_ERROR_MQ_OPEN_FAILED
    );
}

TEST(SolidSyslogPosixMessageQueueBufferPool, CreateOnMqOpenFailureReleasesSlot)
{
    MqFake_FailNextOpen(EINVAL);

    overflow = MakeBuffer();

    // Fill the pool *after* the failed Create; if the failed Create had leaked
    // its acquired slot, the pool would overflow into the fallback one slot
    // sooner - and FillPool's last MakeBuffer would return the same NullBuffer
    // singleton as `overflow`, since both Creates would have run out of slots.
    FillPool();
    for (auto* slot : pooled)
    {
        CHECK_TEXT(slot != overflow, "Pool slot collided with the failed-Create fallback handle");
    }
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

    CHECK_ERROR_REPORTED_ONCE(
        SOLIDSYSLOG_SEVERITY_WARNING,
        &SolidSyslogPosixMessageQueueBufferErrorSource,
        SOLIDSYSLOG_CAT_UNKNOWN_DESTROY,
        SOLIDSYSLOG_POSIX_MESSAGE_QUEUE_BUFFER_ERROR_UNKNOWN_DESTROY
    );
}

/* Destroy(NULL) is reachable from any integrator who keeps a NullBuffer-fallback
 * handle and later releases it. The IndexFromHandle search returns POOL_SIZE
 * (no slot matches NULL), IndexIsValid returns false, so the FreeIfInUse branch
 * is skipped - caller sees an UNKNOWN_DESTROY warning, no crash. */
TEST(SolidSyslogPosixMessageQueueBufferPool, DestroyOfNullHandleReportsWarningWithoutCrashing)
{
    ErrorHandlerFake_Install(nullptr);

    SolidSyslogPosixMessageQueueBuffer_Destroy(nullptr);

    CHECK_ERROR_REPORTED_ONCE(
        SOLIDSYSLOG_SEVERITY_WARNING,
        &SolidSyslogPosixMessageQueueBufferErrorSource,
        SOLIDSYSLOG_CAT_UNKNOWN_DESTROY,
        SOLIDSYSLOG_POSIX_MESSAGE_QUEUE_BUFFER_ERROR_UNKNOWN_DESTROY
    );
}

TEST(SolidSyslogPosixMessageQueueBufferPool, DestroyOfStaleHandleReportsWarning)
{
    pooled[0] = MakeBuffer();
    SolidSyslogPosixMessageQueueBuffer_Destroy(pooled[0]);
    ErrorHandlerFake_Install(nullptr);

    SolidSyslogPosixMessageQueueBuffer_Destroy(pooled[0]);
    pooled[0] = nullptr;

    CHECK_ERROR_REPORTED_ONCE(
        SOLIDSYSLOG_SEVERITY_WARNING,
        &SolidSyslogPosixMessageQueueBufferErrorSource,
        SOLIDSYSLOG_CAT_UNKNOWN_DESTROY,
        SOLIDSYSLOG_POSIX_MESSAGE_QUEUE_BUFFER_ERROR_UNKNOWN_DESTROY
    );
}

/* Slot-indexed queue names are only observable with at least two pool
 * slots. The default build runs at pool size 1; the
 * `tunable-override-debug` preset bumps it to 2 (see
 * Tests/Fixtures/TunableOverrides.h). When the runtime pool
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
