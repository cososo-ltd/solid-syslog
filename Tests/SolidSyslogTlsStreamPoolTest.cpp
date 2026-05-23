#include "CppUTest/TestHarness.h"

extern "C"
{
#include "ConfigLockFake.h"
#include "ErrorHandlerFake.h"
#include "ErrorHandlerFakeEx.h"
#include "OpenSslFake.h"
#include "SolidSyslogErrorMessages.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogStream.h"
#include "SolidSyslogStreamDefinition.h"
#include "SolidSyslogTlsStream.h"
#include "SolidSyslogTlsStreamErrors.h"
#include "SolidSyslogTunables.h"
#include "StreamFake.h"
}

#include "TestUtils.h"

using namespace CososoTesting; // NOLINT(google-build-using-namespace) -- test-file scope only; brings ONCE/NEVER into scope for CALLED_FAKE

namespace
{
void NoOpSleep(int milliseconds)
{
    (void) milliseconds;
}
} // namespace

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
TEST_GROUP(SolidSyslogTlsStreamPool)
{
    struct SolidSyslogStream*         transport = nullptr;
    struct SolidSyslogTlsStreamConfig config    = {};
    // cppcheck-suppress constVariable -- assigned in test bodies; cppcheck does not model CppUTest lifecycle
    struct SolidSyslogStream* pooled[SOLIDSYSLOG_TLS_STREAM_POOL_SIZE] = {};
    struct SolidSyslogStream* overflow                                 = nullptr;

    void setup() override
    {
        OpenSslFake_Reset();
        transport = StreamFake_Create();
        config.Transport = transport;
        config.Sleep = NoOpSleep;
    }

    void teardown() override
    {
        for (auto* handle : pooled)
        {
            if (handle != nullptr)
            {
                SolidSyslogTlsStream_Destroy(handle);
            }
        }
        // cppcheck-suppress knownConditionTrueFalse -- assigned in test bodies; cppcheck does not model CppUTest lifecycle
        if (overflow != nullptr)
        {
            SolidSyslogTlsStream_Destroy(overflow);
        }
        StreamFake_Destroy(transport);
        ConfigLockFake_Uninstall();
        ErrorHandlerFake_Uninstall();
    }

    void FillPool()
    {
        for (auto*& slot : pooled)
        {
            slot = SolidSyslogTlsStream_Create(&config);
        }
    }
};

// clang-format on

TEST(SolidSyslogTlsStreamPool, FillingPoolThenOverflowReturnsDistinctFallback)
{
    FillPool();

    overflow = SolidSyslogTlsStream_Create(&config);

    CHECK_IS_FALLBACK(overflow, pooled);
}

TEST(SolidSyslogTlsStreamPool, ExhaustedCreateReportsError)
{
    ErrorHandlerFakeEx_Install(nullptr);
    FillPool();

    overflow = SolidSyslogTlsStream_Create(&config);

    CALLED_FAKE(ErrorHandlerFakeEx_Handle, ONCE);
    LONGS_EQUAL(SOLIDSYSLOG_SEVERITY_ERROR, ErrorHandlerFakeEx_LastSeverity());
    POINTERS_EQUAL(&TlsStreamErrorSource, ErrorHandlerFakeEx_LastSource());
    UNSIGNED_LONGS_EQUAL(TLSSTREAM_ERROR_POOL_EXHAUSTED, ErrorHandlerFakeEx_LastCode());
}

TEST(SolidSyslogTlsStreamPool, FallbackSendReturnsTrueToDropOnTheFloor)
{
    FillPool();
    overflow = SolidSyslogTlsStream_Create(&config);

    CHECK_TRUE(SolidSyslogStream_Send(overflow, "x", 1));
}

TEST(SolidSyslogTlsStreamPool, CreateAcquiresAndReleasesConfigLockOnFirstFreeSlot)
{
    ConfigLockFake_Install();

    pooled[0] = SolidSyslogTlsStream_Create(&config);

    CALLED_FAKE(ConfigLockFake_Lock, ONCE);
    CALLED_FAKE(ConfigLockFake_Unlock, ONCE);
}

TEST(SolidSyslogTlsStreamPool, CreateLocksOncePerSlotProbedWhenPoolIsFull)
{
    FillPool();
    ConfigLockFake_Install();

    overflow = SolidSyslogTlsStream_Create(&config);

    LONGS_EQUAL(SOLIDSYSLOG_TLS_STREAM_POOL_SIZE, ConfigLockFake_LockCallCount());
    LONGS_EQUAL(SOLIDSYSLOG_TLS_STREAM_POOL_SIZE, ConfigLockFake_UnlockCallCount());
}

TEST(SolidSyslogTlsStreamPool, DestroyOfPooledHandleLocksOnce)
{
    pooled[0] = SolidSyslogTlsStream_Create(&config);
    ConfigLockFake_Install();

    SolidSyslogTlsStream_Destroy(pooled[0]);
    pooled[0] = nullptr;

    CALLED_FAKE(ConfigLockFake_Lock, ONCE);
    CALLED_FAKE(ConfigLockFake_Unlock, ONCE);
}

TEST(SolidSyslogTlsStreamPool, DestroyOfUnknownHandleDoesNotLock)
{
    ConfigLockFake_Install();
    struct SolidSyslogStream stranger = {};

    SolidSyslogTlsStream_Destroy(&stranger);

    CALLED_FAKE(ConfigLockFake_Lock, NEVER);
    CALLED_FAKE(ConfigLockFake_Unlock, NEVER);
}

TEST(SolidSyslogTlsStreamPool, DestroyOfUnknownHandleReportsWarning)
{
    ErrorHandlerFakeEx_Install(nullptr);
    struct SolidSyslogStream stranger = {};

    SolidSyslogTlsStream_Destroy(&stranger);

    CALLED_FAKE(ErrorHandlerFakeEx_Handle, ONCE);
    LONGS_EQUAL(SOLIDSYSLOG_SEVERITY_WARNING, ErrorHandlerFakeEx_LastSeverity());
    POINTERS_EQUAL(&TlsStreamErrorSource, ErrorHandlerFakeEx_LastSource());
    UNSIGNED_LONGS_EQUAL(TLSSTREAM_ERROR_UNKNOWN_DESTROY, ErrorHandlerFakeEx_LastCode());
}

TEST(SolidSyslogTlsStreamPool, DestroyOfStaleHandleReportsWarning)
{
    pooled[0] = SolidSyslogTlsStream_Create(&config);
    SolidSyslogTlsStream_Destroy(pooled[0]);
    ErrorHandlerFakeEx_Install(nullptr);

    SolidSyslogTlsStream_Destroy(pooled[0]);
    pooled[0] = nullptr;

    CALLED_FAKE(ErrorHandlerFakeEx_Handle, ONCE);
    LONGS_EQUAL(SOLIDSYSLOG_SEVERITY_WARNING, ErrorHandlerFakeEx_LastSeverity());
    POINTERS_EQUAL(&TlsStreamErrorSource, ErrorHandlerFakeEx_LastSource());
    UNSIGNED_LONGS_EQUAL(TLSSTREAM_ERROR_UNKNOWN_DESTROY, ErrorHandlerFakeEx_LastCode());
}
