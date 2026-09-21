#include "TestUtils.h"
#include "CppUTest/TestHarness.h"

using namespace CososoTesting;

#include "lwip/sockets.h"

#include "ConfigLockFake.h"
#include "ErrorHandlerFake.h"
#include "LwipSocketsFake.h"
#include "SolidSyslogErrorCategory.h"
#include "SolidSyslogLwipSocketTcpStream.h"
#include "SolidSyslogLwipSocketTcpStreamErrors.h"
#include "SolidSyslogNullStream.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogStream.h"
#include "SolidSyslogTunables.h"

static const struct SolidSyslogLwipSocketTcpStreamConfig config = {nullptr, nullptr};

// clang-format off
TEST_GROUP(SolidSyslogLwipSocketTcpStreamPool)
{
    struct SolidSyslogStream* pooled[SOLIDSYSLOG_TCP_STREAM_POOL_SIZE] = {};
    struct SolidSyslogStream* overflow                               = nullptr;

    void teardown() override
    {
        for (auto* handle : pooled)
        {
            if (handle != nullptr)
            {
                SolidSyslogLwipSocketTcpStream_Destroy(handle);
            }
        }
        ConfigLockFake_Uninstall();
    }

    void FillPool()
    {
        for (auto*& slot : pooled)
        {
            slot = SolidSyslogLwipSocketTcpStream_Create(&config);
        }
    }
};

// clang-format on

TEST(SolidSyslogLwipSocketTcpStreamPool, FillingPoolThenOverflowReturnsTheNullStream)
{
    FillPool();

    overflow = SolidSyslogLwipSocketTcpStream_Create(&config);

    POINTERS_EQUAL(SolidSyslogNullStream_Get(), overflow);
}

TEST(SolidSyslogLwipSocketTcpStreamPool, ExhaustedCreateReportsError)
{
    ErrorHandlerFake_Install(nullptr);
    FillPool();

    overflow = SolidSyslogLwipSocketTcpStream_Create(&config);

    CHECK_ERROR_REPORTED_ONCE(
        SOLIDSYSLOG_SEVERITY_CRITICAL,
        &SolidSyslogLwipSocketTcpStreamErrorSource,
        SOLIDSYSLOG_CAT_POOL_EXHAUSTED,
        SOLIDSYSLOG_TCP_STREAM_ERROR_POOL_EXHAUSTED
    );
}

TEST(SolidSyslogLwipSocketTcpStreamPool, CreateAcquiresAndReleasesConfigLockOnFirstFreeSlot)
{
    ConfigLockFake_Install();

    pooled[0] = SolidSyslogLwipSocketTcpStream_Create(&config);

    CALLED_FAKE(ConfigLockFake_Lock, ONCE);
    CALLED_FAKE(ConfigLockFake_Unlock, ONCE);
}

TEST(SolidSyslogLwipSocketTcpStreamPool, CreateLocksOncePerSlotProbedWhenPoolIsFull)
{
    FillPool();
    ConfigLockFake_Install();

    overflow = SolidSyslogLwipSocketTcpStream_Create(&config);

    LONGS_EQUAL(SOLIDSYSLOG_TCP_STREAM_POOL_SIZE, ConfigLockFake_LockCallCount());
    LONGS_EQUAL(SOLIDSYSLOG_TCP_STREAM_POOL_SIZE, ConfigLockFake_UnlockCallCount());
}

TEST(SolidSyslogLwipSocketTcpStreamPool, DestroyOfPooledHandleLocksOnce)
{
    pooled[0] = SolidSyslogLwipSocketTcpStream_Create(&config);
    ConfigLockFake_Install();

    SolidSyslogLwipSocketTcpStream_Destroy(pooled[0]);
    pooled[0] = nullptr;

    CALLED_FAKE(ConfigLockFake_Lock, ONCE);
    CALLED_FAKE(ConfigLockFake_Unlock, ONCE);
}

TEST(SolidSyslogLwipSocketTcpStreamPool, DestroyOfUnknownHandleDoesNotLock)
{
    ConfigLockFake_Install();
    char stranger = 0;

    SolidSyslogLwipSocketTcpStream_Destroy((struct SolidSyslogStream*) &stranger);

    CALLED_FAKE(ConfigLockFake_Lock, NEVER);
    CALLED_FAKE(ConfigLockFake_Unlock, NEVER);
}

TEST(SolidSyslogLwipSocketTcpStreamPool, DestroyOfUnknownHandleReportsWarning)
{
    ErrorHandlerFake_Install(nullptr);
    char stranger = 0;

    SolidSyslogLwipSocketTcpStream_Destroy((struct SolidSyslogStream*) &stranger);

    CHECK_ERROR_REPORTED_ONCE(
        SOLIDSYSLOG_SEVERITY_WARNING,
        &SolidSyslogLwipSocketTcpStreamErrorSource,
        SOLIDSYSLOG_CAT_UNKNOWN_DESTROY,
        SOLIDSYSLOG_TCP_STREAM_ERROR_UNKNOWN_DESTROY
    );
}

TEST(SolidSyslogLwipSocketTcpStreamPool, DestroyOfStaleHandleReportsWarning)
{
    pooled[0] = SolidSyslogLwipSocketTcpStream_Create(&config);
    SolidSyslogLwipSocketTcpStream_Destroy(pooled[0]);
    ErrorHandlerFake_Install(nullptr);

    SolidSyslogLwipSocketTcpStream_Destroy(pooled[0]);
    pooled[0] = nullptr;

    CHECK_ERROR_REPORTED_ONCE(
        SOLIDSYSLOG_SEVERITY_WARNING,
        &SolidSyslogLwipSocketTcpStreamErrorSource,
        SOLIDSYSLOG_CAT_UNKNOWN_DESTROY,
        SOLIDSYSLOG_TCP_STREAM_ERROR_UNKNOWN_DESTROY
    );
}

