#include "CppUTest/TestHarness.h"

extern "C"
{
#include "ConfigLockFake.h"
#include "ErrorHandlerFake.h"
#include "SolidSyslogErrorMessages.h"
#include "SolidSyslogMbedTlsStream.h"
#include "SolidSyslogNullStream.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogStream.h"
#include "SolidSyslogStreamDefinition.h"
#include "SolidSyslogTunables.h"
}

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
TEST_GROUP(SolidSyslogMbedTlsStreamPool)
{
    struct SolidSyslogMbedTlsStreamConfig config = {};
    // cppcheck-suppress constVariable -- assigned in test bodies; cppcheck does not model CppUTest lifecycle
    struct SolidSyslogStream* pooled[SOLIDSYSLOG_MBED_TLS_STREAM_POOL_SIZE] = {};
    struct SolidSyslogStream* overflow                                      = nullptr;

    void teardown() override
    {
        for (auto* handle : pooled)
        {
            if (handle != nullptr)
            {
                SolidSyslogMbedTlsStream_Destroy(handle);
            }
        }
        // cppcheck-suppress knownConditionTrueFalse -- assigned in test bodies; cppcheck does not model CppUTest lifecycle
        if (overflow != nullptr)
        {
            SolidSyslogMbedTlsStream_Destroy(overflow);
        }
        ConfigLockFake_Uninstall();
        ErrorHandlerFake_Uninstall();
    }

    void FillPool()
    {
        for (auto*& slot : pooled)
        {
            slot = SolidSyslogMbedTlsStream_Create(&config);
        }
    }
};

// clang-format on

TEST(SolidSyslogMbedTlsStreamPool, CreateReturnsHandleDistinctFromFallback)
{
    struct SolidSyslogStream* handle = SolidSyslogMbedTlsStream_Create(&config);

    CHECK_TEXT(handle != nullptr, "Create returned nullptr");
    CHECK_TEXT(handle != SolidSyslogNullStream_Get(), "Create returned the NullStream fallback");

    SolidSyslogMbedTlsStream_Destroy(handle);
}

TEST(SolidSyslogMbedTlsStreamPool, FillingPoolThenOverflowReturnsDistinctFallback)
{
    FillPool();

    overflow = SolidSyslogMbedTlsStream_Create(&config);

    CHECK_IS_FALLBACK(overflow, pooled);
}

TEST(SolidSyslogMbedTlsStreamPool, ExhaustedCreateReportsError)
{
    ErrorHandlerFake_Install(nullptr);
    FillPool();

    overflow = SolidSyslogMbedTlsStream_Create(&config);

    CALLED_FAKE(ErrorHandlerFake_Handle, ONCE);
    LONGS_EQUAL(SOLIDSYSLOG_SEVERITY_ERROR, ErrorHandlerFake_LastSeverity());
    STRCMP_EQUAL(SOLIDSYSLOG_ERROR_MSG_MBEDTLSSTREAM_POOL_EXHAUSTED, ErrorHandlerFake_LastMessage());
}

TEST(SolidSyslogMbedTlsStreamPool, FallbackSendReturnsTrueToDropOnTheFloor)
{
    FillPool();
    overflow = SolidSyslogMbedTlsStream_Create(&config);

    CHECK_TRUE(SolidSyslogStream_Send(overflow, "x", 1));
}

TEST(SolidSyslogMbedTlsStreamPool, CreateAcquiresAndReleasesConfigLockOnFirstFreeSlot)
{
    ConfigLockFake_Install();

    pooled[0] = SolidSyslogMbedTlsStream_Create(&config);

    CALLED_FAKE(ConfigLockFake_Lock, ONCE);
    CALLED_FAKE(ConfigLockFake_Unlock, ONCE);
}

TEST(SolidSyslogMbedTlsStreamPool, CreateLocksOncePerSlotProbedWhenPoolIsFull)
{
    FillPool();
    ConfigLockFake_Install();

    overflow = SolidSyslogMbedTlsStream_Create(&config);

    LONGS_EQUAL(SOLIDSYSLOG_MBED_TLS_STREAM_POOL_SIZE, ConfigLockFake_LockCallCount());
    LONGS_EQUAL(SOLIDSYSLOG_MBED_TLS_STREAM_POOL_SIZE, ConfigLockFake_UnlockCallCount());
}

TEST(SolidSyslogMbedTlsStreamPool, DestroyOfPooledHandleLocksOnce)
{
    pooled[0] = SolidSyslogMbedTlsStream_Create(&config);
    ConfigLockFake_Install();

    SolidSyslogMbedTlsStream_Destroy(pooled[0]);
    pooled[0] = nullptr;

    CALLED_FAKE(ConfigLockFake_Lock, ONCE);
    CALLED_FAKE(ConfigLockFake_Unlock, ONCE);
}

TEST(SolidSyslogMbedTlsStreamPool, DestroyOfUnknownHandleDoesNotLock)
{
    ConfigLockFake_Install();
    struct SolidSyslogStream stranger = {};

    SolidSyslogMbedTlsStream_Destroy(&stranger);

    CALLED_FAKE(ConfigLockFake_Lock, NEVER);
    CALLED_FAKE(ConfigLockFake_Unlock, NEVER);
}

TEST(SolidSyslogMbedTlsStreamPool, DestroyOfUnknownHandleReportsWarning)
{
    ErrorHandlerFake_Install(nullptr);
    struct SolidSyslogStream stranger = {};

    SolidSyslogMbedTlsStream_Destroy(&stranger);

    CALLED_FAKE(ErrorHandlerFake_Handle, ONCE);
    LONGS_EQUAL(SOLIDSYSLOG_SEVERITY_WARNING, ErrorHandlerFake_LastSeverity());
    STRCMP_EQUAL(SOLIDSYSLOG_ERROR_MSG_MBEDTLSSTREAM_UNKNOWN_DESTROY, ErrorHandlerFake_LastMessage());
}

TEST(SolidSyslogMbedTlsStreamPool, DestroyOfStaleHandleReportsWarning)
{
    pooled[0] = SolidSyslogMbedTlsStream_Create(&config);
    SolidSyslogMbedTlsStream_Destroy(pooled[0]);
    ErrorHandlerFake_Install(nullptr);

    SolidSyslogMbedTlsStream_Destroy(pooled[0]);
    pooled[0] = nullptr;

    CALLED_FAKE(ErrorHandlerFake_Handle, ONCE);
    LONGS_EQUAL(SOLIDSYSLOG_SEVERITY_WARNING, ErrorHandlerFake_LastSeverity());
    STRCMP_EQUAL(SOLIDSYSLOG_ERROR_MSG_MBEDTLSSTREAM_UNKNOWN_DESTROY, ErrorHandlerFake_LastMessage());
}
