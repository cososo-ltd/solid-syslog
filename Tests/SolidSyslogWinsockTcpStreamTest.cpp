#include "TestUtils.h"
#include "CppUTest/TestHarness.h"

using namespace CososoTesting; // NOLINT(google-build-using-namespace) -- test-file scope only; brings NEVER/ONCE/TWICE/THRICE into scope for the CALLED_*
    // macros
#include "ConfigLockFake.h"
#include "ErrorHandlerFake.h"
#include "SolidSyslogWinsockAddress.h"
#include "SolidSyslogWinsockAddressPrivate.h"
#include "SolidSyslogErrorMessages.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogStream.h"
#include "SolidSyslogStreamDefinition.h"
#include "SolidSyslogTransport.h"
#include "SolidSyslogTunables.h"
#include "SolidSyslogWinsockTcpStream.h"
#include "SolidSyslogWinsockTcpStreamInternal.h"
#include "WinsockFake.h"
#include <cstdint>
#include <winsock2.h>
#include <ws2tcpip.h>

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
static const char* const TEST_MESSAGE     = "hello";
static const size_t      TEST_MESSAGE_LEN = 5;
static const char* const TEST_ADDRESS     = "127.0.0.1";
static const int         TEST_PORT        = 514;

TEST_GROUP(SolidSyslogWinsockTcpStream)
{
    // cppcheck-suppress unreadVariable -- used across TEST_GROUP methods; cppcheck does not model CppUTest macros
    struct SolidSyslogStream* stream = nullptr;
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
        stream                  = SolidSyslogWinsockTcpStream_Create();
        addr                    = SolidSyslogWinsockAddress_Create();
        struct sockaddr_in* sin = SolidSyslogWinsockAddress_AsSockaddrIn(addr);
        sin->sin_family         = AF_INET;
        sin->sin_port           = htons(TEST_PORT);
        inet_pton(AF_INET, TEST_ADDRESS, &sin->sin_addr);
    }

    void teardown() override
    {
        SolidSyslogWinsockAddress_Destroy(addr);
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
        CALLED_FAKE(WinsockFake_Close, ONCE);                        \
        CHECK(WinsockFake_SocketFd() == WinsockFake_LastClosedFd()); \
    } while (0)

// NOLINTEND(cppcoreguidelines-macro-usage,cppcoreguidelines-avoid-do-while)

TEST(SolidSyslogWinsockTcpStream, CreateDestroyWorksWithoutCrashing)
{
}

TEST(SolidSyslogWinsockTcpStream, OpenCallsSocketOnce)
{
    SolidSyslogStream_Open(stream, addr);
    CALLED_FAKE(WinsockFake_Socket, ONCE);
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
    CALLED_FAKE(WinsockFake_Connect, ONCE);
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
    CALLED_FAKE(WinsockFake_Connect, NEVER);
    CALLED_FAKE(WinsockFake_SetSockOpt, NEVER);
}

TEST(SolidSyslogWinsockTcpStream, OpenClosesSocketOnConnectFailure)
{
    WinsockFake_SetConnectFails(true);
    SolidSyslogStream_Open(stream, addr);
    CALLED_FAKE(WinsockFake_Close, ONCE);
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
    CALLED_FAKE(WinsockFake_Fionbio, ONCE);
    LONGS_EQUAL(1, WinsockFake_FionbioArgAt(0));
}

TEST(SolidSyslogWinsockTcpStream, OpenSkipsSelectWhenConnectReturnsImmediately)
{
    /* Default fake connect returns 0 (immediate success); select must not be
       reached because connect short-circuits the wait. */
    SolidSyslogStream_Open(stream, addr);
    CALLED_FAKE(WinsockFake_Select, NEVER);
}

TEST(SolidSyslogWinsockTcpStream, OpenInvokesSelectWhenConnectReturnsWouldBlock)
{
    WinsockFake_SetConnectFailsWithLastError(WSAEWOULDBLOCK);
    SolidSyslogStream_Open(stream, addr);
    CALLED_FAKE(WinsockFake_Select, ONCE);
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
    CALLED_FAKE(WinsockFake_Close, ONCE);
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
    CALLED_FAKE(WinsockFake_Select, NEVER);
}

TEST(SolidSyslogWinsockTcpStream, SendCallsSendOnce)
{
    SolidSyslogStream_Open(stream, addr);
    SolidSyslogStream_Send(stream, TEST_MESSAGE, TEST_MESSAGE_LEN);
    CALLED_FAKE(WinsockFake_Send, ONCE);
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
    CALLED_FAKE(WinsockFake_Send, ONCE);
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
    CALLED_FAKE(WinsockFake_Close, ONCE);
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
    CALLED_FAKE(WinsockFake_Close, NEVER);
}

TEST(SolidSyslogWinsockTcpStream, ReadCallsRecvOnce)
{
    SolidSyslogStream_Open(stream, addr);
    char buf[16];
    SolidSyslogStream_Read(stream, buf, sizeof(buf));
    CALLED_FAKE(WinsockFake_Recv, ONCE);
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
    char buf[16];
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
    CALLED_FAKE(WinsockFake_Close, ONCE);
}

// clang-format off
TEST_GROUP(SolidSyslogWinsockTcpStreamPool)
{
    // cppcheck-suppress constVariable -- assigned in test bodies; cppcheck does not model CppUTest lifecycle
    struct SolidSyslogStream* pooled[SOLIDSYSLOG_WINSOCK_TCP_STREAM_POOL_SIZE] = {};
    struct SolidSyslogStream* overflow                                         = nullptr;

    void teardown() override
    {
        for (auto* handle : pooled)
        {
            if (handle != nullptr)
            {
                SolidSyslogWinsockTcpStream_Destroy(handle);
            }
        }
        // cppcheck-suppress knownConditionTrueFalse -- assigned in test bodies; cppcheck does not model CppUTest lifecycle
        if (overflow != nullptr)
        {
            SolidSyslogWinsockTcpStream_Destroy(overflow);
        }
        ConfigLockFake_Uninstall();
        ErrorHandlerFake_Uninstall();
    }

    void FillPool()
    {
        for (auto*& slot : pooled)
        {
            slot = SolidSyslogWinsockTcpStream_Create();
        }
    }
};

// clang-format on

TEST(SolidSyslogWinsockTcpStreamPool, FillingPoolThenOverflowReturnsDistinctFallback)
{
    FillPool();

    overflow = SolidSyslogWinsockTcpStream_Create();

    CHECK_IS_FALLBACK(overflow, pooled);
}

TEST(SolidSyslogWinsockTcpStreamPool, ExhaustedCreateReportsError)
{
    ErrorHandlerFake_Install(nullptr);
    FillPool();

    overflow = SolidSyslogWinsockTcpStream_Create();

    CALLED_FAKE(ErrorHandlerFake_Handle, ONCE);
    LONGS_EQUAL(SOLIDSYSLOG_SEVERITY_ERROR, ErrorHandlerFake_LastSeverity());
    STRCMP_EQUAL(SOLIDSYSLOG_ERROR_MSG_WINSOCKTCPSTREAM_POOL_EXHAUSTED, ErrorHandlerFake_LastMessage());
}

TEST(SolidSyslogWinsockTcpStreamPool, FallbackSendIsNoOp)
{
    FillPool();
    overflow = SolidSyslogWinsockTcpStream_Create();

    CHECK_TRUE(SolidSyslogStream_Send(overflow, "x", 1));
}

TEST(SolidSyslogWinsockTcpStreamPool, CreateAcquiresAndReleasesConfigLockOnFirstFreeSlot)
{
    ConfigLockFake_Install();

    pooled[0] = SolidSyslogWinsockTcpStream_Create();

    CALLED_FAKE(ConfigLockFake_Lock, ONCE);
    CALLED_FAKE(ConfigLockFake_Unlock, ONCE);
}

TEST(SolidSyslogWinsockTcpStreamPool, CreateLocksOncePerSlotProbedWhenPoolIsFull)
{
    FillPool();
    ConfigLockFake_Install();

    overflow = SolidSyslogWinsockTcpStream_Create();

    LONGS_EQUAL(SOLIDSYSLOG_WINSOCK_TCP_STREAM_POOL_SIZE, ConfigLockFake_LockCallCount());
    LONGS_EQUAL(SOLIDSYSLOG_WINSOCK_TCP_STREAM_POOL_SIZE, ConfigLockFake_UnlockCallCount());
}

TEST(SolidSyslogWinsockTcpStreamPool, DestroyOfPooledHandleLocksOnce)
{
    pooled[0] = SolidSyslogWinsockTcpStream_Create();
    ConfigLockFake_Install();

    SolidSyslogWinsockTcpStream_Destroy(pooled[0]);
    pooled[0] = nullptr;

    CALLED_FAKE(ConfigLockFake_Lock, ONCE);
    CALLED_FAKE(ConfigLockFake_Unlock, ONCE);
}

TEST(SolidSyslogWinsockTcpStreamPool, DestroyOfUnknownHandleDoesNotLock)
{
    ConfigLockFake_Install();
    struct SolidSyslogStream stranger = {};

    SolidSyslogWinsockTcpStream_Destroy(&stranger);

    CALLED_FAKE(ConfigLockFake_Lock, NEVER);
    CALLED_FAKE(ConfigLockFake_Unlock, NEVER);
}

TEST(SolidSyslogWinsockTcpStreamPool, DestroyOfUnknownHandleReportsWarning)
{
    ErrorHandlerFake_Install(nullptr);
    struct SolidSyslogStream stranger = {};

    SolidSyslogWinsockTcpStream_Destroy(&stranger);

    CALLED_FAKE(ErrorHandlerFake_Handle, ONCE);
    LONGS_EQUAL(SOLIDSYSLOG_SEVERITY_WARNING, ErrorHandlerFake_LastSeverity());
    STRCMP_EQUAL(SOLIDSYSLOG_ERROR_MSG_WINSOCKTCPSTREAM_UNKNOWN_DESTROY, ErrorHandlerFake_LastMessage());
}

TEST(SolidSyslogWinsockTcpStreamPool, DestroyOfStaleHandleReportsWarning)
{
    pooled[0] = SolidSyslogWinsockTcpStream_Create();
    SolidSyslogWinsockTcpStream_Destroy(pooled[0]);
    ErrorHandlerFake_Install(nullptr);

    SolidSyslogWinsockTcpStream_Destroy(pooled[0]);
    pooled[0] = nullptr;

    CALLED_FAKE(ErrorHandlerFake_Handle, ONCE);
    LONGS_EQUAL(SOLIDSYSLOG_SEVERITY_WARNING, ErrorHandlerFake_LastSeverity());
    STRCMP_EQUAL(SOLIDSYSLOG_ERROR_MSG_WINSOCKTCPSTREAM_UNKNOWN_DESTROY, ErrorHandlerFake_LastMessage());
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
