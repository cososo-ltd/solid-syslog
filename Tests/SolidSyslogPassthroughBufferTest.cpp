#include <stddef.h>

#include "TestUtils.h"
#include "CppUTest/TestHarness.h"

using namespace CososoTesting; // NOLINT(google-build-using-namespace) -- test-file scope only; brings NEVER/ONCE/TWICE/THRICE into scope for the CALLED_*
    // macros
#include "SolidSyslogBuffer.h"
#include "SolidSyslogPassthroughBuffer.h"
#include "SolidSyslogTunables.h"
#include "SenderFake.h"

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
        // cppcheck-suppress unreadVariable -- used across TEST_GROUP methods; cppcheck does not model CppUTest macros
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

IGNORE_TEST(SolidSyslogPassthroughBuffer, HappyPathOnly)

{
    // Error handling not yet implemented — see Epic #31
    //   Create with NULL sender returns NULL
    //   Write with NULL buffer does not crash
    //   Destroy with NULL buffer does not crash
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
        // cppcheck-suppress unreadVariable -- used across TEST_GROUP methods; cppcheck does not model CppUTest macros
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
