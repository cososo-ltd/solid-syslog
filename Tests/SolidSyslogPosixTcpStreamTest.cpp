#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <stddef.h>
#include <cerrno>
#include <cstdint>

#include "CppUTest/TestHarness.h"
#include "SolidSyslogAddress.h"
#include "SolidSyslogPosixTcpStream.h"
#include "SolidSyslogStream.h"
#include "SolidSyslogTransport.h"
#include "SocketFake.h"

// clang-format off
static const char* const TEST_MESSAGE     = "hello";
static const size_t      TEST_MESSAGE_LEN = 5;
static const char* const TEST_ADDRESS     = "127.0.0.1";
static const int         TEST_PORT        = 514;

TEST_GROUP(SolidSyslogPosixTcpStream)
{
    SolidSyslogPosixTcpStreamStorage streamStorage{};
    // cppcheck-suppress unreadVariable -- used across TEST_GROUP methods; cppcheck does not model CppUTest macros
    struct SolidSyslogStream* stream = nullptr;
    SolidSyslogAddressStorage addrStorage{};
    // cppcheck-suppress unreadVariable -- assigned in setup; cppcheck does not model CppUTest macros
    struct SolidSyslogAddress* addr = nullptr;

    void setup() override
    {
        SocketFake_Reset();
        // cppcheck-suppress unreadVariable -- used in tests; cppcheck does not model CppUTest macros
        stream = SolidSyslogPosixTcpStream_Create(&streamStorage);
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
        SolidSyslogPosixTcpStream_Destroy(stream);
    }

    [[nodiscard]] SolidSyslogSsize Read16ByteBuffer() const
    {
        char buf[16];
        return SolidSyslogStream_Read(stream, buf, sizeof(buf));
    }
};

// clang-format on

// NOLINTBEGIN(cppcoreguidelines-macro-usage,cppcoreguidelines-avoid-do-while)
#define CHECK_SOCKET_CLOSED_ONCE()                                     \
    do                                                                 \
    {                                                                  \
        LONGS_EQUAL(1, SocketFake_CloseCallCount());                   \
        LONGS_EQUAL(SocketFake_SocketFd(), SocketFake_LastClosedFd()); \
    } while (0)

// NOLINTEND(cppcoreguidelines-macro-usage,cppcoreguidelines-avoid-do-while)

TEST(SolidSyslogPosixTcpStream, CreateDestroyWorksWithoutCrashing)
{
}

TEST(SolidSyslogPosixTcpStream, CreateReturnsHandleInsideCallerSuppliedStorage)
{
    SolidSyslogPosixTcpStreamStorage storage{};
    struct SolidSyslogStream*        localStream = SolidSyslogPosixTcpStream_Create(&storage);
    POINTERS_EQUAL(&storage, localStream);
    SolidSyslogPosixTcpStream_Destroy(localStream);
}

TEST(SolidSyslogPosixTcpStream, OpenCallsSocketOnce)
{
    SolidSyslogStream_Open(stream, addr);
    LONGS_EQUAL(1, SocketFake_SocketCallCount());
}

TEST(SolidSyslogPosixTcpStream, OpenCallsSocketWithAF_INET)
{
    SolidSyslogStream_Open(stream, addr);
    LONGS_EQUAL(AF_INET, SocketFake_SocketDomain());
}

TEST(SolidSyslogPosixTcpStream, OpenCallsSocketWithSOCK_STREAM)
{
    SolidSyslogStream_Open(stream, addr);
    LONGS_EQUAL(SOCK_STREAM, SocketFake_SocketType());
}

TEST(SolidSyslogPosixTcpStream, OpenEnablesTcpNoDelay)
{
    SolidSyslogStream_Open(stream, addr);
    CHECK_TRUE(SocketFake_HasSetSockOpt(IPPROTO_TCP, TCP_NODELAY));
}

TEST(SolidSyslogPosixTcpStream, OpenSetsNonBlockingFlagBeforeConnect)
{
    SolidSyslogStream_Open(stream, addr);
    /* Non-blocking is set via fcntl(F_SETFL, ... | O_NONBLOCK) so connect()
       returns EINPROGRESS immediately and the caller can bound the wait. */
    CHECK_TRUE(SocketFake_FcntlSetFlSetNonBlocking());
}

TEST(SolidSyslogPosixTcpStream, OpenFailsWhenFcntlSetFlFails)
{
    /* If the kernel refuses to put the socket into non-blocking mode the
       caller cannot bound the connect wait — fail fast. */
    SocketFake_SetFcntlSetFlFails(true);
    CHECK_FALSE(SolidSyslogStream_Open(stream, addr));
    LONGS_EQUAL(1, SocketFake_CloseCallCount());
}

TEST(SolidSyslogPosixTcpStream, OpenCallsConnectWithSocketFd)
{
    SolidSyslogStream_Open(stream, addr);
    LONGS_EQUAL(1, SocketFake_ConnectCallCount());
    LONGS_EQUAL(SocketFake_SocketFd(), SocketFake_LastConnectFd());
}

TEST(SolidSyslogPosixTcpStream, OpenCallsConnectWithProvidedAddress)
{
    SolidSyslogStream_Open(stream, addr);
    LONGS_EQUAL(TEST_PORT, SocketFake_LastConnectPort());
    STRCMP_EQUAL(TEST_ADDRESS, SocketFake_LastConnectAddrAsString());
}

TEST(SolidSyslogPosixTcpStream, OpenReturnsTrueOnSuccess)
{
    CHECK_TRUE(SolidSyslogStream_Open(stream, addr));
}

TEST(SolidSyslogPosixTcpStream, OpenReturnsFalseOnConnectFailure)
{
    SocketFake_SetConnectFails(true);
    CHECK_FALSE(SolidSyslogStream_Open(stream, addr));
}

TEST(SolidSyslogPosixTcpStream, OpenReturnsFalseWhenSocketFails)
{
    SocketFake_SetSocketFails(true);
    CHECK_FALSE(SolidSyslogStream_Open(stream, addr));
}

TEST(SolidSyslogPosixTcpStream, OpenSkipsConnectAndSetsockoptWhenSocketFails)
{
    SocketFake_SetSocketFails(true);
    SolidSyslogStream_Open(stream, addr);
    LONGS_EQUAL(0, SocketFake_ConnectCallCount());
    LONGS_EQUAL(0, SocketFake_SetSockOptCallCount());
}

TEST(SolidSyslogPosixTcpStream, SendReturnsFalseOnShortWrite)
{
    SolidSyslogStream_Open(stream, addr);
    SocketFake_SetSendReturn(3);
    CHECK_FALSE(SolidSyslogStream_Send(stream, TEST_MESSAGE, TEST_MESSAGE_LEN));
}

TEST(SolidSyslogPosixTcpStream, SendDoesNotRetryAfterShortWrite)
{
    SolidSyslogStream_Open(stream, addr);
    SocketFake_SetSendReturn(3);
    SolidSyslogStream_Send(stream, TEST_MESSAGE, TEST_MESSAGE_LEN);
    LONGS_EQUAL(1, SocketFake_SendCallCount());
}

TEST(SolidSyslogPosixTcpStream, SendReturnsFalseOnEintr)
{
    /* Non-blocking + fail-fast: any failure from a single send() call closes
       the socket and surfaces failure. EINTR is no longer retried. */
    SolidSyslogStream_Open(stream, addr);
    SocketFake_FailNextSendWithErrno(EINTR);
    CHECK_FALSE(SolidSyslogStream_Send(stream, TEST_MESSAGE, TEST_MESSAGE_LEN));
}

TEST(SolidSyslogPosixTcpStream, SendReturnsFalseOnEagain)
{
    SolidSyslogStream_Open(stream, addr);
    SocketFake_FailNextSendWithErrno(EAGAIN);
    CHECK_FALSE(SolidSyslogStream_Send(stream, TEST_MESSAGE, TEST_MESSAGE_LEN));
}

TEST(SolidSyslogPosixTcpStream, SendClosesSocketOnFailure)
{
    SolidSyslogStream_Open(stream, addr);
    SocketFake_SetSendFails(true);
    SolidSyslogStream_Send(stream, TEST_MESSAGE, TEST_MESSAGE_LEN);
    CHECK_SOCKET_CLOSED_ONCE();
}

TEST(SolidSyslogPosixTcpStream, OpenClosesSocketOnConnectFailure)
{
    SocketFake_SetConnectFails(true);
    SolidSyslogStream_Open(stream, addr);
    LONGS_EQUAL(1, SocketFake_CloseCallCount());
    LONGS_EQUAL(SocketFake_SocketFd(), SocketFake_LastClosedFd());
}

TEST(SolidSyslogPosixTcpStream, SendCallsSendOnce)
{
    SolidSyslogStream_Open(stream, addr);
    SolidSyslogStream_Send(stream, TEST_MESSAGE, TEST_MESSAGE_LEN);
    LONGS_EQUAL(1, SocketFake_SendCallCount());
}

TEST(SolidSyslogPosixTcpStream, SendPassesBuffer)
{
    SolidSyslogStream_Open(stream, addr);
    SolidSyslogStream_Send(stream, TEST_MESSAGE, TEST_MESSAGE_LEN);
    STRCMP_EQUAL(TEST_MESSAGE, SocketFake_SendBufAsString(0));
}

TEST(SolidSyslogPosixTcpStream, SendPassesLength)
{
    SolidSyslogStream_Open(stream, addr);
    SolidSyslogStream_Send(stream, TEST_MESSAGE, TEST_MESSAGE_LEN);
    LONGS_EQUAL(TEST_MESSAGE_LEN, SocketFake_SendLen(0));
}

TEST(SolidSyslogPosixTcpStream, SendPassesMSG_NOSIGNALFlag)
{
    SolidSyslogStream_Open(stream, addr);
    SolidSyslogStream_Send(stream, TEST_MESSAGE, TEST_MESSAGE_LEN);
    LONGS_EQUAL(MSG_NOSIGNAL, SocketFake_SendFlags(0));
}

TEST(SolidSyslogPosixTcpStream, SendPassesSocketFd)
{
    SolidSyslogStream_Open(stream, addr);
    SolidSyslogStream_Send(stream, TEST_MESSAGE, TEST_MESSAGE_LEN);
    LONGS_EQUAL(SocketFake_SocketFd(), SocketFake_LastSendFd());
}

TEST(SolidSyslogPosixTcpStream, SendReturnsTrueOnSuccess)
{
    SolidSyslogStream_Open(stream, addr);
    CHECK_TRUE(SolidSyslogStream_Send(stream, TEST_MESSAGE, TEST_MESSAGE_LEN));
}

TEST(SolidSyslogPosixTcpStream, SendReturnsFalseOnSendFailure)
{
    SolidSyslogStream_Open(stream, addr);
    SocketFake_SetSendFails(true);
    CHECK_FALSE(SolidSyslogStream_Send(stream, TEST_MESSAGE, TEST_MESSAGE_LEN));
}

TEST(SolidSyslogPosixTcpStream, CloseCallsCloseOnce)
{
    SolidSyslogStream_Open(stream, addr);
    SolidSyslogStream_Close(stream);
    LONGS_EQUAL(1, SocketFake_CloseCallCount());
}

TEST(SolidSyslogPosixTcpStream, CloseCalledWithSocketFd)
{
    SolidSyslogStream_Open(stream, addr);
    SolidSyslogStream_Close(stream);
    LONGS_EQUAL(SocketFake_SocketFd(), SocketFake_LastClosedFd());
}

TEST(SolidSyslogPosixTcpStream, CloseIsNoOpWhenNotOpen)
{
    SolidSyslogStream_Close(stream);
    LONGS_EQUAL(0, SocketFake_CloseCallCount());
}

TEST(SolidSyslogPosixTcpStream, ReadCallsRecvOnce)
{
    SolidSyslogStream_Open(stream, addr);
    char buf[16];
    SolidSyslogStream_Read(stream, buf, sizeof(buf));
    LONGS_EQUAL(1, SocketFake_RecvCallCount());
}

TEST(SolidSyslogPosixTcpStream, ReadPassesSocketFdToRecv)
{
    SolidSyslogStream_Open(stream, addr);
    char buf[16];
    SolidSyslogStream_Read(stream, buf, sizeof(buf));
    LONGS_EQUAL(SocketFake_SocketFd(), SocketFake_LastRecvFd());
}

TEST(SolidSyslogPosixTcpStream, ReadPassesBufferToRecv)
{
    SolidSyslogStream_Open(stream, addr);
    char buf[16];
    SolidSyslogStream_Read(stream, buf, sizeof(buf));
    POINTERS_EQUAL(buf, SocketFake_LastRecvBuf());
}

TEST(SolidSyslogPosixTcpStream, ReadPassesLengthToRecv)
{
    SolidSyslogStream_Open(stream, addr);
    char buf[16];
    SolidSyslogStream_Read(stream, buf, sizeof(buf));
    LONGS_EQUAL(sizeof(buf), SocketFake_LastRecvLen());
}

TEST(SolidSyslogPosixTcpStream, ReadPassesZeroFlagsToRecv)
{
    SolidSyslogStream_Open(stream, addr);
    char buf[16];
    SolidSyslogStream_Read(stream, buf, sizeof(buf));
    LONGS_EQUAL(0, SocketFake_LastRecvFlags());
}

TEST(SolidSyslogPosixTcpStream, ReadReturnsRecvReturnValue)
{
    SocketFake_SetRecvReturn(7);
    SolidSyslogStream_Open(stream, addr);
    char             buf[16];
    SolidSyslogSsize n = SolidSyslogStream_Read(stream, buf, sizeof(buf));
    LONGS_EQUAL(7, n);
}

TEST(SolidSyslogPosixTcpStream, DestroyClosesOpenSocket)
{
    SolidSyslogStream_Open(stream, addr);
    SolidSyslogPosixTcpStream_Destroy(stream);
    LONGS_EQUAL(1, SocketFake_CloseCallCount());
}

TEST(SolidSyslogPosixTcpStream, DestroyClosesWithSocketFd)
{
    SolidSyslogStream_Open(stream, addr);
    SolidSyslogPosixTcpStream_Destroy(stream);
    LONGS_EQUAL(SocketFake_SocketFd(), SocketFake_LastClosedFd());
}

TEST(SolidSyslogPosixTcpStream, DefaultPortMatchesRfc6587)
{
    LONGS_EQUAL(601, SOLIDSYSLOG_TCP_DEFAULT_PORT);
}

/* ----------------------------------------------------------------------
 * Non-blocking connect with bounded wait — keeps the service-thread drain
 * rate insensitive to a slow or refused peer.
 * -------------------------------------------------------------------- */

TEST(SolidSyslogPosixTcpStream, OpenSkipsSelectWhenConnectReturnsImmediately)
{
    /* Default fake connect returns 0 (immediate success); select must not be
       reached because connect short-circuits the wait. */
    SolidSyslogStream_Open(stream, addr);
    LONGS_EQUAL(0, SocketFake_SelectCallCount());
}

TEST(SolidSyslogPosixTcpStream, OpenInvokesSelectWhenConnectReturnsEinprogress)
{
    SocketFake_SetConnectFailsWithErrno(EINPROGRESS);
    SolidSyslogStream_Open(stream, addr);
    LONGS_EQUAL(1, SocketFake_SelectCallCount());
}

TEST(SolidSyslogPosixTcpStream, OpenPassesBoundedConnectTimeoutToSelect)
{
    SocketFake_SetConnectFailsWithErrno(EINPROGRESS);
    SolidSyslogStream_Open(stream, addr);
    /* CONNECT_TIMEOUT_MICROSECONDS = 200 000 → 0 s + 200 000 µs. */
    LONGS_EQUAL(0, SocketFake_LastSelectTimeoutSec());
    LONGS_EQUAL(200000, SocketFake_LastSelectTimeoutUsec());
}

TEST(SolidSyslogPosixTcpStream, OpenSucceedsWhenSelectReportsWritableAndZeroSO_ERROR)
{
    SocketFake_SetConnectFailsWithErrno(EINPROGRESS);
    SocketFake_SetSelectWritable(true);
    SocketFake_SetSoError(0);
    CHECK_TRUE(SolidSyslogStream_Open(stream, addr));
}

TEST(SolidSyslogPosixTcpStream, OpenFailsWhenSelectReportsTimeout)
{
    SocketFake_SetConnectFailsWithErrno(EINPROGRESS);
    SocketFake_SetSelectWritable(false);
    SocketFake_SetSelectReturn(0);
    CHECK_FALSE(SolidSyslogStream_Open(stream, addr));
}

TEST(SolidSyslogPosixTcpStream, OpenClosesSocketOnSelectTimeout)
{
    SocketFake_SetConnectFailsWithErrno(EINPROGRESS);
    SocketFake_SetSelectWritable(false);
    SocketFake_SetSelectReturn(0);
    SolidSyslogStream_Open(stream, addr);
    CHECK_SOCKET_CLOSED_ONCE();
}

TEST(SolidSyslogPosixTcpStream, OpenFailsWhenSelectFlagsErrorOnFd)
{
    SocketFake_SetConnectFailsWithErrno(EINPROGRESS);
    SocketFake_SetSelectWritable(false);
    SocketFake_SetSelectError(true);
    CHECK_FALSE(SolidSyslogStream_Open(stream, addr));
}

TEST(SolidSyslogPosixTcpStream, OpenFailsWhenDeferredSO_ERRORIsNonZero)
{
    /* select reports writable, but SO_ERROR on the socket reveals the
       deferred connect failure (e.g. ECONNREFUSED). */
    SocketFake_SetConnectFailsWithErrno(EINPROGRESS);
    SocketFake_SetSelectWritable(true);
    SocketFake_SetSoError(ECONNREFUSED);
    CHECK_FALSE(SolidSyslogStream_Open(stream, addr));
}

TEST(SolidSyslogPosixTcpStream, OpenReadsSO_ERRORAfterSelectWritable)
{
    SocketFake_SetConnectFailsWithErrno(EINPROGRESS);
    SolidSyslogStream_Open(stream, addr);
    LONGS_EQUAL(SOL_SOCKET, SocketFake_LastGetSockOptLevel());
    LONGS_EQUAL(SO_ERROR, SocketFake_LastGetSockOptOptname());
}

TEST(SolidSyslogPosixTcpStream, OpenFailsWhenConnectFailsImmediatelyWithRefused)
{
    /* Non-EINPROGRESS errors are immediate failures — no select wait, no
       SO_ERROR check, just fail fast. */
    SocketFake_SetConnectFailsWithErrno(ECONNREFUSED);
    CHECK_FALSE(SolidSyslogStream_Open(stream, addr));
    LONGS_EQUAL(0, SocketFake_SelectCallCount());
}

TEST(SolidSyslogPosixTcpStream, OpenFailsWhenSO_ERRORLookupFails)
{
    SocketFake_SetConnectFailsWithErrno(EINPROGRESS);
    SocketFake_SetSelectWritable(true);
    SocketFake_SetSoErrorLookupFails(true);
    CHECK_FALSE(SolidSyslogStream_Open(stream, addr));
}

/* ----------------------------------------------------------------------
 * Non-blocking Read contract: bytes → return them; nothing → 0;
 * EOF/error → close internally and return -1.
 * -------------------------------------------------------------------- */

TEST(SolidSyslogPosixTcpStream, ReadReturnsZeroOnEagain)
{
    SolidSyslogStream_Open(stream, addr);
    SocketFake_FailNextRecvWithErrno(EAGAIN);
    LONGS_EQUAL(0, Read16ByteBuffer());
}

TEST(SolidSyslogPosixTcpStream, ReadReturnsZeroOnWouldBlock)
{
    SolidSyslogStream_Open(stream, addr);
    SocketFake_FailNextRecvWithErrno(EWOULDBLOCK);
    LONGS_EQUAL(0, Read16ByteBuffer());
}

TEST(SolidSyslogPosixTcpStream, ReadReturnsNegativeOneOnEofAndClosesSocket)
{
    SolidSyslogStream_Open(stream, addr);
    SocketFake_SetRecvReturn(0); /* EOF */
    LONGS_EQUAL(-1, Read16ByteBuffer());
    CHECK_SOCKET_CLOSED_ONCE();
}

TEST(SolidSyslogPosixTcpStream, ReadReturnsNegativeOneOnErrorAndClosesSocket)
{
    SolidSyslogStream_Open(stream, addr);
    SocketFake_FailNextRecvWithErrno(ECONNRESET);
    LONGS_EQUAL(-1, Read16ByteBuffer());
    CHECK_SOCKET_CLOSED_ONCE();
}
