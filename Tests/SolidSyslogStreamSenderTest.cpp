#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/socket.h>

#include "SolidSyslogEndpoint.h"
#include "SolidSyslogFormatter.h"
#include "SolidSyslogGetAddrInfoResolver.h"
#include "SolidSyslogPosixTcpStream.h"
#include "SolidSyslogSender.h"
#include "SolidSyslogStreamSender.h"
#include "SocketFake.h"
#include "CppUTest/TestHarness.h"

// clang-format off
static const char* const TEST_HOST           = "127.0.0.1";
static const int         TEST_PORT           = 514;
static const char* const TEST_ALTERNATE_HOST = "192.168.1.1";
static const int         TEST_ALTERNATE_PORT = 9999;
static const char* const TEST_MESSAGE        = "hello";
static const size_t      TEST_MESSAGE_LEN    = 5;
// clang-format on

static int GetPort()
{
    return TEST_PORT;
}

static const char* GetHost()
{
    return TEST_HOST;
}

static int GetAlternatePort()
{
    return TEST_ALTERNATE_PORT;
}

static const char* GetAlternateHost()
{
    return TEST_ALTERNATE_HOST;
}

static int getPortCallCount;

static int SpyGetPort()
{
    getPortCallCount++;
    return TEST_PORT;
}

static int getHostCallCount;

static const char* SpyGetHost()
{
    getHostCallCount++;
    return TEST_HOST;
}

// Endpoint stubs — delegate to per-test function pointers so existing
// callback-spy tests in TEST_GROUP(SolidSyslogStreamSenderConfig) keep counting
// callback invocations through the new endpoint path. endpointVersion is the
// per-test version reported by TestEndpointVersion; bump it between Sends to
// drive fingerprint-reconnection tests.
static const char* (*endpointGetHost)() = GetHost;
static int (*endpointGetPort)()         = GetPort;
static uint32_t endpointVersion         = 0;

static void TestEndpoint(struct SolidSyslogEndpoint* endpoint)
{
    SolidSyslogFormatter_BoundedString(endpoint->host, endpointGetHost(), SOLIDSYSLOG_MAX_HOST_SIZE);
    endpoint->port = (uint16_t) endpointGetPort();
}

static uint32_t TestEndpointVersion() // NOLINT(modernize-use-trailing-return-type)
{
    return endpointVersion;
}

// clang-format off
TEST_GROUP(SolidSyslogStreamSender)
{
    struct SolidSyslogResolver*      resolver = nullptr;
    SolidSyslogPosixTcpStreamStorage streamStorage{};
    struct SolidSyslogStream*        stream = nullptr;
    struct SolidSyslogStreamSenderConfig config;
    SolidSyslogStreamSenderStorage senderStorage{};
    // cppcheck-suppress constVariablePointer -- Send requires non-const self; false positive from macro expansion
    // cppcheck-suppress unreadVariable -- used across TEST_GROUP methods; cppcheck does not model CppUTest macros
    struct SolidSyslogSender* sender = nullptr;

    void setup() override
    {
        SocketFake_Reset();
        endpointGetHost = GetHost;
        endpointVersion = 0;
        endpointGetPort = GetPort;
        resolver        = SolidSyslogGetAddrInfoResolver_Create();
        stream          = SolidSyslogPosixTcpStream_Create(&streamStorage);
        config          = {resolver, stream, TestEndpoint, TestEndpointVersion};
        // cppcheck-suppress unreadVariable -- read by teardown and tests; cppcheck does not model CppUTest lifecycle
        sender = SolidSyslogStreamSender_Create(&senderStorage, &config);
    }

    void teardown() override
    {
        SolidSyslogStreamSender_Destroy(sender);
        SolidSyslogPosixTcpStream_Destroy(stream);
        SolidSyslogGetAddrInfoResolver_Destroy();
    }

    void Send() const
    {
        SolidSyslogSender_Send(sender, TEST_MESSAGE, TEST_MESSAGE_LEN);
    }
};

// clang-format on

TEST(SolidSyslogStreamSender, CreateReturnsNonNull)
{
    CHECK_TRUE(sender != nullptr);
}

TEST(SolidSyslogStreamSender, CreateReturnsHandleInsideCallerSuppliedStorage)
{
    SolidSyslogStreamSenderStorage       localStorage{};
    struct SolidSyslogStreamSenderConfig localConfig = {resolver, stream, TestEndpoint, TestEndpointVersion};
    struct SolidSyslogSender*            localSender = SolidSyslogStreamSender_Create(&localStorage, &localConfig);
    POINTERS_EQUAL(&localStorage, localSender);
    SolidSyslogStreamSender_Destroy(localSender);
}

TEST(SolidSyslogStreamSender, CreateDoesNotOpenSocket)
{
    LONGS_EQUAL(0, SocketFake_SocketCallCount());
}

TEST(SolidSyslogStreamSender, FirstSendOpensStreamSocket)
{
    Send();
    LONGS_EQUAL(1, SocketFake_SocketCallCount());
    LONGS_EQUAL(AF_INET, SocketFake_SocketDomain());
    LONGS_EQUAL(SOCK_STREAM, SocketFake_SocketType());
}

TEST(SolidSyslogStreamSender, FirstSendSetsTcpNoDelay)
{
    Send();
    CHECK_TRUE(SocketFake_HasSetSockOpt(IPPROTO_TCP, TCP_NODELAY));
}

// clang-format off
TEST_GROUP(SolidSyslogStreamSenderDestroy)
{
    struct SolidSyslogResolver*      resolver = nullptr;
    SolidSyslogPosixTcpStreamStorage streamStorage{};
    struct SolidSyslogStream*        stream = nullptr;
    struct SolidSyslogStreamSenderConfig config;
    SolidSyslogStreamSenderStorage senderStorage{};

    void setup() override
    {
        SocketFake_Reset();
        endpointGetHost = GetHost;
        endpointVersion = 0;
        endpointGetPort = GetPort;
        resolver        = SolidSyslogGetAddrInfoResolver_Create();
        stream          = SolidSyslogPosixTcpStream_Create(&streamStorage);
        // cppcheck-suppress unreadVariable -- used in test bodies; cppcheck does not model CppUTest macros
        config = {resolver, stream, TestEndpoint, TestEndpointVersion};
    }

    void teardown() override
    {
        SolidSyslogPosixTcpStream_Destroy(stream);
        SolidSyslogGetAddrInfoResolver_Destroy();
    }

    void CreateAndDestroy()
    {
        struct SolidSyslogSender* localSender = SolidSyslogStreamSender_Create(&senderStorage, &config);
        SolidSyslogStreamSender_Destroy(localSender);
    }
};

// clang-format on

TEST(SolidSyslogStreamSenderDestroy, DestroyWithoutSendDoesNotClose)
{
    CreateAndDestroy();
    LONGS_EQUAL(0, SocketFake_CloseCallCount());
}

TEST(SolidSyslogStreamSenderDestroy, DestroyAfterSendClosesSocket)
{
    struct SolidSyslogSender* sender = SolidSyslogStreamSender_Create(&senderStorage, &config);
    SolidSyslogSender_Send(sender, "x", 1);
    SolidSyslogStreamSender_Destroy(sender);
    LONGS_EQUAL(1, SocketFake_CloseCallCount());
    LONGS_EQUAL(SocketFake_SocketFd(), SocketFake_LastClosedFd());
}

TEST(SolidSyslogStreamSenderDestroy, DestroyAfterDisconnectDoesNotDoubleClose)
{
    struct SolidSyslogSender* sender = SolidSyslogStreamSender_Create(&senderStorage, &config);
    SolidSyslogSender_Send(sender, "x", 1);
    SolidSyslogSender_Disconnect(sender);
    SolidSyslogStreamSender_Destroy(sender);
    LONGS_EQUAL(1, SocketFake_CloseCallCount());
}

TEST(SolidSyslogStreamSender, SendConnectsOnFirstCall)
{
    Send();
    LONGS_EQUAL(1, SocketFake_ConnectCallCount());
}

TEST(SolidSyslogStreamSender, SendConnectsWithCorrectPort)
{
    Send();
    LONGS_EQUAL(TEST_PORT, SocketFake_LastConnectPort());
}

TEST(SolidSyslogStreamSender, SendConnectsWithCorrectAddress)
{
    Send();
    STRCMP_EQUAL(TEST_HOST, SocketFake_LastConnectAddrAsString());
}

TEST(SolidSyslogStreamSender, SendConnectsWithSocketFd)
{
    Send();
    LONGS_EQUAL(SocketFake_SocketFd(), SocketFake_LastConnectFd());
}

TEST(SolidSyslogStreamSender, SecondSendDoesNotReconnect)
{
    Send();
    Send();
    LONGS_EQUAL(1, SocketFake_ConnectCallCount());
}

TEST(SolidSyslogStreamSender, SendTransmitsOctetCountingPrefix)
{
    Send();
    STRCMP_EQUAL("5 ", SocketFake_SendBufAsString(0));
}

TEST(SolidSyslogStreamSender, SendTransmitsMessageBody)
{
    Send();
    STRCMP_EQUAL(TEST_MESSAGE, SocketFake_SendBufAsString(1));
}

TEST(SolidSyslogStreamSender, SendMakesTwoSendCalls)
{
    Send();
    LONGS_EQUAL(2, SocketFake_SendCallCount());
}

TEST(SolidSyslogStreamSender, SendUsesSocketFd)
{
    Send();
    LONGS_EQUAL(SocketFake_SocketFd(), SocketFake_LastSendFd());
}

TEST(SolidSyslogStreamSender, SendReturnsTrueOnSuccess)
{
    CHECK_TRUE(SolidSyslogSender_Send(sender, TEST_MESSAGE, TEST_MESSAGE_LEN));
}

TEST(SolidSyslogStreamSender, DisconnectAfterSendClosesSocket)
{
    Send();
    SolidSyslogSender_Disconnect(sender);
    LONGS_EQUAL(1, SocketFake_CloseCallCount());
}

TEST(SolidSyslogStreamSender, DisconnectIsIdempotent)
{
    Send();
    SolidSyslogSender_Disconnect(sender);
    SolidSyslogSender_Disconnect(sender);
    LONGS_EQUAL(1, SocketFake_CloseCallCount());
}

TEST(SolidSyslogStreamSender, SendAfterDisconnectReopensSocket)
{
    Send();
    SolidSyslogSender_Disconnect(sender);
    Send();
    LONGS_EQUAL(2, SocketFake_SocketCallCount());
}

TEST(SolidSyslogStreamSender, SendAfterDisconnectResolves)
{
    Send();
    SolidSyslogSender_Disconnect(sender);
    Send();
    LONGS_EQUAL(2, SocketFake_GetAddrInfoCallCount());
}

TEST(SolidSyslogStreamSender, DisconnectWithoutSendDoesNotClose)
{
    SolidSyslogSender_Disconnect(sender);
    LONGS_EQUAL(0, SocketFake_CloseCallCount());
}

TEST(SolidSyslogStreamSender, EndpointVersionChangeBetweenSendsTriggersReconnect)
{
    Send();
    endpointVersion = 1;
    Send();
    LONGS_EQUAL(2, SocketFake_ConnectCallCount());
}

TEST(SolidSyslogStreamSender, EndpointVersionChangeUsesNewPortOnReconnect)
{
    Send();
    endpointVersion = 1;
    endpointGetPort = GetAlternatePort;
    Send();
    LONGS_EQUAL(TEST_ALTERNATE_PORT, SocketFake_LastConnectPort());
}

// clang-format off
TEST_GROUP(SolidSyslogStreamSenderConfig)
{
    // cppcheck-suppress unreadVariable -- assigned in CreateSender; cppcheck does not model CppUTest macros
    const char* (*getHostFn)(void) = GetHost; // NOLINT(modernize-redundant-void-arg) -- C idiom
    // cppcheck-suppress unreadVariable -- assigned in CreateSender; cppcheck does not model CppUTest macros
    int (*getPortFn)(void) = GetPort; // NOLINT(modernize-redundant-void-arg) -- C idiom
    SolidSyslogPosixTcpStreamStorage streamStorage{};
    struct SolidSyslogStream*        stream = nullptr;
    SolidSyslogStreamSenderStorage senderStorage{};
    // cppcheck-suppress constVariablePointer -- Send requires non-const self; false positive from macro expansion
    // cppcheck-suppress unreadVariable -- used across TEST_GROUP methods; cppcheck does not model CppUTest macros
    struct SolidSyslogSender* sender = nullptr;

    void setup() override
    {
        SocketFake_Reset();
        getPortCallCount = 0;
        getHostCallCount = 0;
        endpointGetHost  = GetHost;
        endpointVersion  = 0;
        endpointGetPort  = GetPort;
    }

    void teardown() override
    {
        SolidSyslogStreamSender_Destroy(sender);
        SolidSyslogPosixTcpStream_Destroy(stream);
        SolidSyslogGetAddrInfoResolver_Destroy();
    }

    void CreateSender()
    {
        endpointGetHost = getHostFn;
        endpointGetPort = getPortFn;
        struct SolidSyslogResolver* resolver = SolidSyslogGetAddrInfoResolver_Create();
        stream                               = SolidSyslogPosixTcpStream_Create(&streamStorage);
        struct SolidSyslogStreamSenderConfig config = {resolver, stream, TestEndpoint, TestEndpointVersion};
        // cppcheck-suppress unreadVariable -- read by teardown and tests; cppcheck does not model CppUTest lifecycle
        sender = SolidSyslogStreamSender_Create(&senderStorage, &config);
    }

    void Send() const
    {
        SolidSyslogSender_Send(sender, TEST_MESSAGE, TEST_MESSAGE_LEN);
    }
};

// clang-format on

TEST(SolidSyslogStreamSenderConfig, GetPortCalledOnFirstSend)
{
    getPortFn = SpyGetPort;
    CreateSender();
    LONGS_EQUAL(0, getPortCallCount);
    Send();
    LONGS_EQUAL(1, getPortCallCount);
}

TEST(SolidSyslogStreamSenderConfig, GetPortNotCalledOnSecondSend)
{
    getPortFn = SpyGetPort;
    CreateSender();
    Send();
    Send();
    LONGS_EQUAL(1, getPortCallCount);
}

TEST(SolidSyslogStreamSenderConfig, ConnectsWithAlternatePort)
{
    getPortFn = GetAlternatePort;
    CreateSender();
    Send();
    LONGS_EQUAL(TEST_ALTERNATE_PORT, SocketFake_LastConnectPort());
}

TEST(SolidSyslogStreamSenderConfig, GetHostCalledOnFirstSend)
{
    getHostFn = SpyGetHost;
    CreateSender();
    LONGS_EQUAL(0, getHostCallCount);
    Send();
    LONGS_EQUAL(1, getHostCallCount);
}

TEST(SolidSyslogStreamSenderConfig, GetHostNotCalledOnSecondSend)
{
    getHostFn = SpyGetHost;
    CreateSender();
    Send();
    Send();
    LONGS_EQUAL(1, getHostCallCount);
}

TEST(SolidSyslogStreamSenderConfig, ConnectsWithAlternateHost)
{
    getHostFn = GetAlternateHost;
    CreateSender();
    Send();
    STRCMP_EQUAL(TEST_ALTERNATE_HOST, SocketFake_LastConnectAddrAsString());
}

TEST(SolidSyslogStreamSenderConfig, GetAddrInfoCalledWithHostname)
{
    getHostFn = SpyGetHost;
    CreateSender();
    Send();
    STRCMP_EQUAL(TEST_HOST, SocketFake_LastGetAddrInfoHostname());
}

// clang-format off
TEST_GROUP(SolidSyslogStreamSenderFailure)
{
    struct SolidSyslogResolver*      resolver = nullptr;
    SolidSyslogPosixTcpStreamStorage streamStorage{};
    struct SolidSyslogStream*        stream = nullptr;
    struct SolidSyslogStreamSenderConfig config;
    SolidSyslogStreamSenderStorage senderStorage{};
    // cppcheck-suppress constVariablePointer -- Send requires non-const self; false positive from macro expansion
    // cppcheck-suppress unreadVariable -- used across TEST_GROUP methods; cppcheck does not model CppUTest macros
    struct SolidSyslogSender* sender = nullptr;

    void setup() override
    {
        SocketFake_Reset();
        endpointGetHost = GetHost;
        endpointVersion = 0;
        endpointGetPort = GetPort;
        resolver        = SolidSyslogGetAddrInfoResolver_Create();
        stream          = SolidSyslogPosixTcpStream_Create(&streamStorage);
        config          = {resolver, stream, TestEndpoint, TestEndpointVersion};
        // cppcheck-suppress unreadVariable -- read by teardown and tests; cppcheck does not model CppUTest lifecycle
        sender = SolidSyslogStreamSender_Create(&senderStorage, &config);
    }

    void teardown() override
    {
        SolidSyslogStreamSender_Destroy(sender);
        SolidSyslogPosixTcpStream_Destroy(stream);
        SolidSyslogGetAddrInfoResolver_Destroy();
    }

    // NOLINTNEXTLINE(modernize-use-nodiscard) -- test helper; return value intentionally ignored in some tests
    bool Send() const
    {
        return SolidSyslogSender_Send(sender, TEST_MESSAGE, TEST_MESSAGE_LEN);
    }
};

// clang-format on

TEST(SolidSyslogStreamSenderFailure, SendReturnsFalseWhenConnectFails)
{
    SocketFake_SetConnectFails(true);
    CHECK_FALSE(Send());
}

TEST(SolidSyslogStreamSenderFailure, ConnectFailureClosesSocket)
{
    SocketFake_SetConnectFails(true);
    Send();
    LONGS_EQUAL(1, SocketFake_CloseCallCount());
}

TEST(SolidSyslogStreamSenderFailure, SendReturnsFalseWhenSendFails)
{
    SocketFake_SetSendFails(true);
    CHECK_FALSE(Send());
}

TEST(SolidSyslogStreamSenderFailure, SendFailureClosesSocket)
{
    SocketFake_SetSendFails(true);
    Send();
    LONGS_EQUAL(1, SocketFake_CloseCallCount());
}

TEST(SolidSyslogStreamSenderFailure, DestroyAfterSendFailureDoesNotDoubleClose)
{
    SocketFake_SetSendFails(true);
    Send();
    SolidSyslogStreamSender_Destroy(sender);
    LONGS_EQUAL(1, SocketFake_CloseCallCount());
}

TEST(SolidSyslogStreamSenderFailure, SendFailureMarksDisconnected)
{
    Send();
    LONGS_EQUAL(1, SocketFake_ConnectCallCount());
    SocketFake_SetSendFails(true);
    Send();
    SocketFake_SetSendFails(false);
    Send();
    LONGS_EQUAL(2, SocketFake_ConnectCallCount());
}

TEST(SolidSyslogStreamSenderFailure, ReconnectCreatesNewSocket)
{
    Send();
    int firstSocketCallCount = SocketFake_SocketCallCount();
    SocketFake_SetSendFails(true);
    Send();
    SocketFake_SetSendFails(false);
    Send();
    LONGS_EQUAL(firstSocketCallCount + 1, SocketFake_SocketCallCount());
}

TEST(SolidSyslogStreamSenderFailure, ReconnectSetsTcpNoDelay)
{
    Send();
    SocketFake_SetSendFails(true);
    Send();
    SocketFake_SetSendFails(false);
    Send();
    /* Two opens (initial + reconnect); both must set TCP_NODELAY.
     * Counted indirectly: TCP_NODELAY appears twice, plus SO_SNDTIMEO twice = 4 total. */
    LONGS_EQUAL(4, SocketFake_SetSockOptCallCount());
    CHECK_TRUE(SocketFake_HasSetSockOpt(IPPROTO_TCP, TCP_NODELAY));
}

TEST(SolidSyslogStreamSenderFailure, ReconnectResolvesDns)
{
    Send();
    SocketFake_SetSendFails(true);
    Send();
    SocketFake_SetSendFails(false);
    Send();
    LONGS_EQUAL(2, SocketFake_GetAddrInfoCallCount());
}

TEST(SolidSyslogStreamSenderFailure, ReconnectConnectsWithNewFd)
{
    Send();
    SocketFake_SetSendFails(true);
    Send();
    SocketFake_SetSendFails(false);
    Send();
    LONGS_EQUAL(SocketFake_SocketFd(), SocketFake_LastConnectFd());
}

TEST(SolidSyslogStreamSenderFailure, SuccessfulSendAfterReconnect)
{
    Send();
    SocketFake_SetSendFails(true);
    Send();
    SocketFake_SetSendFails(false);
    CHECK_TRUE(Send());
}

TEST(SolidSyslogStreamSenderFailure, SendUsesMsgNoSignal)
{
    Send();
    LONGS_EQUAL(MSG_NOSIGNAL, SocketFake_SendFlags(0));
    LONGS_EQUAL(MSG_NOSIGNAL, SocketFake_SendFlags(1));
}

TEST(SolidSyslogStreamSenderFailure, SendReturnsFalseWhenBodySendFails)
{
    SocketFake_FailSendOnCall(1);
    CHECK_FALSE(Send());
}

TEST(SolidSyslogStreamSenderFailure, BodySendFailureClosesSocket)
{
    SocketFake_FailSendOnCall(1);
    Send();
    LONGS_EQUAL(1, SocketFake_CloseCallCount());
}

TEST(SolidSyslogStreamSenderFailure, SendReturnsFalseWhenResolverFails)
{
    SocketFake_SetGetAddrInfoFails(true);
    CHECK_FALSE(Send());
}

TEST(SolidSyslogStreamSenderFailure, SendDoesNotOpenStreamWhenResolverFails)
{
    SocketFake_SetGetAddrInfoFails(true);
    Send();
    LONGS_EQUAL(0, SocketFake_SocketCallCount());
    LONGS_EQUAL(0, SocketFake_ConnectCallCount());
}

TEST(SolidSyslogStreamSenderFailure, SendReturnsTrueWhenResolverSucceeds)
{
    CHECK_TRUE(Send());
}

TEST(SolidSyslogStreamSenderFailure, SendRecoversAfterTransientResolveFailure)
{
    SocketFake_SetGetAddrInfoFails(true);
    CHECK_FALSE(Send());
    SocketFake_SetGetAddrInfoFails(false);
    CHECK_TRUE(Send());
}

TEST(SolidSyslogStreamSenderFailure, NoEndpointConfiguredConnectsToPortZero)
{
    SolidSyslogStreamSender_Destroy(sender);
    struct SolidSyslogStreamSenderConfig configNoEndpoint = {resolver, stream, nullptr, nullptr};
    struct SolidSyslogSender*            senderNoEndpoint = SolidSyslogStreamSender_Create(&senderStorage, &configNoEndpoint);
    SolidSyslogSender_Send(senderNoEndpoint, TEST_MESSAGE, TEST_MESSAGE_LEN);
    LONGS_EQUAL(1, SocketFake_ConnectCallCount());
    LONGS_EQUAL(0, SocketFake_LastConnectPort());
}
