#include "TestUtils.h"
#include "CppUTest/TestHarness.h"

using namespace CososoTesting;

#include "lwip/sockets.h"

#include <cerrno>
#include <cstdint>

#include "ConfigLockFake.h"
#include "ErrorHandlerFake.h"
#include "LwipSocketsFake.h"
#include "SolidSyslogErrorCategory.h"
#include "SolidSyslogLwipSocketAddress.h"
#include "SolidSyslogLwipSocketAddressPrivate.h"
#include "SolidSyslogLwipSocketTcpStream.h"
#include "SolidSyslogLwipSocketTcpStreamErrors.h"
#include "SolidSyslogNullStream.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogStream.h"
#include "SolidSyslogStreamCategories.h"
#include "SolidSyslogTunables.h"

static const struct SolidSyslogLwipSocketTcpStreamConfig config = {nullptr, nullptr};

// clang-format off
// 6514 rather than 514: 514 is 0x0202, so host and network order are the same
// bytes and an assertion on the port would hold whichever the adapter wrote.
static const uint16_t TEST_PORT       = 6514U;
static const uint32_t TEST_IPV4       = 0xC000020AU;
static const int      TEST_DESCRIPTOR = 7;
static const uint32_t TEST_CONNECT_TIMEOUT_MS = 20U;
static void* const    TEST_CONTEXT    = reinterpret_cast<void*>(0xABCDU);
// clang-format on

namespace
{
unsigned FakeGetConnectTimeoutMs_CallCount = 0U;
void* FakeGetConnectTimeoutMs_LastContext = nullptr;

extern "C" uint32_t FakeGetConnectTimeoutMs(void* context)
{
    FakeGetConnectTimeoutMs_CallCount++;
    FakeGetConnectTimeoutMs_LastContext = context;
    return TEST_CONNECT_TIMEOUT_MS;
}
} // namespace

// clang-format off
TEST_GROUP(SolidSyslogLwipSocketTcpStream)
{
    struct SolidSyslogStream*  stream  = nullptr;
    struct SolidSyslogAddress* address = nullptr;

    void setup() override
    {
        LwipSocketsFake_Reset();
        FakeGetConnectTimeoutMs_CallCount = 0U;
        FakeGetConnectTimeoutMs_LastContext = nullptr;
        stream  = SolidSyslogLwipSocketTcpStream_Create(&config);
        address = SolidSyslogLwipSocketAddress_Create();
        struct sockaddr_in* sin = SolidSyslogLwipSocketAddress_AsSockaddrIn(address);
        sin->sin_family         = AF_INET;
        sin->sin_port           = lwip_htons(TEST_PORT);
        sin->sin_addr.s_addr    = lwip_htonl(TEST_IPV4);
        // Installed after the pool draws above, so an event count starts clean.
        ErrorHandlerFake_Install(nullptr);
    }

    void teardown() override
    {
        SolidSyslogLwipSocketAddress_Destroy(address);
        SolidSyslogLwipSocketTcpStream_Destroy(stream);
    }

    bool Open() const
    {
        return SolidSyslogStream_Open(stream, address);
    }
};

// clang-format on

TEST(SolidSyslogLwipSocketTcpStream, OpenTakesAnIpv4TcpSocket)
{
    CHECK_TRUE(Open());

    LONGS_EQUAL(1U, LwipSocketsFake_SocketCallCount());
    LONGS_EQUAL(AF_INET, LwipSocketsFake_LastSocketDomain());
    LONGS_EQUAL(SOCK_STREAM, LwipSocketsFake_LastSocketType());
    LONGS_EQUAL(0, LwipSocketsFake_LastSocketProtocol());
}


TEST(SolidSyslogLwipSocketTcpStream, OpenConnectsToTheAddressOnThatSocket)
{
    LwipSocketsFake_SetSocketResult(TEST_DESCRIPTOR);

    CHECK_TRUE(Open());

    LONGS_EQUAL(1U, LwipSocketsFake_ConnectCallCount());
    LONGS_EQUAL(TEST_DESCRIPTOR, LwipSocketsFake_LastConnectSocket());
    LONGS_EQUAL(lwip_htons(TEST_PORT), LwipSocketsFake_LastConnectAddress()->sin_port);
    LONGS_EQUAL(lwip_htonl(TEST_IPV4), LwipSocketsFake_LastConnectAddress()->sin_addr.s_addr);
    LONGS_EQUAL(sizeof(struct sockaddr_in), LwipSocketsFake_LastConnectAddressLength());
}

TEST(SolidSyslogLwipSocketTcpStream, OpenMakesTheSocketNonBlockingBeforeItConnects)
{
    LwipSocketsFake_SetSocketResult(TEST_DESCRIPTOR);

    CHECK_TRUE(Open());

    LONGS_EQUAL(1U, LwipSocketsFake_FcntlCallCount());
    LONGS_EQUAL(TEST_DESCRIPTOR, LwipSocketsFake_LastFcntlSocket());
    LONGS_EQUAL(F_SETFL, LwipSocketsFake_LastFcntlCommand());
    LONGS_EQUAL(O_NONBLOCK, LwipSocketsFake_LastFcntlValue());
    LONGS_EQUAL(1U, LwipSocketsFake_FcntlCallsBeforeConnect());
}

TEST(SolidSyslogLwipSocketTcpStream, AConnectStillInProgressWaitsForTheSocketToBecomeWritable)
{
    LwipSocketsFake_SetSocketResult(TEST_DESCRIPTOR);
    LwipSocketsFake_SetConnectResult(-1, EINPROGRESS);

    CHECK_TRUE(Open());

    LONGS_EQUAL(1U, LwipSocketsFake_SelectCallCount());
    LONGS_EQUAL(TEST_DESCRIPTOR + 1, LwipSocketsFake_LastSelectMaxFdPlusOne());
    LONGS_EQUAL(TEST_DESCRIPTOR, LwipSocketsFake_LastSelectWriteDescriptor());
    LONGS_EQUAL(TEST_DESCRIPTOR, LwipSocketsFake_LastSelectExceptionDescriptor());
}

TEST(SolidSyslogLwipSocketTcpStream, TheWaitIsBoundedByTheTunableWhenTheIntegratorInstallsNoGetter)
{
    LwipSocketsFake_SetConnectResult(-1, EINPROGRESS);

    CHECK_TRUE(Open());

    LONGS_EQUAL(SOLIDSYSLOG_TCP_CONNECT_TIMEOUT_MS, LwipSocketsFake_LastSelectTimeoutMs());
}

TEST(SolidSyslogLwipSocketTcpStream, TheWaitIsBoundedByTheInstalledGetterReadOnEveryAttempt)
{
    SolidSyslogLwipSocketTcpStream_Destroy(stream);
    const struct SolidSyslogLwipSocketTcpStreamConfig tuned = {FakeGetConnectTimeoutMs, TEST_CONTEXT};
    stream = SolidSyslogLwipSocketTcpStream_Create(&tuned);
    LwipSocketsFake_SetConnectResult(-1, EINPROGRESS);

    CHECK_TRUE(Open());
    CHECK_TRUE(Open());

    LONGS_EQUAL(TEST_CONNECT_TIMEOUT_MS, LwipSocketsFake_LastSelectTimeoutMs());
    LONGS_EQUAL(2U, FakeGetConnectTimeoutMs_CallCount);
    POINTERS_EQUAL(TEST_CONTEXT, FakeGetConnectTimeoutMs_LastContext);
}

TEST(SolidSyslogLwipSocketTcpStream, AWritableSocketIsConfirmedByReadingTheDeferredError)
{
    LwipSocketsFake_SetSocketResult(TEST_DESCRIPTOR);
    LwipSocketsFake_SetConnectResult(-1, EINPROGRESS);

    CHECK_TRUE(Open());

    LONGS_EQUAL(1U, LwipSocketsFake_GetSockOptCallCount());
    LONGS_EQUAL(TEST_DESCRIPTOR, LwipSocketsFake_LastGetSockOptSocket());
    LONGS_EQUAL(SOL_SOCKET, LwipSocketsFake_LastGetSockOptLevel());
    LONGS_EQUAL(SO_ERROR, LwipSocketsFake_LastGetSockOptName());
}

TEST(SolidSyslogLwipSocketTcpStream, AConnectThatFailedAfterTheSynIsRefusedAndReported)
{
    LwipSocketsFake_SetConnectResult(-1, EINPROGRESS);
    LwipSocketsFake_SetSocketError(ECONNREFUSED);

    CHECK_FALSE(Open());

    CHECK_ERROR_REPORTED_ONCE(
        SOLIDSYSLOG_STREAM_CONNECT_REMOTE_SEVERITY,
        &SolidSyslogLwipSocketTcpStreamErrorSource,
        SOLIDSYSLOG_CAT_STREAM_CONNECT_FAILED,
        SOLIDSYSLOG_TCP_STREAM_ERROR_CONNECT_REFUSED
    );
}

TEST(SolidSyslogLwipSocketTcpStream, AConnectBudgetThatExpiresIsReportedAsATimeout)
{
    LwipSocketsFake_SetConnectResult(-1, EINPROGRESS);
    LwipSocketsFake_SetSelectResult(0);

    CHECK_FALSE(Open());

    CHECK_ERROR_REPORTED_ONCE(
        SOLIDSYSLOG_STREAM_CONNECT_REMOTE_SEVERITY,
        &SolidSyslogLwipSocketTcpStreamErrorSource,
        SOLIDSYSLOG_CAT_STREAM_CONNECT_FAILED,
        SOLIDSYSLOG_TCP_STREAM_ERROR_CONNECT_TIMED_OUT
    );
}

TEST(SolidSyslogLwipSocketTcpStream, AWaitTheStackEndsInTheExceptionSetIsRefusedNotTimedOut)
{
    LwipSocketsFake_SetConnectResult(-1, EINPROGRESS);
    LwipSocketsFake_SetSelectSignalsException();

    CHECK_FALSE(Open());

    CHECK_ERROR_REPORTED_ONCE(
        SOLIDSYSLOG_STREAM_CONNECT_REMOTE_SEVERITY,
        &SolidSyslogLwipSocketTcpStreamErrorSource,
        SOLIDSYSLOG_CAT_STREAM_CONNECT_FAILED,
        SOLIDSYSLOG_TCP_STREAM_ERROR_CONNECT_REFUSED
    );
}

TEST(SolidSyslogLwipSocketTcpStream, ADestinationThatRefusesImmediatelyIsReportedAsRefused)
{
    LwipSocketsFake_SetConnectResult(-1, ECONNREFUSED);

    CHECK_FALSE(Open());

    LONGS_EQUAL(0U, LwipSocketsFake_SelectCallCount());
    CHECK_ERROR_REPORTED_ONCE(
        SOLIDSYSLOG_STREAM_CONNECT_REMOTE_SEVERITY,
        &SolidSyslogLwipSocketTcpStreamErrorSource,
        SOLIDSYSLOG_CAT_STREAM_CONNECT_FAILED,
        SOLIDSYSLOG_TCP_STREAM_ERROR_CONNECT_REFUSED
    );
}

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

