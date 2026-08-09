#include "CppUTest/TestHarness.h"

extern "C"
{
#include "ConfigLockFake.h"
#include "ErrorHandlerFake.h"
#include "OpenSslFake.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogStream.h"
#include "SolidSyslogStreamDefinition.h"
#include "SolidSyslogOpenSslStream.h"
#include "SolidSyslogOpenSslStreamErrors.h"
#include "SolidSyslogTunables.h"
#include "StreamFake.h"
}

#include "SolidSyslogErrorCategory.h"
#include "TestUtils.h"

using namespace CososoTesting;

namespace
{
void NoOpSleep(int milliseconds)
{
    (void) milliseconds;
}
} // namespace

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
TEST_GROUP(SolidSyslogOpenSslStreamPool)
{
    struct SolidSyslogStream*         transport = nullptr;
    struct SolidSyslogOpenSslStreamConfig config    = {};
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
                SolidSyslogOpenSslStream_Destroy(handle);
            }
        }
        if (overflow != nullptr)
        {
            SolidSyslogOpenSslStream_Destroy(overflow);
        }
        StreamFake_Destroy(transport);
        ConfigLockFake_Uninstall();
    }

    void FillPool()
    {
        for (auto*& slot : pooled)
        {
            slot = SolidSyslogOpenSslStream_Create(&config);
        }
    }
};

// clang-format on

TEST(SolidSyslogOpenSslStreamPool, FillingPoolThenOverflowReturnsDistinctFallback)
{
    FillPool();

    overflow = SolidSyslogOpenSslStream_Create(&config);

    CHECK_IS_FALLBACK(overflow, pooled);
}

TEST(SolidSyslogOpenSslStreamPool, ExhaustedCreateReportsError)
{
    ErrorHandlerFake_Install(nullptr);
    FillPool();

    overflow = SolidSyslogOpenSslStream_Create(&config);

    CALLED_FAKE(ErrorHandlerFake_Handle, ONCE);
    LONGS_EQUAL(SOLIDSYSLOG_SEVERITY_CRITICAL, ErrorHandlerFake_LastSeverity());
    POINTERS_EQUAL(&OpenSslStreamErrorSource, ErrorHandlerFake_LastSource());
    UNSIGNED_LONGS_EQUAL(SOLIDSYSLOG_CAT_POOL_EXHAUSTED, ErrorHandlerFake_LastCategory());
    UNSIGNED_LONGS_EQUAL(OPENSSLSTREAM_ERROR_POOL_EXHAUSTED, ErrorHandlerFake_LastDetail());
}

TEST(SolidSyslogOpenSslStreamPool, FallbackSendReturnsTrueToDropOnTheFloor)
{
    FillPool();
    overflow = SolidSyslogOpenSslStream_Create(&config);

    CHECK_TRUE(SolidSyslogStream_Send(overflow, "x", 1));
}

TEST(SolidSyslogOpenSslStreamPool, CreateAcquiresAndReleasesConfigLockOnFirstFreeSlot)
{
    ConfigLockFake_Install();

    pooled[0] = SolidSyslogOpenSslStream_Create(&config);

    CALLED_FAKE(ConfigLockFake_Lock, ONCE);
    CALLED_FAKE(ConfigLockFake_Unlock, ONCE);
}

TEST(SolidSyslogOpenSslStreamPool, CreateLocksOncePerSlotProbedWhenPoolIsFull)
{
    FillPool();
    ConfigLockFake_Install();

    overflow = SolidSyslogOpenSslStream_Create(&config);

    LONGS_EQUAL(SOLIDSYSLOG_TLS_STREAM_POOL_SIZE, ConfigLockFake_LockCallCount());
    LONGS_EQUAL(SOLIDSYSLOG_TLS_STREAM_POOL_SIZE, ConfigLockFake_UnlockCallCount());
}

TEST(SolidSyslogOpenSslStreamPool, DestroyOfPooledHandleLocksOnce)
{
    pooled[0] = SolidSyslogOpenSslStream_Create(&config);
    ConfigLockFake_Install();

    SolidSyslogOpenSslStream_Destroy(pooled[0]);
    pooled[0] = nullptr;

    CALLED_FAKE(ConfigLockFake_Lock, ONCE);
    CALLED_FAKE(ConfigLockFake_Unlock, ONCE);
}

TEST(SolidSyslogOpenSslStreamPool, DestroyOfUnknownHandleDoesNotLock)
{
    ConfigLockFake_Install();
    struct SolidSyslogStream stranger = {};

    SolidSyslogOpenSslStream_Destroy(&stranger);

    CALLED_FAKE(ConfigLockFake_Lock, NEVER);
    CALLED_FAKE(ConfigLockFake_Unlock, NEVER);
}

TEST(SolidSyslogOpenSslStreamPool, DestroyOfUnknownHandleReportsWarning)
{
    ErrorHandlerFake_Install(nullptr);
    struct SolidSyslogStream stranger = {};

    SolidSyslogOpenSslStream_Destroy(&stranger);

    CALLED_FAKE(ErrorHandlerFake_Handle, ONCE);
    LONGS_EQUAL(SOLIDSYSLOG_SEVERITY_WARNING, ErrorHandlerFake_LastSeverity());
    POINTERS_EQUAL(&OpenSslStreamErrorSource, ErrorHandlerFake_LastSource());
    UNSIGNED_LONGS_EQUAL(SOLIDSYSLOG_CAT_UNKNOWN_DESTROY, ErrorHandlerFake_LastCategory());
    UNSIGNED_LONGS_EQUAL(OPENSSLSTREAM_ERROR_UNKNOWN_DESTROY, ErrorHandlerFake_LastDetail());
}

TEST(SolidSyslogOpenSslStreamPool, DestroyOfStaleHandleReportsWarning)
{
    pooled[0] = SolidSyslogOpenSslStream_Create(&config);
    SolidSyslogOpenSslStream_Destroy(pooled[0]);
    ErrorHandlerFake_Install(nullptr);

    SolidSyslogOpenSslStream_Destroy(pooled[0]);
    pooled[0] = nullptr;

    CALLED_FAKE(ErrorHandlerFake_Handle, ONCE);
    LONGS_EQUAL(SOLIDSYSLOG_SEVERITY_WARNING, ErrorHandlerFake_LastSeverity());
    POINTERS_EQUAL(&OpenSslStreamErrorSource, ErrorHandlerFake_LastSource());
    UNSIGNED_LONGS_EQUAL(SOLIDSYSLOG_CAT_UNKNOWN_DESTROY, ErrorHandlerFake_LastCategory());
    UNSIGNED_LONGS_EQUAL(OPENSSLSTREAM_ERROR_UNKNOWN_DESTROY, ErrorHandlerFake_LastDetail());
}
