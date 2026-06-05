#include <stddef.h>

#include "TestUtils.h"
#include "CppUTest/TestHarness.h"

using namespace CososoTesting;
#include "ErrorHandlerFake.h"
#include "SenderFake.h"
#include "SolidSyslogBuffer.h"
#include "SolidSyslogErrorCategory.h"
#include "SolidSyslogPassthroughBuffer.h"
#include "SolidSyslogPassthroughBufferErrors.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogTunables.h"

static const char* const TEST_MESSAGE = "hello";
static const size_t TEST_MESSAGE_LEN = 5;

// clang-format off
TEST_GROUP(SolidSyslogPassthroughBuffer)
{
    struct SolidSyslogSender* fakeSender = nullptr;
    struct SolidSyslogBuffer* buffer     = nullptr;

    void setup() override
    {
        fakeSender = SenderFake_Create();
        buffer = SolidSyslogPassthroughBuffer_Create(fakeSender);
    }

    void teardown() override
    {
        SolidSyslogPassthroughBuffer_Destroy(buffer);
        SenderFake_Destroy(fakeSender);
    }

    void Write() const
    {
        SolidSyslogBuffer_Write(buffer, TEST_MESSAGE, TEST_MESSAGE_LEN);
    }
};

// clang-format on

TEST(SolidSyslogPassthroughBuffer, CreateDestroyWorksWithoutCrashing)
{
}

TEST(SolidSyslogPassthroughBuffer, WriteForwardsBufferToSender)
{
    Write();
    STRCMP_EQUAL(TEST_MESSAGE, SenderFake_LastBufferAsString(fakeSender));
}

TEST(SolidSyslogPassthroughBuffer, WriteForwardsSizeToSender)
{
    Write();
    LONGS_EQUAL(TEST_MESSAGE_LEN, SenderFake_LastSize(fakeSender));
}

TEST(SolidSyslogPassthroughBuffer, WriteResultsInOneSend)
{
    Write();
    CALLED_FAKE_ON(SenderFake_Send, fakeSender, ONCE);
}

TEST(SolidSyslogPassthroughBuffer, TwoWritesResultInTwoSends)
{
    Write();
    Write();
    CALLED_FAKE_ON(SenderFake_Send, fakeSender, TWICE);
}

TEST(SolidSyslogPassthroughBuffer, NoWritesResultInNoSends)
{
    CALLED_FAKE_ON(SenderFake_Send, fakeSender, NEVER);
}

TEST(SolidSyslogPassthroughBuffer, ReadReturnsNothingToSend)
{
    char data[512];
    size_t bytesRead = 0;
    bool sent = SolidSyslogBuffer_Read(buffer, data, sizeof(data), &bytesRead);
    CHECK_FALSE(sent);
}

TEST(SolidSyslogPassthroughBuffer, DestroyWithNullHandleEmitsUnknownDestroyWarning)
{
    ErrorHandlerFake_Install(nullptr);

    SolidSyslogPassthroughBuffer_Destroy(nullptr);

    CALLED_FAKE(ErrorHandlerFake_Handle, ONCE);
    LONGS_EQUAL(SOLIDSYSLOG_SEVERITY_WARNING, ErrorHandlerFake_LastSeverity());
    POINTERS_EQUAL(&PassthroughBufferErrorSource, ErrorHandlerFake_LastSource());
    UNSIGNED_LONGS_EQUAL(SOLIDSYSLOG_CAT_UNKNOWN_DESTROY, ErrorHandlerFake_LastCategory());
    UNSIGNED_LONGS_EQUAL(PASSTHROUGHBUFFER_ERROR_UNKNOWN_DESTROY, ErrorHandlerFake_LastDetail());
}

TEST(SolidSyslogPassthroughBuffer, UseAfterDestroyIsCrashSafeViaNullBufferVtable)
{
    /* After Destroy the slot's abstract-base vtable is the shared NullBuffer's, so
     * Write/Read through the stale handle is a safe no-op rather than a NULL-fn-pointer
     * crash. NullBuffer.Write swallows; NullBuffer.Read returns false with bytesRead=0. */
    SolidSyslogPassthroughBuffer_Destroy(buffer);

    SolidSyslogBuffer_Write(buffer, "x", 1);
    char data[16] = {};
    size_t bytesRead = 99;
    CHECK_FALSE(SolidSyslogBuffer_Read(buffer, data, sizeof(data), &bytesRead));
    LONGS_EQUAL(0, bytesRead);
    CALLED_FAKE_ON(SenderFake_Send, fakeSender, NEVER);

    buffer = SolidSyslogPassthroughBuffer_Create(fakeSender); // for teardown
}

// Pool tests — prove SOLIDSYSLOG_PASSTHROUGH_BUFFER_POOL_SIZE caps live
// instances and overflow falls back to a distinct no-op buffer. Generic
// pool mechanics (lock counts, per-probe locking, stale-handle warning)
// are covered by SolidSyslogPoolAllocatorTest.cpp.

// clang-format off
TEST_GROUP(SolidSyslogPassthroughBufferPool)
{
    struct SolidSyslogSender* fakeSender                                         = nullptr;
    struct SolidSyslogBuffer* pooled[SOLIDSYSLOG_PASSTHROUGH_BUFFER_POOL_SIZE]  = {};
    struct SolidSyslogBuffer* overflow                                           = nullptr;

    void setup() override
    {
        fakeSender = SenderFake_Create();
    }

    void teardown() override
    {
        for (auto* handle : pooled)
        {
            if (handle != nullptr)
            {
                SolidSyslogPassthroughBuffer_Destroy(handle);
            }
        }
        if (overflow != nullptr)
        {
            SolidSyslogPassthroughBuffer_Destroy(overflow);
        }
        SenderFake_Destroy(fakeSender);
    }

    [[nodiscard]] struct SolidSyslogBuffer* MakeBuffer() const
    {
        return SolidSyslogPassthroughBuffer_Create(fakeSender);
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

TEST(SolidSyslogPassthroughBufferPool, OverflowReportsPoolExhausted)
{
    FillPool();
    ErrorHandlerFake_Install(nullptr);

    overflow = MakeBuffer();

    CALLED_FAKE(ErrorHandlerFake_Handle, ONCE);
    LONGS_EQUAL(SOLIDSYSLOG_SEVERITY_CRITICAL, ErrorHandlerFake_LastSeverity());
    POINTERS_EQUAL(&PassthroughBufferErrorSource, ErrorHandlerFake_LastSource());
    UNSIGNED_LONGS_EQUAL(SOLIDSYSLOG_CAT_POOL_EXHAUSTED, ErrorHandlerFake_LastCategory());
    UNSIGNED_LONGS_EQUAL(PASSTHROUGHBUFFER_ERROR_POOL_EXHAUSTED, ErrorHandlerFake_LastDetail());
}

TEST(SolidSyslogPassthroughBufferPool, FillingPoolThenOverflowReturnsDistinctFallback)
{
    FillPool();

    overflow = MakeBuffer();

    CHECK_TEXT(overflow != nullptr, "Fallback handle was nullptr");
    for (auto* slot : pooled)
    {
        CHECK_TEXT(slot != nullptr, "pool slot was nullptr (FillPool failed?)");
        CHECK_TEXT(overflow != slot, "Fallback handle collided with a pool slot");
    }
}

TEST(SolidSyslogPassthroughBufferPool, FallbackWriteAndReadAreNoOps)
{
    FillPool();
    overflow = MakeBuffer();

    SolidSyslogBuffer_Write(overflow, "hello", 5);

    char readBuffer[16] = {};
    size_t bytesRead = 99;
    CHECK_FALSE(SolidSyslogBuffer_Read(overflow, readBuffer, sizeof(readBuffer), &bytesRead));
    LONGS_EQUAL(0, bytesRead);
    CALLED_FAKE_ON(SenderFake_Send, fakeSender, NEVER);
}

TEST(SolidSyslogPassthroughBufferPool, CreateWithNullSenderReportsError)
{
    ErrorHandlerFake_Install(nullptr);

    overflow = SolidSyslogPassthroughBuffer_Create(nullptr);

    CALLED_FAKE(ErrorHandlerFake_Handle, ONCE);
    LONGS_EQUAL(SOLIDSYSLOG_SEVERITY_CRITICAL, ErrorHandlerFake_LastSeverity());
    POINTERS_EQUAL(&PassthroughBufferErrorSource, ErrorHandlerFake_LastSource());
    UNSIGNED_LONGS_EQUAL(SOLIDSYSLOG_CAT_BAD_CONFIG, ErrorHandlerFake_LastCategory());
    UNSIGNED_LONGS_EQUAL(PASSTHROUGHBUFFER_ERROR_NULL_SENDER, ErrorHandlerFake_LastDetail());
}

TEST(SolidSyslogPassthroughBufferPool, CreateWithNullSenderReturnsFallbackDistinctFromAnyPoolSlot)
{
    FillPool();

    overflow = SolidSyslogPassthroughBuffer_Create(nullptr);

    CHECK_TEXT(overflow != nullptr, "Fallback handle was nullptr");
    for (auto* slot : pooled)
    {
        CHECK_TEXT(overflow != slot, "Fallback handle collided with a pool slot");
    }
}

TEST(SolidSyslogPassthroughBufferPool, CreateWithNullSenderDoesNotConsumeAPoolSlot)
{
    // If the failed Create had leaked its acquired slot, FillPool would overflow
    // into the fallback one slot sooner and one pool slot would collide with
    // `overflow` (both pointing at the NullBuffer singleton).
    overflow = SolidSyslogPassthroughBuffer_Create(nullptr);

    FillPool();
    for (auto* slot : pooled)
    {
        CHECK_TEXT(slot != overflow, "Pool slot collided with the NULL-sender fallback handle");
    }
}
