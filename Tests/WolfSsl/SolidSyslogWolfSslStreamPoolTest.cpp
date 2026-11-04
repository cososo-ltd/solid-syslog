#include "CppUTest/TestHarness.h"

extern "C"
{
#include "ConfigLockFake.h"
#include "ErrorHandlerFake.h"
#include "SolidSyslogNullStream.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogStream.h"
#include "SolidSyslogStreamDefinition.h"
#include "SolidSyslogTunables.h"
#include "SolidSyslogWolfSslCredentialsDefinition.h"
#include "SolidSyslogWolfSslStream.h"
#include "SolidSyslogWolfSslStreamErrors.h"
#include "StreamFake.h"
#include "WolfSslCredentialsFake.h"
#include "WolfSslTlsFake.h"
}

#include "SolidSyslogErrorCategory.h"
#include "TestUtils.h"

using namespace CososoTesting;

namespace
{
void NoOpSleep(uint32_t milliseconds)
{
    (void) milliseconds;
}
} // namespace

// Asserts Create refused the configuration and handed back the shared NullStream.
#define CHECK_NULL_STREAM(handle) POINTERS_EQUAL(SolidSyslogNullStream_Get(), (handle))

// Asserts handle is non-null and not one of the slots in pool.
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
TEST_GROUP(SolidSyslogWolfSslStreamPool)
{
    struct SolidSyslogStream*             transport = nullptr;
    struct SolidSyslogWolfSslStreamConfig config    = {};
    struct SolidSyslogStream* pooled[SOLIDSYSLOG_TLS_STREAM_POOL_SIZE] = {};
    struct SolidSyslogStream* overflow                                 = nullptr;

    void setup() override
    {
        WolfSslTlsFake_Reset();
        transport        = StreamFake_Create();
        config.Transport = transport;
        config.Sleep     = NoOpSleep;
        WolfSslCredentialsFake_Reset();
        config.Credentials = WolfSslCredentialsFake_Get();
    }

    void teardown() override
    {
        for (auto* handle : pooled)
        {
            if (handle != nullptr)
            {
                SolidSyslogWolfSslStream_Destroy(handle);
            }
        }
        if (overflow != nullptr)
        {
            SolidSyslogWolfSslStream_Destroy(overflow);
        }
        StreamFake_Destroy(transport);
        ConfigLockFake_Uninstall();
    }

    void FillPool()
    {
        for (auto*& slot : pooled)
        {
            slot = SolidSyslogWolfSslStream_Create(&config);
        }
    }
};

// clang-format on

TEST(SolidSyslogWolfSslStreamPool, CreateWithNullConfigReturnsFallback)
{
    struct SolidSyslogStream* fallback = SolidSyslogWolfSslStream_Create(nullptr);

    CHECK_NULL_STREAM(fallback);
}

TEST(SolidSyslogWolfSslStreamPool, CreateWithNullConfigReportsError)
{
    ErrorHandlerFake_Install(nullptr);

    SolidSyslogWolfSslStream_Create(nullptr);

    CHECK_ERROR_REPORTED_ONCE(
        SOLIDSYSLOG_SEVERITY_CRITICAL,
        &SolidSyslogWolfSslStreamErrorSource,
        SOLIDSYSLOG_CAT_BAD_CONFIG,
        SOLIDSYSLOG_TLS_STREAM_ERROR_NULL_CONFIG
    );
}

TEST(SolidSyslogWolfSslStreamPool, CreateWithNullTransportReturnsFallback)
{
    config.Transport = nullptr;

    struct SolidSyslogStream* fallback = SolidSyslogWolfSslStream_Create(&config);

    CHECK_NULL_STREAM(fallback);
}

TEST(SolidSyslogWolfSslStreamPool, CreateWithNullTransportReportsError)
{
    ErrorHandlerFake_Install(nullptr);
    config.Transport = nullptr;

    SolidSyslogWolfSslStream_Create(&config);

    CHECK_ERROR_REPORTED_ONCE(
        SOLIDSYSLOG_SEVERITY_CRITICAL,
        &SolidSyslogWolfSslStreamErrorSource,
        SOLIDSYSLOG_CAT_BAD_CONFIG,
        SOLIDSYSLOG_TLS_STREAM_ERROR_NULL_TRANSPORT
    );
}

TEST(SolidSyslogWolfSslStreamPool, CreateWithNullSleepReturnsFallback)
{
    config.Sleep = nullptr;

    struct SolidSyslogStream* fallback = SolidSyslogWolfSslStream_Create(&config);

    CHECK_NULL_STREAM(fallback);
}

TEST(SolidSyslogWolfSslStreamPool, CreateWithNullSleepReportsError)
{
    ErrorHandlerFake_Install(nullptr);
    config.Sleep = nullptr;

    SolidSyslogWolfSslStream_Create(&config);

    CHECK_ERROR_REPORTED_ONCE(
        SOLIDSYSLOG_SEVERITY_CRITICAL,
        &SolidSyslogWolfSslStreamErrorSource,
        SOLIDSYSLOG_CAT_BAD_CONFIG,
        SOLIDSYSLOG_TLS_STREAM_ERROR_NULL_SLEEP
    );
}

TEST(SolidSyslogWolfSslStreamPool, CreateWithNullCredentialsReturnsFallback)
{
    config.Credentials = nullptr;

    struct SolidSyslogStream* fallback = SolidSyslogWolfSslStream_Create(&config);

    CHECK_NULL_STREAM(fallback);
}

TEST(SolidSyslogWolfSslStreamPool, CreateWithNullCredentialsReportsError)
{
    ErrorHandlerFake_Install(nullptr);
    config.Credentials = nullptr;

    SolidSyslogWolfSslStream_Create(&config);

    CHECK_ERROR_REPORTED_ONCE(
        SOLIDSYSLOG_SEVERITY_CRITICAL,
        &SolidSyslogWolfSslStreamErrorSource,
        SOLIDSYSLOG_CAT_BAD_CONFIG,
        SOLIDSYSLOG_TLS_STREAM_ERROR_NULL_CREDENTIALS
    );
}

TEST(SolidSyslogWolfSslStreamPool, CreateReturnsHandleDistinctFromFallback)
{
    struct SolidSyslogStream* handle = SolidSyslogWolfSslStream_Create(&config);

    CHECK_TEXT(handle != nullptr, "Create returned nullptr");
    CHECK_TEXT(handle != SolidSyslogNullStream_Get(), "Create returned the NullStream fallback");

    SolidSyslogWolfSslStream_Destroy(handle);
}

TEST(SolidSyslogWolfSslStreamPool, FillingPoolThenOverflowReturnsDistinctFallback)
{
    FillPool();

    overflow = SolidSyslogWolfSslStream_Create(&config);

    CHECK_IS_FALLBACK(overflow, pooled);
}

TEST(SolidSyslogWolfSslStreamPool, ExhaustedCreateReportsError)
{
    ErrorHandlerFake_Install(nullptr);
    FillPool();

    overflow = SolidSyslogWolfSslStream_Create(&config);

    CHECK_ERROR_REPORTED_ONCE(
        SOLIDSYSLOG_SEVERITY_CRITICAL,
        &SolidSyslogWolfSslStreamErrorSource,
        SOLIDSYSLOG_CAT_POOL_EXHAUSTED,
        SOLIDSYSLOG_TLS_STREAM_ERROR_POOL_EXHAUSTED
    );
}

TEST(SolidSyslogWolfSslStreamPool, FallbackSendReturnsTrueToDropOnTheFloor)
{
    FillPool();
    overflow = SolidSyslogWolfSslStream_Create(&config);

    CHECK_TRUE(SolidSyslogStream_Send(overflow, "x", 1));
}

TEST(SolidSyslogWolfSslStreamPool, CreateAcquiresAndReleasesConfigLockOnFirstFreeSlot)
{
    ConfigLockFake_Install();

    pooled[0] = SolidSyslogWolfSslStream_Create(&config);

    CALLED_FAKE(ConfigLockFake_Lock, ONCE);
    CALLED_FAKE(ConfigLockFake_Unlock, ONCE);
}

TEST(SolidSyslogWolfSslStreamPool, CreateLocksOncePerSlotProbedWhenPoolIsFull)
{
    FillPool();
    ConfigLockFake_Install();

    overflow = SolidSyslogWolfSslStream_Create(&config);

    LONGS_EQUAL(SOLIDSYSLOG_TLS_STREAM_POOL_SIZE, ConfigLockFake_LockCallCount());
    LONGS_EQUAL(SOLIDSYSLOG_TLS_STREAM_POOL_SIZE, ConfigLockFake_UnlockCallCount());
}

TEST(SolidSyslogWolfSslStreamPool, DestroyOfPooledHandleLocksOnce)
{
    pooled[0] = SolidSyslogWolfSslStream_Create(&config);
    ConfigLockFake_Install();

    SolidSyslogWolfSslStream_Destroy(pooled[0]);
    pooled[0] = nullptr;

    CALLED_FAKE(ConfigLockFake_Lock, ONCE);
    CALLED_FAKE(ConfigLockFake_Unlock, ONCE);
}

TEST(SolidSyslogWolfSslStreamPool, DestroyOfUnknownHandleDoesNotLock)
{
    ConfigLockFake_Install();
    struct SolidSyslogStream stranger = {};

    SolidSyslogWolfSslStream_Destroy(&stranger);

    CALLED_FAKE(ConfigLockFake_Lock, NEVER);
    CALLED_FAKE(ConfigLockFake_Unlock, NEVER);
}

TEST(SolidSyslogWolfSslStreamPool, DestroyOfUnknownHandleReportsWarning)
{
    ErrorHandlerFake_Install(nullptr);
    struct SolidSyslogStream stranger = {};

    SolidSyslogWolfSslStream_Destroy(&stranger);

    CHECK_ERROR_REPORTED_ONCE(
        SOLIDSYSLOG_SEVERITY_WARNING,
        &SolidSyslogWolfSslStreamErrorSource,
        SOLIDSYSLOG_CAT_UNKNOWN_DESTROY,
        SOLIDSYSLOG_TLS_STREAM_ERROR_UNKNOWN_DESTROY
    );
}

TEST(SolidSyslogWolfSslStreamPool, DestroyOfStaleHandleReportsWarning)
{
    pooled[0] = SolidSyslogWolfSslStream_Create(&config);
    SolidSyslogWolfSslStream_Destroy(pooled[0]);
    ErrorHandlerFake_Install(nullptr);

    SolidSyslogWolfSslStream_Destroy(pooled[0]);
    pooled[0] = nullptr;

    CHECK_ERROR_REPORTED_ONCE(
        SOLIDSYSLOG_SEVERITY_WARNING,
        &SolidSyslogWolfSslStreamErrorSource,
        SOLIDSYSLOG_CAT_UNKNOWN_DESTROY,
        SOLIDSYSLOG_TLS_STREAM_ERROR_UNKNOWN_DESTROY
    );
}

TEST(SolidSyslogWolfSslStreamPool, DestroyOfAnOpenStreamClosesTheTransport)
{
    pooled[0] = SolidSyslogWolfSslStream_Create(&config);
    SolidSyslogStream_Open(pooled[0], nullptr);

    SolidSyslogWolfSslStream_Destroy(pooled[0]);
    pooled[0] = nullptr;

    LONGS_EQUAL(1, StreamFake_CloseCallCount(transport));
}

TEST(SolidSyslogWolfSslStreamPool, ADestroyedHandleSendsAsTheNullStream)
{
    pooled[0] = SolidSyslogWolfSslStream_Create(&config);
    SolidSyslogWolfSslStream_Destroy(pooled[0]);

    CHECK_TRUE(SolidSyslogStream_Send(pooled[0], "x", 1));
    pooled[0] = nullptr;
}
