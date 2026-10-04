#include "TestUtils.h"
#include "CppUTest/TestHarness.h"

using namespace CososoTesting;

#include "vxWorks.h"

#include <errno.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>

#include "ConfigLockFake.h"
#include "ErrorHandlerFake.h"
#include "SolidSyslogAddress.h"
#include "SolidSyslogError.h"
#include "SolidSyslogStreamCategories.h"
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
