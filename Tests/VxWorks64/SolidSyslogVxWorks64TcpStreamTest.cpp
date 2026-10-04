#include "TestUtils.h"
#include "CppUTest/TestHarness.h"

using namespace CososoTesting;

#include "vxWorks.h"

#include <errno.h>
#include <limits.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>

#include "ConfigLockFake.h"
#include "ErrorHandlerFake.h"
#include "SolidSyslogAddress.h"
#include "SolidSyslogError.h"
#include "SolidSyslogErrorCategory.h"
#include "SolidSyslogStreamCategories.h"
#include "SolidSyslogNullStream.h"
#include "SolidSyslogStream.h"
#include "SolidSyslogStreamDefinition.h"
#include "SolidSyslogTunables.h"
#include "SolidSyslogVxWorks64Address.h"
#include "SolidSyslogVxWorks64AddressPrivate.h"
#include "SolidSyslogVxWorks64TcpStream.h"
#include "SolidSyslogVxWorks64TcpStreamErrors.h"
#include "VxWorks64NetFake.h"

namespace
{
uint32_t FakeGetConnectTimeoutMs_ReturnValue = 0U;
void* FakeGetConnectTimeoutMs_LastContext = nullptr;

extern "C" uint32_t FakeGetConnectTimeoutMs(void* context)
{
    FakeGetConnectTimeoutMs_LastContext = context;
    return FakeGetConnectTimeoutMs_ReturnValue;
}
} // namespace

static const char TEST_RECORD[] = "hello";

// clang-format off
TEST_GROUP(SolidSyslogVxWorks64TcpStream)
{
    struct SolidSyslogStream* stream   = nullptr;
    struct SolidSyslogAddress* address = nullptr;

    void setup() override
    {
        VxWorks64NetFake_Reset();
        FakeGetConnectTimeoutMs_ReturnValue = 0U;
        FakeGetConnectTimeoutMs_LastContext = nullptr;
        stream  = SolidSyslogVxWorks64TcpStream_Create(nullptr);
        address = SolidSyslogVxWorks64Address_Create();
        struct sockaddr_in* sin = SolidSyslogVxWorks64Address_AsSockaddrIn(address);
        sin->sin_family         = AF_INET;
        sin->sin_port           = htons(514U);
        sin->sin_addr.s_addr    = htonl(0x0A00000AU);
    }

    void teardown() override
    {
        SolidSyslogVxWorks64Address_Destroy(address);
        SolidSyslogVxWorks64TcpStream_Destroy(stream);
    }

    void RecreateWithTimeoutGetter(void* context)
    {
        SolidSyslogVxWorks64TcpStream_Destroy(stream);
        struct SolidSyslogVxWorks64TcpStreamConfig config = {};
        config.GetConnectTimeoutMs   = FakeGetConnectTimeoutMs;
        config.ConnectTimeoutContext = context;
        stream = SolidSyslogVxWorks64TcpStream_Create(&config);
    }

    [[nodiscard]] bool OpenAndSend() const
    {
        SolidSyslogStream_Open(stream, address);
        return SolidSyslogStream_Send(stream, TEST_RECORD, sizeof(TEST_RECORD) - 1U);
    }

    SolidSyslogSsize OpenAndRead(char* buffer, size_t size) const
    {
        SolidSyslogStream_Open(stream, address);
        return SolidSyslogStream_Read(stream, buffer, size);
    }

    /* The connect-failure tests differ only in the error the connect reports,
     * so the arrange and act live here and each test is left as its assertion. */
    void OpenAfterConnectFails(int connectErrno) const
    {
        ErrorHandlerFake_Install(nullptr);
        VxWorks64NetFake_FailConnectWithErrno(connectErrno);
        SolidSyslogStream_Open(stream, address);
    }
};

// clang-format on

// Asserts Open reported exactly one connect failure. Source and category are the
// same for every path, so only the severity and the detail naming the path vary.
#define CHECK_CONNECT_FAILURE_REPORTED(severity, detail) \
    CHECK_ERROR_REPORTED_ONCE(                           \
        (severity),                                      \
        &SolidSyslogVxWorks64TcpStreamErrorSource,       \
        SOLIDSYSLOG_CAT_STREAM_CONNECT_FAILED,           \
        (detail)                                         \
    )

// Asserts Open reported exactly one refused option, which is a warning: the
// connection stands either way.
#define CHECK_OPTION_REFUSED_REPORTED()                        \
    CHECK_ERROR_REPORTED_ONCE(                                 \
        SOLIDSYSLOG_SEVERITY_WARNING,                          \
        &SolidSyslogVxWorks64TcpStreamErrorSource,             \
        SOLIDSYSLOG_CAT_STREAM_OPTION_REFUSED,                 \
        SOLIDSYSLOG_TCP_STREAM_ERROR_SOCKET_OPTION_REFUSED     \
    )

TEST(SolidSyslogVxWorks64TcpStream, CreateDestroyWorksWithoutCrashing)
{
}

TEST(SolidSyslogVxWorks64TcpStream, OpenTakesAnIpv4StreamSocket)
{
    SolidSyslogStream_Open(stream, address);
    CALLED_FAKE(VxWorks64NetFake_Socket, ONCE);
    LONGS_EQUAL(AF_INET, VxWorks64NetFake_LastSocketDomain());
    LONGS_EQUAL(SOCK_STREAM, VxWorks64NetFake_LastSocketType());
    LONGS_EQUAL(0, VxWorks64NetFake_LastSocketProtocol());
}

TEST(SolidSyslogVxWorks64TcpStream, OpenConnectsThatSocketToTheAddress)
{
    SolidSyslogStream_Open(stream, address);
    CALLED_FAKE(VxWorks64NetFake_ConnectWithTimeout, ONCE);
    LONGS_EQUAL(VxWorks64NetFake_SocketFd(), VxWorks64NetFake_LastConnectFd());
    POINTERS_EQUAL(SolidSyslogVxWorks64Address_AsSockaddrIn(address), VxWorks64NetFake_LastConnectAddress());
    LONGS_EQUAL(sizeof(struct sockaddr_in), VxWorks64NetFake_LastConnectAddressLength());
}

TEST(SolidSyslogVxWorks64TcpStream, OpenSucceedsWhenTheConnectSucceeds)
{
    CHECK_TRUE(SolidSyslogStream_Open(stream, address));
}

TEST(SolidSyslogVxWorks64TcpStream, OpenBoundsTheConnectByTheTunableWhenNoGetterIsConfigured)
{
    SolidSyslogStream_Open(stream, address);
    CHECK_TRUE(VxWorks64NetFake_LastConnectWasBounded());
    LONGS_EQUAL(SOLIDSYSLOG_TCP_CONNECT_TIMEOUT_MS / 1000U, VxWorks64NetFake_LastConnectTimeoutSeconds());
    LONGS_EQUAL((SOLIDSYSLOG_TCP_CONNECT_TIMEOUT_MS % 1000U) * 1000U, VxWorks64NetFake_LastConnectTimeoutMicroseconds());
}

TEST(SolidSyslogVxWorks64TcpStream, OpenBoundsTheConnectByTheConfiguredGetter)
{
    RecreateWithTimeoutGetter(nullptr);
    FakeGetConnectTimeoutMs_ReturnValue = 1500U;
    SolidSyslogStream_Open(stream, address);
    LONGS_EQUAL(1, VxWorks64NetFake_LastConnectTimeoutSeconds());
    LONGS_EQUAL(500000, VxWorks64NetFake_LastConnectTimeoutMicroseconds());
}

TEST(SolidSyslogVxWorks64TcpStream, OpenPassesTheConfiguredContextToTheGetter)
{
    int context = 0;
    RecreateWithTimeoutGetter(&context);
    SolidSyslogStream_Open(stream, address);
    POINTERS_EQUAL(&context, FakeGetConnectTimeoutMs_LastContext);
}

TEST(SolidSyslogVxWorks64TcpStream, OpenFailsWhenNoSocketCanBeTaken)
{
    VxWorks64NetFake_SetSocketFails(true);
    CHECK_FALSE(SolidSyslogStream_Open(stream, address));
}

TEST(SolidSyslogVxWorks64TcpStream, OpenReportsTheEndpointUnavailableWhenNoSocketCanBeTaken)
{
    ErrorHandlerFake_Install(nullptr);
    VxWorks64NetFake_SetSocketFails(true);
    SolidSyslogStream_Open(stream, address);
    CHECK_CONNECT_FAILURE_REPORTED(SOLIDSYSLOG_STREAM_CONNECT_LOCAL_SEVERITY, SOLIDSYSLOG_TCP_STREAM_ERROR_ENDPOINT_UNAVAILABLE);
}

TEST(SolidSyslogVxWorks64TcpStream, OpenClosesTheSocketWhenTheConnectFails)
{
    VxWorks64NetFake_FailConnectWithErrno(ECONNREFUSED);
    SolidSyslogStream_Open(stream, address);
    CALLED_FAKE(VxWorks64NetFake_Close, ONCE);
    LONGS_EQUAL(VxWorks64NetFake_SocketFd(), VxWorks64NetFake_LastClosedFd());
}

TEST(SolidSyslogVxWorks64TcpStream, OpenReportsTheConnectTimedOutWhenTheBoundExpires)
{
    OpenAfterConnectFails(EINPROGRESS);
    CHECK_CONNECT_FAILURE_REPORTED(SOLIDSYSLOG_STREAM_CONNECT_REMOTE_SEVERITY, SOLIDSYSLOG_TCP_STREAM_ERROR_CONNECT_TIMED_OUT);
}

TEST(SolidSyslogVxWorks64TcpStream, OpenReportsTheConnectRefusedWhenThePeerRefuses)
{
    OpenAfterConnectFails(ECONNREFUSED);
    CHECK_CONNECT_FAILURE_REPORTED(SOLIDSYSLOG_STREAM_CONNECT_REMOTE_SEVERITY, SOLIDSYSLOG_TCP_STREAM_ERROR_CONNECT_REFUSED);
}

TEST(SolidSyslogVxWorks64TcpStream, OpenReportsTheConnectRefusedWhenTheStackGivesUpOnThePeer)
{
    OpenAfterConnectFails(ETIMEDOUT);
    CHECK_CONNECT_FAILURE_REPORTED(SOLIDSYSLOG_STREAM_CONNECT_REMOTE_SEVERITY, SOLIDSYSLOG_TCP_STREAM_ERROR_CONNECT_REFUSED);
}

TEST(SolidSyslogVxWorks64TcpStream, OpenReportsTheConnectNotStartedWhenItFailsForALocalReason)
{
    OpenAfterConnectFails(ENETUNREACH);
    CHECK_CONNECT_FAILURE_REPORTED(SOLIDSYSLOG_STREAM_CONNECT_LOCAL_SEVERITY, SOLIDSYSLOG_TCP_STREAM_ERROR_CONNECT_NOT_STARTED);
}

TEST(SolidSyslogVxWorks64TcpStream, OpenSendsEachRecordWithoutWaitingToCoalesceIt)
{
    SolidSyslogStream_Open(stream, address);
    CHECK_TRUE(VxWorks64NetFake_SocketOptionWasSetTo(IPPROTO_TCP, TCP_NODELAY, 1));
}

TEST(SolidSyslogVxWorks64TcpStream, OpenAsksTheStackToProbeAnIdlePeer)
{
    SolidSyslogStream_Open(stream, address);
    CHECK_TRUE(VxWorks64NetFake_SocketOptionWasSetTo(SOL_SOCKET, SO_KEEPALIVE, 1));
}

TEST(SolidSyslogVxWorks64TcpStream, OpenSetsNoOptionsOnAConnectionThatFailed)
{
    VxWorks64NetFake_FailConnectWithErrno(ECONNREFUSED);
    SolidSyslogStream_Open(stream, address);
    CALLED_FAKE(VxWorks64NetFake_Setsockopt, NEVER);
}

TEST(SolidSyslogVxWorks64TcpStream, OpenWarnsWhenTheStackRefusesAnOption)
{
    ErrorHandlerFake_Install(nullptr);
    VxWorks64NetFake_RefuseSocketOption(IPPROTO_TCP, TCP_NODELAY);
    SolidSyslogStream_Open(stream, address);
    CHECK_OPTION_REFUSED_REPORTED();
}

TEST(SolidSyslogVxWorks64TcpStream, OpenWarnsWhenTheStackRefusesKeepalive)
{
    ErrorHandlerFake_Install(nullptr);
    VxWorks64NetFake_RefuseSocketOption(SOL_SOCKET, SO_KEEPALIVE);
    SolidSyslogStream_Open(stream, address);
    CHECK_OPTION_REFUSED_REPORTED();
}

TEST(SolidSyslogVxWorks64TcpStream, OpenStillSucceedsWhenTheStackRefusesAnOption)
{
    VxWorks64NetFake_RefuseSocketOption(IPPROTO_TCP, TCP_NODELAY);
    CHECK_TRUE(SolidSyslogStream_Open(stream, address));
}

TEST(SolidSyslogVxWorks64TcpStream, SendHandsTheWholeRecordToTheConnectedSocket)
{
    (void) OpenAndSend();
    CALLED_FAKE(VxWorks64NetFake_Send, ONCE);
    LONGS_EQUAL(VxWorks64NetFake_SocketFd(), VxWorks64NetFake_LastSendFd());
    POINTERS_EQUAL(TEST_RECORD, VxWorks64NetFake_LastSendBuf());
    LONGS_EQUAL(sizeof(TEST_RECORD) - 1U, VxWorks64NetFake_LastSendLen());
}

TEST(SolidSyslogVxWorks64TcpStream, SendNeverWaitsForRoomInTheStack)
{
    (void) OpenAndSend();
    LONGS_EQUAL(MSG_DONTWAIT, VxWorks64NetFake_LastSendFlags());
}

TEST(SolidSyslogVxWorks64TcpStream, SendSucceedsWhenTheStackTakesTheWholeRecord)
{
    CHECK_TRUE(OpenAndSend());
}

TEST(SolidSyslogVxWorks64TcpStream, SendFailsWhenTheStackTakesOnlyPartOfTheRecord)
{
    VxWorks64NetFake_LimitSendTo(2);
    CHECK_FALSE(OpenAndSend());
}

TEST(SolidSyslogVxWorks64TcpStream, SendFailsWhenTheStackRefusesTheRecord)
{
    VxWorks64NetFake_FailSendWithErrno(EPIPE);
    CHECK_FALSE(OpenAndSend());
}

TEST(SolidSyslogVxWorks64TcpStream, SendClosesTheConnectionWhenTheRecordDoesNotGoWhole)
{
    VxWorks64NetFake_LimitSendTo(2);
    (void) OpenAndSend();
    CALLED_FAKE(VxWorks64NetFake_Close, ONCE);
    LONGS_EQUAL(VxWorks64NetFake_SocketFd(), VxWorks64NetFake_LastClosedFd());
}

TEST(SolidSyslogVxWorks64TcpStream, SendFirstPeeksWithoutWaitingToSeeWhetherThePeerHasClosed)
{
    (void) OpenAndSend();
    CALLED_FAKE(VxWorks64NetFake_Recv, ONCE);
    LONGS_EQUAL(VxWorks64NetFake_SocketFd(), VxWorks64NetFake_LastRecvFd());
    LONGS_EQUAL(1, VxWorks64NetFake_LastRecvLen());
    LONGS_EQUAL(MSG_PEEK | MSG_DONTWAIT, VxWorks64NetFake_LastRecvFlags());
}

TEST(SolidSyslogVxWorks64TcpStream, SendRefusesTheRecordOnceThePeerHasClosed)
{
    VxWorks64NetFake_RecvDelivers(nullptr, 0);
    CHECK_FALSE(OpenAndSend());
    CALLED_FAKE(VxWorks64NetFake_Send, NEVER);
}

TEST(SolidSyslogVxWorks64TcpStream, SendRefusesTheRecordWhenThePeerHasResetTheConnection)
{
    VxWorks64NetFake_FailRecvWithErrno(ECONNRESET);
    CHECK_FALSE(OpenAndSend());
    CALLED_FAKE(VxWorks64NetFake_Send, NEVER);
}

TEST(SolidSyslogVxWorks64TcpStream, SendGoesAheadWhenThePeerHasSentSomethingUnread)
{
    VxWorks64NetFake_RecvDelivers("x", 1);
    CHECK_TRUE(OpenAndSend());
}

TEST(SolidSyslogVxWorks64TcpStream, SendGoesAheadWhenTheStackSaysTryAgainForNothingWaiting)
{
    VxWorks64NetFake_FailRecvWithErrno(EAGAIN);
    CHECK_TRUE(OpenAndSend());
}

TEST(SolidSyslogVxWorks64TcpStream, SendClosesTheConnectionOnceThePeerHasClosed)
{
    VxWorks64NetFake_RecvDelivers(nullptr, 0);
    (void) OpenAndSend();
    CALLED_FAKE(VxWorks64NetFake_Close, ONCE);
}

TEST(SolidSyslogVxWorks64TcpStream, SendRefusesARecordLongerThanTheStackCanBeToldOf)
{
    SolidSyslogStream_Open(stream, address);
    CHECK_FALSE(SolidSyslogStream_Send(stream, TEST_RECORD, (size_t) INT_MAX + 1U));
    CALLED_FAKE(VxWorks64NetFake_Send, NEVER);
}

TEST(SolidSyslogVxWorks64TcpStream, ReadAsksTheConnectedSocketForWhatIsWaiting)
{
    char buffer[16];
    (void) OpenAndRead(buffer, sizeof(buffer));
    CALLED_FAKE(VxWorks64NetFake_Recv, ONCE);
    LONGS_EQUAL(VxWorks64NetFake_SocketFd(), VxWorks64NetFake_LastRecvFd());
    POINTERS_EQUAL(buffer, VxWorks64NetFake_LastRecvBuf());
    LONGS_EQUAL(sizeof(buffer), VxWorks64NetFake_LastRecvLen());
}

TEST(SolidSyslogVxWorks64TcpStream, ReadNeverWaitsForData)
{
    char buffer[16];
    (void) OpenAndRead(buffer, sizeof(buffer));
    LONGS_EQUAL(MSG_DONTWAIT, VxWorks64NetFake_LastRecvFlags());
}

TEST(SolidSyslogVxWorks64TcpStream, ReadReturnsHowManyBytesArrived)
{
    char buffer[16];
    VxWorks64NetFake_RecvDelivers("abc", 3);
    LONGS_EQUAL(3, OpenAndRead(buffer, sizeof(buffer)));
}

TEST(SolidSyslogVxWorks64TcpStream, ReadReportsATeardownOnceThePeerHasClosed)
{
    char buffer[16];
    VxWorks64NetFake_RecvDelivers(nullptr, 0);
    LONGS_EQUAL(-1, OpenAndRead(buffer, sizeof(buffer)));
}

TEST(SolidSyslogVxWorks64TcpStream, ReadReportsATeardownWhenTheConnectionFails)
{
    char buffer[16];
    VxWorks64NetFake_FailRecvWithErrno(ECONNRESET);
    LONGS_EQUAL(-1, OpenAndRead(buffer, sizeof(buffer)));
}

TEST(SolidSyslogVxWorks64TcpStream, ReadReturnsZeroWhenNothingIsWaiting)
{
    char buffer[16];
    VxWorks64NetFake_FailRecvWithErrno(EWOULDBLOCK);
    LONGS_EQUAL(0, OpenAndRead(buffer, sizeof(buffer)));
}

TEST(SolidSyslogVxWorks64TcpStream, ReadReturnsZeroWhenTheStackSaysTryAgain)
{
    char buffer[16];
    VxWorks64NetFake_FailRecvWithErrno(EAGAIN);
    LONGS_EQUAL(0, OpenAndRead(buffer, sizeof(buffer)));
}

TEST(SolidSyslogVxWorks64TcpStream, ReadClosesTheConnectionWhenItReportsATeardown)
{
    char buffer[16];
    VxWorks64NetFake_RecvDelivers(nullptr, 0);
    (void) OpenAndRead(buffer, sizeof(buffer));
    CALLED_FAKE(VxWorks64NetFake_Close, ONCE);
    LONGS_EQUAL(VxWorks64NetFake_SocketFd(), VxWorks64NetFake_LastClosedFd());
}

TEST(SolidSyslogVxWorks64TcpStream, ReadOffersTheStackNoMoreThanItCanBeToldOf)
{
    char buffer[16];
    (void) OpenAndRead(buffer, (size_t) INT_MAX + 1U);
    LONGS_EQUAL(INT_MAX, VxWorks64NetFake_LastRecvLen());
}

TEST(SolidSyslogVxWorks64TcpStream, CloseClosesTheConnectedSocket)
{
    SolidSyslogStream_Open(stream, address);
    SolidSyslogStream_Close(stream);
    CALLED_FAKE(VxWorks64NetFake_Close, ONCE);
    LONGS_EQUAL(VxWorks64NetFake_SocketFd(), VxWorks64NetFake_LastClosedFd());
}

TEST(SolidSyslogVxWorks64TcpStream, CloseTwiceClosesTheSocketOnce)
{
    SolidSyslogStream_Open(stream, address);
    SolidSyslogStream_Close(stream);
    SolidSyslogStream_Close(stream);
    CALLED_FAKE(VxWorks64NetFake_Close, ONCE);
}

TEST(SolidSyslogVxWorks64TcpStream, CloseOnAStreamThatNeverOpenedClosesNothing)
{
    SolidSyslogStream_Close(stream);
    CALLED_FAKE(VxWorks64NetFake_Close, NEVER);
}

TEST(SolidSyslogVxWorks64TcpStream, OpenOnAnOpenStreamClosesTheSocketItHeld)
{
    SolidSyslogStream_Open(stream, address);
    SolidSyslogStream_Open(stream, address);
    CALLED_FAKE(VxWorks64NetFake_Close, ONCE);
}

TEST(SolidSyslogVxWorks64TcpStream, VersionStaysZeroBecauseNothingAboutTheStreamChangesAtRuntime)
{
    LONGS_EQUAL(0U, SolidSyslogStream_Version(stream));
}

TEST(SolidSyslogVxWorks64TcpStream, DestroyClosesAnOpenSocket)
{
    SolidSyslogStream_Open(stream, address);
    SolidSyslogVxWorks64TcpStream_Destroy(stream);
    stream = nullptr;
    CALLED_FAKE(VxWorks64NetFake_Close, ONCE);
}

TEST(SolidSyslogVxWorks64TcpStream, SendAfterDestroyTouchesNoSocket)
{
    SolidSyslogStream_Open(stream, address);
    struct SolidSyslogStream* stale = stream;
    SolidSyslogVxWorks64TcpStream_Destroy(stream);
    stream = nullptr;
    (void) SolidSyslogStream_Send(stale, TEST_RECORD, sizeof(TEST_RECORD) - 1U);
    CALLED_FAKE(VxWorks64NetFake_Recv, NEVER);
    CALLED_FAKE(VxWorks64NetFake_Send, NEVER);
}

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
TEST_GROUP(SolidSyslogVxWorks64TcpStreamPool)
{
    struct SolidSyslogStream* pooled[SOLIDSYSLOG_TCP_STREAM_POOL_SIZE] = {};
    struct SolidSyslogStream* overflow                                 = nullptr;

    void teardown() override
    {
        for (auto* handle : pooled)
        {
            if (handle != nullptr)
            {
                SolidSyslogVxWorks64TcpStream_Destroy(handle);
            }
        }
        if (overflow != nullptr)
        {
            SolidSyslogVxWorks64TcpStream_Destroy(overflow);
        }
        ConfigLockFake_Uninstall();
    }

    void FillPool()
    {
        for (auto*& slot : pooled)
        {
            slot = SolidSyslogVxWorks64TcpStream_Create(nullptr);
        }
    }
};

// clang-format on

TEST(SolidSyslogVxWorks64TcpStreamPool, FillingThePoolThenOverflowingItReturnsADistinctFallback)
{
    FillPool();
    overflow = SolidSyslogVxWorks64TcpStream_Create(nullptr);
    CHECK_IS_FALLBACK(overflow, pooled);
}

TEST(SolidSyslogVxWorks64TcpStreamPool, AnExhaustedPoolReportsItself)
{
    ErrorHandlerFake_Install(nullptr);
    FillPool();
    overflow = SolidSyslogVxWorks64TcpStream_Create(nullptr);
    CHECK_ERROR_REPORTED_ONCE(
        SOLIDSYSLOG_POOL_EXHAUSTED_SEVERITY,
        &SolidSyslogVxWorks64TcpStreamErrorSource,
        SOLIDSYSLOG_CAT_POOL_EXHAUSTED,
        SOLIDSYSLOG_TCP_STREAM_ERROR_POOL_EXHAUSTED
    );
}

TEST(SolidSyslogVxWorks64TcpStreamPool, DestroyingAHandleThePoolDoesNotOwnReportsIt)
{
    ErrorHandlerFake_Install(nullptr);
    struct SolidSyslogStream stranger = {};
    SolidSyslogVxWorks64TcpStream_Destroy(&stranger);
    CHECK_ERROR_REPORTED_ONCE(
        SOLIDSYSLOG_UNKNOWN_DESTROY_SEVERITY,
        &SolidSyslogVxWorks64TcpStreamErrorSource,
        SOLIDSYSLOG_CAT_UNKNOWN_DESTROY,
        SOLIDSYSLOG_TCP_STREAM_ERROR_UNKNOWN_DESTROY
    );
}

TEST(SolidSyslogVxWorks64TcpStreamPool, DestroyingAHandleTwiceReportsTheSecond)
{
    pooled[0] = SolidSyslogVxWorks64TcpStream_Create(nullptr);
    SolidSyslogVxWorks64TcpStream_Destroy(pooled[0]);
    ErrorHandlerFake_Install(nullptr);
    SolidSyslogVxWorks64TcpStream_Destroy(pooled[0]);
    pooled[0] = nullptr;
    CHECK_ERROR_REPORTED_ONCE(
        SOLIDSYSLOG_UNKNOWN_DESTROY_SEVERITY,
        &SolidSyslogVxWorks64TcpStreamErrorSource,
        SOLIDSYSLOG_CAT_UNKNOWN_DESTROY,
        SOLIDSYSLOG_TCP_STREAM_ERROR_UNKNOWN_DESTROY
    );
}

TEST(SolidSyslogVxWorks64TcpStreamPool, TheFallbackIsTheSharedNullStream)
{
    FillPool();
    overflow = SolidSyslogVxWorks64TcpStream_Create(nullptr);
    POINTERS_EQUAL(SolidSyslogNullStream_Get(), overflow);
}

TEST(SolidSyslogVxWorks64TcpStreamPool, CreateTakesTheConfigLockOnceForTheFirstFreeSlot)
{
    ConfigLockFake_Install();
    pooled[0] = SolidSyslogVxWorks64TcpStream_Create(nullptr);
    CALLED_FAKE(ConfigLockFake_Lock, ONCE);
    CALLED_FAKE(ConfigLockFake_Unlock, ONCE);
}

TEST(SolidSyslogVxWorks64TcpStreamPool, DestroyOfAPooledHandleTakesTheConfigLockOnce)
{
    pooled[0] = SolidSyslogVxWorks64TcpStream_Create(nullptr);
    ConfigLockFake_Install();
    SolidSyslogVxWorks64TcpStream_Destroy(pooled[0]);
    pooled[0] = nullptr;
    CALLED_FAKE(ConfigLockFake_Lock, ONCE);
    CALLED_FAKE(ConfigLockFake_Unlock, ONCE);
}

TEST(SolidSyslogVxWorks64TcpStreamPool, DestroyOfAHandleThePoolDoesNotOwnTakesNoLock)
{
    ConfigLockFake_Install();
    struct SolidSyslogStream stranger = {};
    SolidSyslogVxWorks64TcpStream_Destroy(&stranger);
    CALLED_FAKE(ConfigLockFake_Lock, NEVER);
}
