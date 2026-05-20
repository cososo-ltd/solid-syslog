#include "CppUTest/TestHarness.h"

#include "ConfigLockFake.h"
#include "ErrorHandlerFake.h"
#include "SenderFake.h"
#include "SolidSyslog.h"
#include "SolidSyslogConfig.h"
#include "SolidSyslogErrorMessages.h"
#include "SolidSyslogNullStore.h"
#include "SolidSyslogPassthroughBuffer.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogTunables.h"
#include "TestUtils.h"

using namespace CososoTesting; // NOLINT(google-build-using-namespace) -- test-file scope only; brings ONCE/NEVER into scope for CALLED_FAKE

// NOLINTBEGIN(cppcoreguidelines-macro-usage,cppcoreguidelines-avoid-do-while) -- macros preserve __FILE__/__LINE__ at the call site

// Asserts handle is non-null and not one of the slots in pool.
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
TEST_GROUP(SolidSyslogPool)
{
    struct SolidSyslogSender* fakeSender = nullptr;
    struct SolidSyslogBuffer* buffer = nullptr;
    SolidSyslogConfig config{};
    // cppcheck-suppress constVariable -- assigned in test bodies; cppcheck does not model CppUTest lifecycle
    struct SolidSyslog* pooled[SOLIDSYSLOG_POOL_SIZE] = {};
    struct SolidSyslog* overflow = nullptr;

    void setup() override
    {
        fakeSender = SenderFake_Create();
        buffer = SolidSyslogPassthroughBuffer_Create(fakeSender);
        config = {buffer, fakeSender, nullptr, nullptr, nullptr, nullptr,
                  SolidSyslogNullStore_Get(), nullptr, 0};
    }

    void teardown() override
    {
        for (auto* handle : pooled)
        {
            if (handle != nullptr)
            {
                SolidSyslog_Destroy(handle);
            }
        }
        // cppcheck-suppress knownConditionTrueFalse -- assigned in test bodies; cppcheck does not model CppUTest lifecycle
        if (overflow != nullptr)
        {
            SolidSyslog_Destroy(overflow);
        }
        SolidSyslogPassthroughBuffer_Destroy(buffer);
        SenderFake_Destroy(fakeSender);
        ConfigLockFake_Uninstall();
        ErrorHandlerFake_Uninstall();
    }

    void FillPool()
    {
        for (auto*& slot : pooled)
        {
            slot = SolidSyslog_Create(&config);
        }
    }
};

// clang-format on

TEST(SolidSyslogPool, FillingPoolThenOverflowReturnsDistinctFallback)
{
    FillPool();

    overflow = SolidSyslog_Create(&config);

    CHECK_IS_FALLBACK(overflow, pooled);
}

TEST(SolidSyslogPool, ExhaustedCreateReportsError)
{
    ErrorHandlerFake_Install(nullptr);
    FillPool();

    overflow = SolidSyslog_Create(&config);

    CALLED_FAKE(ErrorHandlerFake_Handle, ONCE);
    LONGS_EQUAL(SOLIDSYSLOG_SEVERITY_ERROR, ErrorHandlerFake_LastSeverity());
    STRCMP_EQUAL(SOLIDSYSLOG_ERROR_MSG_SOLIDSYSLOG_POOL_EXHAUSTED, ErrorHandlerFake_LastMessage());
}

TEST(SolidSyslogPool, FallbackLogSilentlyDropsTheMessage)
{
    FillPool();
    overflow = SolidSyslog_Create(&config);

    SolidSyslogMessage message = {SOLIDSYSLOG_FACILITY_LOCAL0, SOLIDSYSLOG_SEVERITY_INFORMATIONAL, nullptr, nullptr};
    SolidSyslog_Log(overflow, &message);
    SolidSyslog_Service(overflow);

    CALLED_FAKE_ON(SenderFake_Send, fakeSender, NEVER);
}

TEST(SolidSyslogPool, CreateAcquiresAndReleasesConfigLockOnFirstFreeSlot)
{
    ConfigLockFake_Install();

    pooled[0] = SolidSyslog_Create(&config);

    CALLED_FAKE(ConfigLockFake_Lock, ONCE);
    CALLED_FAKE(ConfigLockFake_Unlock, ONCE);
}

TEST(SolidSyslogPool, CreateLocksOncePerSlotProbedWhenPoolIsFull)
{
    FillPool();
    ConfigLockFake_Install();

    overflow = SolidSyslog_Create(&config);

    LONGS_EQUAL(SOLIDSYSLOG_POOL_SIZE, ConfigLockFake_LockCallCount());
    LONGS_EQUAL(SOLIDSYSLOG_POOL_SIZE, ConfigLockFake_UnlockCallCount());
}

TEST(SolidSyslogPool, DestroyOfPooledHandleLocksOnce)
{
    pooled[0] = SolidSyslog_Create(&config);
    ConfigLockFake_Install();

    SolidSyslog_Destroy(pooled[0]);
    pooled[0] = nullptr;

    CALLED_FAKE(ConfigLockFake_Lock, ONCE);
    CALLED_FAKE(ConfigLockFake_Unlock, ONCE);
}

TEST(SolidSyslogPool, DestroyOfUnknownHandleDoesNotLock)
{
    ConfigLockFake_Install();
    /* Any non-pool address — cast a stack byte, value never dereferenced. */
    char stackByte = 0;
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast) -- forging an "unknown" handle to drive the bad-setup path
    auto* stranger = reinterpret_cast<struct SolidSyslog*>(&stackByte);

    SolidSyslog_Destroy(stranger);

    CALLED_FAKE(ConfigLockFake_Lock, NEVER);
    CALLED_FAKE(ConfigLockFake_Unlock, NEVER);
}

TEST(SolidSyslogPool, DestroyOfUnknownHandleReportsWarning)
{
    ErrorHandlerFake_Install(nullptr);
    char stackByte = 0;
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast) -- forging an "unknown" handle to drive the bad-setup path
    auto* stranger = reinterpret_cast<struct SolidSyslog*>(&stackByte);

    SolidSyslog_Destroy(stranger);

    CALLED_FAKE(ErrorHandlerFake_Handle, ONCE);
    LONGS_EQUAL(SOLIDSYSLOG_SEVERITY_WARNING, ErrorHandlerFake_LastSeverity());
    STRCMP_EQUAL(SOLIDSYSLOG_ERROR_MSG_SOLIDSYSLOG_UNKNOWN_DESTROY, ErrorHandlerFake_LastMessage());
}

TEST(SolidSyslogPool, DestroyOfStaleHandleReportsWarning)
{
    pooled[0] = SolidSyslog_Create(&config);
    SolidSyslog_Destroy(pooled[0]);
    ErrorHandlerFake_Install(nullptr);

    SolidSyslog_Destroy(pooled[0]);
    pooled[0] = nullptr;

    CALLED_FAKE(ErrorHandlerFake_Handle, ONCE);
    LONGS_EQUAL(SOLIDSYSLOG_SEVERITY_WARNING, ErrorHandlerFake_LastSeverity());
    STRCMP_EQUAL(SOLIDSYSLOG_ERROR_MSG_SOLIDSYSLOG_UNKNOWN_DESTROY, ErrorHandlerFake_LastMessage());
}
