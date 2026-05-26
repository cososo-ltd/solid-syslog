#include <cerrno>
#include <cstdlib>

#include "TestUtils.h"
#include "CppUTest/TestHarness.h"

using namespace CososoTesting;
#include "ConfigLockFake.h"
#include "ErrorHandlerFake.h"
#include "MqFake.h"
#include "SolidSyslogBuffer.h"
#include "SolidSyslogBufferDefinition.h"
#include "SolidSyslogPosixMessageQueueBuffer.h"
#include "SolidSyslogPosixMessageQueueBufferErrors.h"
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
    size_t readSize;

    void setup() override
    {
        MqFake_Reset();
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

TEST(SolidSyslogPosixMessageQueueBuffer, WriteWhenMqSendFailsReportsError)
{
    ErrorHandlerFake_Install(nullptr);
    MqFake_FailNextSend(EAGAIN);

    Write();

    CALLED_FAKE(ErrorHandlerFake_Handle, ONCE);
    LONGS_EQUAL(SOLIDSYSLOG_SEVERITY_ERROR, ErrorHandlerFake_LastSeverity());
    POINTERS_EQUAL(&PosixMessageQueueBufferErrorSource, ErrorHandlerFake_LastSource());
    UNSIGNED_LONGS_EQUAL(POSIXMESSAGEQUEUEBUFFER_ERROR_SEND_FAILED, ErrorHandlerFake_LastCode());
}

/* Regression guard: the production check is errno-agnostic
 * (`if (mq_send(...) != 0)`), so EAGAIN and EMSGSIZE flow through
 * the same branch today. A future contributor who adds errno-specific
 * handling has to deliberately drop one of these cases. */
TEST(SolidSyslogPosixMessageQueueBuffer, WriteWithOversizedMessageReportsError)
{
    ErrorHandlerFake_Install(nullptr);
    MqFake_FailNextSend(EMSGSIZE);

    Write();

    CALLED_FAKE(ErrorHandlerFake_Handle, ONCE);
    UNSIGNED_LONGS_EQUAL(POSIXMESSAGEQUEUEBUFFER_ERROR_SEND_FAILED, ErrorHandlerFake_LastCode());
}

TEST(SolidSyslogPosixMessageQueueBuffer, ReadWhenMqReceiveFailsReportsError)
{
    ErrorHandlerFake_Install(nullptr);
    MqFake_FailNextReceive(EMSGSIZE);

    Read();

    CALLED_FAKE(ErrorHandlerFake_Handle, ONCE);
    LONGS_EQUAL(SOLIDSYSLOG_SEVERITY_ERROR, ErrorHandlerFake_LastSeverity());
    POINTERS_EQUAL(&PosixMessageQueueBufferErrorSource, ErrorHandlerFake_LastSource());
    UNSIGNED_LONGS_EQUAL(POSIXMESSAGEQUEUEBUFFER_ERROR_RECEIVE_FAILED, ErrorHandlerFake_LastCode());
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
 * Read entry — invalid caller usage, not a runtime failure, so no
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

    buffer = SolidSyslogPosixMessageQueueBuffer_Create(SOLIDSYSLOG_MAX_MESSAGE_SIZE, 10); // for teardown
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
    POINTERS_EQUAL(&PosixMessageQueueBufferErrorSource, ErrorHandlerFake_LastSource());
    UNSIGNED_LONGS_EQUAL(POSIXMESSAGEQUEUEBUFFER_ERROR_POOL_EXHAUSTED, ErrorHandlerFake_LastCode());
}

TEST(SolidSyslogPosixMessageQueueBufferPool, CreateOnMqOpenFailureReportsError)
{
    ErrorHandlerFake_Install(nullptr);
    MqFake_FailNextOpen(EINVAL);

    overflow = MakeBuffer();

    CALLED_FAKE(ErrorHandlerFake_Handle, ONCE);
    LONGS_EQUAL(SOLIDSYSLOG_SEVERITY_ERROR, ErrorHandlerFake_LastSeverity());
    POINTERS_EQUAL(&PosixMessageQueueBufferErrorSource, ErrorHandlerFake_LastSource());
    UNSIGNED_LONGS_EQUAL(POSIXMESSAGEQUEUEBUFFER_ERROR_MQ_OPEN_FAILED, ErrorHandlerFake_LastCode());
}

TEST(SolidSyslogPosixMessageQueueBufferPool, CreateOnMqOpenFailureReleasesSlot)
{
    MqFake_FailNextOpen(EINVAL);

    overflow = MakeBuffer();

    // Fill the pool *after* the failed Create; if the failed Create had leaked
    // its acquired slot, the pool would overflow into the fallback one slot
    // sooner — and FillPool's last MakeBuffer would return the same NullBuffer
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

    CALLED_FAKE(ErrorHandlerFake_Handle, ONCE);
    LONGS_EQUAL(SOLIDSYSLOG_SEVERITY_WARNING, ErrorHandlerFake_LastSeverity());
    POINTERS_EQUAL(&PosixMessageQueueBufferErrorSource, ErrorHandlerFake_LastSource());
    UNSIGNED_LONGS_EQUAL(POSIXMESSAGEQUEUEBUFFER_ERROR_UNKNOWN_DESTROY, ErrorHandlerFake_LastCode());
}

/* Destroy(NULL) is reachable from any integrator who keeps a NullBuffer-fallback
 * handle and later releases it. The IndexFromHandle search returns POOL_SIZE
 * (no slot matches NULL), IndexIsValid returns false, so the FreeIfInUse branch
 * is skipped — caller sees an UNKNOWN_DESTROY warning, no crash. */
TEST(SolidSyslogPosixMessageQueueBufferPool, DestroyOfNullHandleReportsWarningWithoutCrashing)
{
    ErrorHandlerFake_Install(nullptr);

    SolidSyslogPosixMessageQueueBuffer_Destroy(nullptr);

    CALLED_FAKE(ErrorHandlerFake_Handle, ONCE);
    LONGS_EQUAL(SOLIDSYSLOG_SEVERITY_WARNING, ErrorHandlerFake_LastSeverity());
    POINTERS_EQUAL(&PosixMessageQueueBufferErrorSource, ErrorHandlerFake_LastSource());
    UNSIGNED_LONGS_EQUAL(POSIXMESSAGEQUEUEBUFFER_ERROR_UNKNOWN_DESTROY, ErrorHandlerFake_LastCode());
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
    POINTERS_EQUAL(&PosixMessageQueueBufferErrorSource, ErrorHandlerFake_LastSource());
    UNSIGNED_LONGS_EQUAL(POSIXMESSAGEQUEUEBUFFER_ERROR_UNKNOWN_DESTROY, ErrorHandlerFake_LastCode());
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
