#include "CppUTest/TestHarness.h"
#include "SolidSyslogAddress.h"
#include "SolidSyslogStream.h"
#include "SolidSyslogTransport.h"
#include "SolidSyslogWinsockTcpStream.h"
#include "SolidSyslogWinsockTcpStreamInternal.h"
#include "WinsockFake.h"
#include <cstdint>
#include <winsock2.h>
#include <ws2tcpip.h>

// clang-format off
static const char* const TEST_MESSAGE     = "hello";
static const size_t      TEST_MESSAGE_LEN = 5;
static const char* const TEST_ADDRESS     = "127.0.0.1";
static const int         TEST_PORT        = 514;

TEST_GROUP(SolidSyslogWinsockTcpStream)
{
    SolidSyslogWinsockTcpStreamStorage streamStorage{};
    // cppcheck-suppress unreadVariable -- used across TEST_GROUP methods; cppcheck does not model CppUTest macros
    struct SolidSyslogStream* stream = nullptr;
    SolidSyslogAddressStorage addrStorage{};
    // cppcheck-suppress unreadVariable -- assigned in setup; cppcheck does not model CppUTest macros
    struct SolidSyslogAddress* addr = nullptr;

    void setup() override
    {
        WinsockFake_Reset();
        UT_PTR_SET(WinsockTcpStream_socket,           WinsockFake_socket);
        UT_PTR_SET(WinsockTcpStream_connect,          WinsockFake_connect);
        UT_PTR_SET(WinsockTcpStream_send,             WinsockFake_send);
        UT_PTR_SET(WinsockTcpStream_recv,             WinsockFake_recv);
        UT_PTR_SET(WinsockTcpStream_setsockopt,       WinsockFake_setsockopt);
        UT_PTR_SET(WinsockTcpStream_getsockopt,       WinsockFake_getsockopt);
        UT_PTR_SET(WinsockTcpStream_closesocket,      WinsockFake_closesocket);
        UT_PTR_SET(WinsockTcpStream_ioctlsocket,      WinsockFake_ioctlsocket);
        UT_PTR_SET(WinsockTcpStream_select,           WinsockFake_select);
        UT_PTR_SET(WinsockTcpStream_WSAGetLastError,  WinsockFake_WSAGetLastError);
        // cppcheck-suppress unreadVariable -- used in tests; cppcheck does not model CppUTest macros
        stream = SolidSyslogWinsockTcpStream_Create(&streamStorage);
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast) -- char-type aliasing, legal and necessary
        auto* bytes = reinterpret_cast<std::uint8_t*>(&addrStorage);
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast) -- reinterpret to platform layout, storage is intptr_t-aligned
        auto* sin       = reinterpret_cast<struct sockaddr_in*>(bytes);
        sin->sin_family = AF_INET;
        sin->sin_port   = htons(TEST_PORT);
        inet_pton(AF_INET, TEST_ADDRESS, &sin->sin_addr);
        // cppcheck-suppress unreadVariable -- used across TEST_GROUP methods; cppcheck does not model CppUTest macros
        addr = SolidSyslogAddress_FromStorage(&addrStorage);
    }

    void teardown() override
    {
        SolidSyslogWinsockTcpStream_Destroy(stream);
    }

    [[nodiscard]] SolidSyslogSsize Read16ByteBuffer() const
    {
        char buf[16];
        return SolidSyslogStream_Read(stream, buf, sizeof(buf));
    }
};

// clang-format on

// NOLINTBEGIN(cppcoreguidelines-macro-usage,cppcoreguidelines-avoid-do-while)
#define CHECK_SOCKET_CLOSED_ONCE()                                   \
    do                                                               \
    {                                                                \
        LONGS_EQUAL(1, WinsockFake_CloseCallCount());                \
        CHECK(WinsockFake_SocketFd() == WinsockFake_LastClosedFd()); \
    } while (0)

// NOLINTEND(cppcoreguidelines-macro-usage,cppcoreguidelines-avoid-do-while)

TEST(SolidSyslogWinsockTcpStream, CreateDestroyWorksWithoutCrashing)
{
}

TEST(SolidSyslogWinsockTcpStream, CreateReturnsHandleInsideCallerSuppliedStorage)
{
    SolidSyslogWinsockTcpStreamStorage storage{};
    struct SolidSyslogStream*          localStream = SolidSyslogWinsockTcpStream_Create(&storage);
    POINTERS_EQUAL(&storage, localStream);
    SolidSyslogWinsockTcpStream_Destroy(localStream);
}

TEST(SolidSyslogWinsockTcpStream, OpenCallsSocketOnce)
{
    SolidSyslogStream_Open(stream, addr);
    LONGS_EQUAL(1, WinsockFake_SocketCallCount());
}

TEST(SolidSyslogWinsockTcpStream, OpenCallsSocketWithAF_INET)
{
    SolidSyslogStream_Open(stream, addr);
    LONGS_EQUAL(AF_INET, WinsockFake_SocketDomain());
}

TEST(SolidSyslogWinsockTcpStream, OpenCallsSocketWithSOCK_STREAM)
{
    SolidSyslogStream_Open(stream, addr);
    LONGS_EQUAL(SOCK_STREAM, WinsockFake_SocketType());
}

TEST(SolidSyslogWinsockTcpStream, OpenEnablesTcpNoDelay)
{
    SolidSyslogStream_Open(stream, addr);
    CHECK_TRUE(WinsockFake_HasSetSockOpt(IPPROTO_TCP, TCP_NODELAY));
}

TEST(SolidSyslogWinsockTcpStream, OpenEnablesSoKeepalive)
{
    SolidSyslogStream_Open(stream, addr);
    CHECK_TRUE(WinsockFake_HasSetSockOpt(SOL_SOCKET, SO_KEEPALIVE));
    LONGS_EQUAL(1, WinsockFake_LastSetSockOptValue(SOL_SOCKET, SO_KEEPALIVE));
}

TEST(SolidSyslogWinsockTcpStream, OpenSetsTcpKeepIdleTo45Seconds)
{
    SolidSyslogStream_Open(stream, addr);
    CHECK_TRUE(WinsockFake_HasSetSockOpt(IPPROTO_TCP, TCP_KEEPIDLE));
    LONGS_EQUAL(45, WinsockFake_LastSetSockOptValue(IPPROTO_TCP, TCP_KEEPIDLE));
}

TEST(SolidSyslogWinsockTcpStream, OpenSetsTcpKeepIntvlTo10Seconds)
{
    SolidSyslogStream_Open(stream, addr);
    CHECK_TRUE(WinsockFake_HasSetSockOpt(IPPROTO_TCP, TCP_KEEPINTVL));
    LONGS_EQUAL(10, WinsockFake_LastSetSockOptValue(IPPROTO_TCP, TCP_KEEPINTVL));
}

TEST(SolidSyslogWinsockTcpStream, OpenSetsTcpKeepCntTo4)
{
    SolidSyslogStream_Open(stream, addr);
    CHECK_TRUE(WinsockFake_HasSetSockOpt(IPPROTO_TCP, TCP_KEEPCNT));
    LONGS_EQUAL(4, WinsockFake_LastSetSockOptValue(IPPROTO_TCP, TCP_KEEPCNT));
}

TEST(SolidSyslogWinsockTcpStream, OpenCallsConnectWithSocketFd)
{
    SolidSyslogStream_Open(stream, addr);
    LONGS_EQUAL(1, WinsockFake_ConnectCallCount());
    CHECK(WinsockFake_SocketFd() == WinsockFake_LastConnectFd());
}

TEST(SolidSyslogWinsockTcpStream, OpenCallsConnectWithProvidedAddress)
{
    SolidSyslogStream_Open(stream, addr);
    LONGS_EQUAL(TEST_PORT, WinsockFake_LastConnectPort());
    STRCMP_EQUAL(TEST_ADDRESS, WinsockFake_LastConnectAddrAsString());
}

TEST(SolidSyslogWinsockTcpStream, OpenReturnsTrueOnSuccess)
{
    CHECK_TRUE(SolidSyslogStream_Open(stream, addr));
}

TEST(SolidSyslogWinsockTcpStream, OpenReturnsFalseOnConnectFailure)
{
    WinsockFake_SetConnectFails(true);
    CHECK_FALSE(SolidSyslogStream_Open(stream, addr));
}

TEST(SolidSyslogWinsockTcpStream, OpenReturnsFalseWhenSocketFails)
{
    WinsockFake_SetSocketFails(true);
    CHECK_FALSE(SolidSyslogStream_Open(stream, addr));
}

TEST(SolidSyslogWinsockTcpStream, OpenSkipsConnectAndSetsockoptWhenSocketFails)
{
    WinsockFake_SetSocketFails(true);
    SolidSyslogStream_Open(stream, addr);
    LONGS_EQUAL(0, WinsockFake_ConnectCallCount());
    LONGS_EQUAL(0, WinsockFake_SetSockOptCallCount());
}

TEST(SolidSyslogWinsockTcpStream, OpenClosesSocketOnConnectFailure)
{
    WinsockFake_SetConnectFails(true);
    SolidSyslogStream_Open(stream, addr);
    LONGS_EQUAL(1, WinsockFake_CloseCallCount());
    CHECK(WinsockFake_SocketFd() == WinsockFake_LastClosedFd());
}

/* ----------------------------------------------------------------------
 * Non-blocking connect with bounded wait — the production path that
 * keeps the BlockStore service thread's drain rate from being throttled
 * by Windows' default ~2 s connect()-retry on a refused loopback port.
 * -------------------------------------------------------------------- */

TEST(SolidSyslogWinsockTcpStream, OpenSetsNonBlockingMode)
{
    SolidSyslogStream_Open(stream, addr);
    /* Single FIONBIO call: non-blocking on (1). The socket stays non-blocking
       so Send/Read are also fail-fast — no SO_SNDTIMEO needed. */
    LONGS_EQUAL(1, WinsockFake_FionbioCallCount());
    LONGS_EQUAL(1, WinsockFake_FionbioArgAt(0));
}

TEST(SolidSyslogWinsockTcpStream, OpenSkipsSelectWhenConnectReturnsImmediately)
{
    /* Default fake connect returns 0 (immediate success); select must not be
       reached because connect short-circuits the wait. */
    SolidSyslogStream_Open(stream, addr);
    LONGS_EQUAL(0, WinsockFake_SelectCallCount());
}

TEST(SolidSyslogWinsockTcpStream, OpenInvokesSelectWhenConnectReturnsWouldBlock)
{
    WinsockFake_SetConnectFailsWithLastError(WSAEWOULDBLOCK);
    SolidSyslogStream_Open(stream, addr);
    LONGS_EQUAL(1, WinsockFake_SelectCallCount());
}

TEST(SolidSyslogWinsockTcpStream, OpenPassesBoundedConnectTimeoutToSelect)
{
    WinsockFake_SetConnectFailsWithLastError(WSAEWOULDBLOCK);
    SolidSyslogStream_Open(stream, addr);
    /* CONNECT_TIMEOUT_MILLISECONDS = 200 → 0 s + 200 000 µs. Bounding the
       wait is the whole point of the non-blocking-connect rewrite. */
    LONGS_EQUAL(0, WinsockFake_LastSelectTimeoutSec());
    LONGS_EQUAL(200000, WinsockFake_LastSelectTimeoutUsec());
}

TEST(SolidSyslogWinsockTcpStream, OpenSucceedsWhenSelectReportsWritableAndZeroSO_ERROR)
{
    WinsockFake_SetConnectFailsWithLastError(WSAEWOULDBLOCK);
    WinsockFake_SetSelectWritable(true);
    WinsockFake_SetSoError(0);
    CHECK_TRUE(SolidSyslogStream_Open(stream, addr));
}

TEST(SolidSyslogWinsockTcpStream, OpenFailsWhenSelectReportsTimeout)
{
    WinsockFake_SetConnectFailsWithLastError(WSAEWOULDBLOCK);
    WinsockFake_SetSelectWritable(false);
    WinsockFake_SetSelectReturn(0);
    CHECK_FALSE(SolidSyslogStream_Open(stream, addr));
}

TEST(SolidSyslogWinsockTcpStream, OpenClosesSocketOnSelectTimeout)
{
    WinsockFake_SetConnectFailsWithLastError(WSAEWOULDBLOCK);
    WinsockFake_SetSelectWritable(false);
    WinsockFake_SetSelectReturn(0);
    SolidSyslogStream_Open(stream, addr);
    LONGS_EQUAL(1, WinsockFake_CloseCallCount());
    CHECK(WinsockFake_SocketFd() == WinsockFake_LastClosedFd());
}

TEST(SolidSyslogWinsockTcpStream, OpenFailsWhenSelectFlagsErrorOnFd)
{
    WinsockFake_SetConnectFailsWithLastError(WSAEWOULDBLOCK);
    WinsockFake_SetSelectWritable(false);
    WinsockFake_SetSelectError(true);
    CHECK_FALSE(SolidSyslogStream_Open(stream, addr));
}

TEST(SolidSyslogWinsockTcpStream, OpenFailsWhenDeferredSO_ERRORIsNonZero)
{
    /* select reports writable, but SO_ERROR on the socket reveals the
       deferred connect failure (e.g. WSAECONNREFUSED). */
    WinsockFake_SetConnectFailsWithLastError(WSAEWOULDBLOCK);
    WinsockFake_SetSelectWritable(true);
    WinsockFake_SetSoError(WSAECONNREFUSED);
    CHECK_FALSE(SolidSyslogStream_Open(stream, addr));
}

TEST(SolidSyslogWinsockTcpStream, OpenReadsSO_ERRORAfterSelectWritable)
{
    WinsockFake_SetConnectFailsWithLastError(WSAEWOULDBLOCK);
    SolidSyslogStream_Open(stream, addr);
    LONGS_EQUAL(SOL_SOCKET, WinsockFake_LastGetSockOptLevel());
    LONGS_EQUAL(SO_ERROR, WinsockFake_LastGetSockOptOptname());
}

TEST(SolidSyslogWinsockTcpStream, OpenFailsWhenIoctlsocketFails)
{
    /* If the kernel refuses to put the socket into non-blocking mode the
       caller cannot bound the connect wait — fail fast rather than fall
       back to blocking-connect's ~2 s retry behaviour. */
    WinsockFake_SetIoctlSocketFails(true);
    CHECK_FALSE(SolidSyslogStream_Open(stream, addr));
}

TEST(SolidSyslogWinsockTcpStream, OpenFailsWhenConnectFailsImmediatelyWithRefused)
{
    /* Non-WSAEWOULDBLOCK errors (e.g. WSAECONNREFUSED) are immediate failures —
       no select wait, no SO_ERROR check, just fail fast. */
    WinsockFake_SetConnectFailsWithLastError(WSAECONNREFUSED);
    CHECK_FALSE(SolidSyslogStream_Open(stream, addr));
    LONGS_EQUAL(0, WinsockFake_SelectCallCount());
}

TEST(SolidSyslogWinsockTcpStream, SendCallsSendOnce)
{
    SolidSyslogStream_Open(stream, addr);
    SolidSyslogStream_Send(stream, TEST_MESSAGE, TEST_MESSAGE_LEN);
    LONGS_EQUAL(1, WinsockFake_SendCallCount());
}

TEST(SolidSyslogWinsockTcpStream, SendPassesBuffer)
{
    SolidSyslogStream_Open(stream, addr);
    SolidSyslogStream_Send(stream, TEST_MESSAGE, TEST_MESSAGE_LEN);
    STRCMP_EQUAL(TEST_MESSAGE, WinsockFake_SendBufAsString(0));
}

TEST(SolidSyslogWinsockTcpStream, SendPassesLength)
{
    SolidSyslogStream_Open(stream, addr);
    SolidSyslogStream_Send(stream, TEST_MESSAGE, TEST_MESSAGE_LEN);
    LONGS_EQUAL(TEST_MESSAGE_LEN, WinsockFake_SendLen(0));
}

TEST(SolidSyslogWinsockTcpStream, SendPassesZeroFlags)
{
    SolidSyslogStream_Open(stream, addr);
    SolidSyslogStream_Send(stream, TEST_MESSAGE, TEST_MESSAGE_LEN);
    LONGS_EQUAL(0, WinsockFake_SendFlags(0));
}

TEST(SolidSyslogWinsockTcpStream, SendPassesSocketFd)
{
    SolidSyslogStream_Open(stream, addr);
    SolidSyslogStream_Send(stream, TEST_MESSAGE, TEST_MESSAGE_LEN);
    CHECK(WinsockFake_SocketFd() == WinsockFake_LastSendFd());
}

TEST(SolidSyslogWinsockTcpStream, SendReturnsTrueOnSuccess)
{
    SolidSyslogStream_Open(stream, addr);
    CHECK_TRUE(SolidSyslogStream_Send(stream, TEST_MESSAGE, TEST_MESSAGE_LEN));
}

TEST(SolidSyslogWinsockTcpStream, SendReturnsFalseOnSendFailure)
{
    SolidSyslogStream_Open(stream, addr);
    WinsockFake_SetSendFails(true);
    CHECK_FALSE(SolidSyslogStream_Send(stream, TEST_MESSAGE, TEST_MESSAGE_LEN));
}

TEST(SolidSyslogWinsockTcpStream, SendReturnsFalseOnShortWrite)
{
    SolidSyslogStream_Open(stream, addr);
    WinsockFake_SetSendReturn(3);
    CHECK_FALSE(SolidSyslogStream_Send(stream, TEST_MESSAGE, TEST_MESSAGE_LEN));
}

TEST(SolidSyslogWinsockTcpStream, SendDoesNotRetryAfterShortWrite)
{
    SolidSyslogStream_Open(stream, addr);
    WinsockFake_SetSendReturn(3);
    SolidSyslogStream_Send(stream, TEST_MESSAGE, TEST_MESSAGE_LEN);
    LONGS_EQUAL(1, WinsockFake_SendCallCount());
}

TEST(SolidSyslogWinsockTcpStream, SendClosesSocketOnFailure)
{
    SolidSyslogStream_Open(stream, addr);
    WinsockFake_SetSendFails(true);
    SolidSyslogStream_Send(stream, TEST_MESSAGE, TEST_MESSAGE_LEN);
    CHECK_SOCKET_CLOSED_ONCE();
}

TEST(SolidSyslogWinsockTcpStream, CloseCallsCloseOnce)
{
    SolidSyslogStream_Open(stream, addr);
    SolidSyslogStream_Close(stream);
    LONGS_EQUAL(1, WinsockFake_CloseCallCount());
}

TEST(SolidSyslogWinsockTcpStream, CloseCalledWithSocketFd)
{
    SolidSyslogStream_Open(stream, addr);
    SolidSyslogStream_Close(stream);
    CHECK(WinsockFake_SocketFd() == WinsockFake_LastClosedFd());
}

TEST(SolidSyslogWinsockTcpStream, CloseIsNoOpWhenNotOpen)
{
    SolidSyslogStream_Close(stream);
    LONGS_EQUAL(0, WinsockFake_CloseCallCount());
}

TEST(SolidSyslogWinsockTcpStream, ReadCallsRecvOnce)
{
    SolidSyslogStream_Open(stream, addr);
    char buf[16];
    SolidSyslogStream_Read(stream, buf, sizeof(buf));
    LONGS_EQUAL(1, WinsockFake_RecvCallCount());
}

TEST(SolidSyslogWinsockTcpStream, ReadPassesSocketFdToRecv)
{
    SolidSyslogStream_Open(stream, addr);
    char buf[16];
    SolidSyslogStream_Read(stream, buf, sizeof(buf));
    CHECK(WinsockFake_SocketFd() == WinsockFake_LastRecvFd());
}

TEST(SolidSyslogWinsockTcpStream, ReadPassesBufferToRecv)
{
    SolidSyslogStream_Open(stream, addr);
    char buf[16];
    SolidSyslogStream_Read(stream, buf, sizeof(buf));
    POINTERS_EQUAL(buf, WinsockFake_LastRecvBuf());
}

TEST(SolidSyslogWinsockTcpStream, ReadPassesLengthToRecv)
{
    SolidSyslogStream_Open(stream, addr);
    char buf[16];
    SolidSyslogStream_Read(stream, buf, sizeof(buf));
    LONGS_EQUAL(sizeof(buf), WinsockFake_LastRecvLen());
}

TEST(SolidSyslogWinsockTcpStream, ReadPassesZeroFlagsToRecv)
{
    SolidSyslogStream_Open(stream, addr);
    char buf[16];
    SolidSyslogStream_Read(stream, buf, sizeof(buf));
    LONGS_EQUAL(0, WinsockFake_LastRecvFlags());
}

TEST(SolidSyslogWinsockTcpStream, ReadReturnsRecvReturnValue)
{
    WinsockFake_SetRecvReturn(7);
    SolidSyslogStream_Open(stream, addr);
    char             buf[16];
    SolidSyslogSsize n = SolidSyslogStream_Read(stream, buf, sizeof(buf));
    LONGS_EQUAL(7, n);
}

TEST(SolidSyslogWinsockTcpStream, ReadReturnsZeroOnWouldBlock)
{
    SolidSyslogStream_Open(stream, addr);
    WinsockFake_FailNextRecvWithLastError(WSAEWOULDBLOCK);
    LONGS_EQUAL(0, Read16ByteBuffer());
}

TEST(SolidSyslogWinsockTcpStream, ReadReturnsNegativeOneOnEofAndClosesSocket)
{
    SolidSyslogStream_Open(stream, addr);
    WinsockFake_SetRecvReturn(0); /* EOF */
    LONGS_EQUAL(-1, Read16ByteBuffer());
    CHECK_SOCKET_CLOSED_ONCE();
}

TEST(SolidSyslogWinsockTcpStream, ReadReturnsNegativeOneOnErrorAndClosesSocket)
{
    SolidSyslogStream_Open(stream, addr);
    WinsockFake_FailNextRecvWithLastError(WSAECONNRESET);
    LONGS_EQUAL(-1, Read16ByteBuffer());
    CHECK_SOCKET_CLOSED_ONCE();
}

TEST(SolidSyslogWinsockTcpStream, DestroyClosesOpenSocket)
{
    SolidSyslogStream_Open(stream, addr);
    SolidSyslogWinsockTcpStream_Destroy(stream);
    LONGS_EQUAL(1, WinsockFake_CloseCallCount());
}

TEST(SolidSyslogWinsockTcpStream, DestroyClosesWithSocketFd)
{
    SolidSyslogStream_Open(stream, addr);
    SolidSyslogWinsockTcpStream_Destroy(stream);
    CHECK(WinsockFake_SocketFd() == WinsockFake_LastClosedFd());
}

TEST(SolidSyslogWinsockTcpStream, DefaultPortMatchesRfc6587)
{
    LONGS_EQUAL(601, SOLIDSYSLOG_TCP_DEFAULT_PORT);
}
