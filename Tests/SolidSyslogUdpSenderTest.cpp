#include <netinet/in.h>
#include <stddef.h>
#include <sys/socket.h>
#include <array>
#include <cstdint>

#include "DatagramFake.h"
#include "SolidSyslogEndpoint.h"
#include "SolidSyslogFormatter.h"
#include "SolidSyslogGetAddrInfoResolver.h"
#include "SolidSyslogPosixDatagram.h"
#include "SolidSyslogTunables.h"
#include "SolidSyslogUdpSender.h"
#include "SolidSyslogSender.h"
#include "SocketFake.h"
#include "SolidSyslogDatagram.h"
#include "TestUtils.h"
#include "CppUTest/TestHarness.h"

using namespace CososoTesting; // NOLINT(google-build-using-namespace) -- test-file scope only; brings NEVER/ONCE/TWICE/THRICE into scope for the CALLED_*
                               // macros

// clang-format off
static const char* const TEST_MESSAGE          = "hello";
static const size_t      TEST_MESSAGE_LEN      = 5;
static const char* const TEST_DEFAULT_HOST     = "127.0.0.1";
static const int         TEST_DEFAULT_PORT     = 514;
static const int         TEST_ALTERNATE_PORT   = 9999;
static const size_t      TEST_MAX_MESSAGE_SIZE = SOLIDSYSLOG_MAX_MESSAGE_SIZE;
// clang-format on

static int GetDefaultPort()
{
    return TEST_DEFAULT_PORT;
}

static int GetAlternatePort()
{
    return TEST_ALTERNATE_PORT;
}

static const char* GetDefaultHost()
{
    return TEST_DEFAULT_HOST;
}

static int SpyGetHostCallCount;

static const char* SpyGetHost()
{
    SpyGetHostCallCount++;
    return TEST_DEFAULT_HOST;
}

static int SpyGetPortCallCount;

static int SpyGetPort()
{
    SpyGetPortCallCount++;
    return TEST_DEFAULT_PORT;
}

// Endpoint stubs — delegate to per-test function pointers so existing
// callback-spy tests in TEST_GROUP(SolidSyslogUdpSenderConfig) keep counting
// callback invocations through the new endpoint path. endpointVersion is the
// per-test version reported by TestEndpointVersion; bump it between Sends to
// drive fingerprint-reconnection tests.
static const char* (*endpointGetHost)() = GetDefaultHost;
static int (*endpointGetPort)()         = GetDefaultPort;
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
TEST_GROUP(SolidSyslogUdpSender)
{
    struct SolidSyslogResolver* resolver = nullptr;
    struct SolidSyslogDatagram* datagram = nullptr;
    struct SolidSyslogUdpSenderConfig config;
    // cppcheck-suppress constVariablePointer -- Send requires non-const self; false positive from macro expansion
    // cppcheck-suppress unreadVariable -- used across TEST_GROUP methods; cppcheck does not model CppUTest macros
    struct SolidSyslogSender* sender = nullptr;

    void setup() override
    {
        SocketFake_Reset();
        endpointGetHost = GetDefaultHost;
        endpointVersion = 0;
        endpointGetPort = GetDefaultPort;
        resolver = SolidSyslogGetAddrInfoResolver_Create();
        datagram = SolidSyslogPosixDatagram_Create();
        config = {resolver, datagram, TestEndpoint, TestEndpointVersion};
        // cppcheck-suppress unreadVariable -- read by teardown and tests; cppcheck does not model CppUTest lifecycle
        sender = SolidSyslogUdpSender_Create(&config);
    }

    void teardown() override
    {
        SolidSyslogUdpSender_Destroy();
        SolidSyslogPosixDatagram_Destroy();
        SolidSyslogGetAddrInfoResolver_Destroy();
    }

    void Send() const
    {
        SolidSyslogSender_Send(sender, TEST_MESSAGE, TEST_MESSAGE_LEN);
    }

    void Send(const void* buffer, size_t size) const
    {
        SolidSyslogSender_Send(sender, buffer, size);
    }
};

// clang-format on

TEST(SolidSyslogUdpSender, CreateDestroyWorksWithoutCrashing)
{
}

TEST(SolidSyslogUdpSender, CreateDoesNotOpenSocket)
{
    CALLED_FAKE(SocketFake_Socket, NEVER);
}

TEST(SolidSyslogUdpSender, SendReturnsTrueOnSuccess)
{
    CHECK_TRUE(SolidSyslogSender_Send(sender, TEST_MESSAGE, TEST_MESSAGE_LEN));
}

TEST(SolidSyslogUdpSender, SendReturnsFalseOnSendtoFailure)
{
    SocketFake_SetSendtoFails(true);
    CHECK_FALSE(SolidSyslogSender_Send(sender, TEST_MESSAGE, TEST_MESSAGE_LEN));
}

TEST(SolidSyslogUdpSender, SingleSendResultsInOneSendtoCall)
{
    Send();
    CALLED_FAKE(SocketFake_Sendto, ONCE);
}

TEST(SolidSyslogUdpSender, SendtoReceivesBuffer)
{
    Send();
    STRCMP_EQUAL(TEST_MESSAGE, SocketFake_LastBufAsString());
}

TEST(SolidSyslogUdpSender, SendtoCalledWithDefaultPort)
{
    Send();
    LONGS_EQUAL(TEST_DEFAULT_PORT, SocketFake_LastPort());
}

TEST(SolidSyslogUdpSender, MaxMessageSizeTransmittedWithoutTruncation)
{
    std::array<char, TEST_MAX_MESSAGE_SIZE> buffer{};
    buffer.fill('A');
    Send(buffer.data(), buffer.size());
    LONGS_EQUAL(TEST_MAX_MESSAGE_SIZE, SocketFake_LastLen());
    MEMCMP_EQUAL(buffer.data(), SocketFake_LastBuf(), TEST_MAX_MESSAGE_SIZE);
}

TEST(SolidSyslogUdpSender, SendtoCalledWithFlagsZero)
{
    Send();
    LONGS_EQUAL(0, SocketFake_LastFlags());
}

TEST(SolidSyslogUdpSender, SendtoCalledWithAddressFamilyAF_INET)
{
    Send();
    LONGS_EQUAL(AF_INET, SocketFake_LastAddrFamily());
}

TEST(SolidSyslogUdpSender, SendtoCalledWithDefaultHost)
{
    Send();
    STRCMP_EQUAL(TEST_DEFAULT_HOST, SocketFake_LastAddrAsString());
}

TEST(SolidSyslogUdpSender, SendtoCalledWithAddrlenOfSockaddrIn)
{
    Send();
    LONGS_EQUAL(sizeof(struct sockaddr_in), SocketFake_LastAddrLen());
}

TEST(SolidSyslogUdpSender, FirstSendOpensDatagramSocket)
{
    Send();
    CALLED_FAKE(SocketFake_Socket, ONCE);
    LONGS_EQUAL(AF_INET, SocketFake_SocketDomain());
    LONGS_EQUAL(SOCK_DGRAM, SocketFake_SocketType());
}

TEST(SolidSyslogUdpSender, FirstSendResolves)
{
    Send();
    CALLED_FAKE(SocketFake_GetAddrInfo, ONCE);
}

TEST(SolidSyslogUdpSender, SecondSendDoesNotReopenSocket)
{
    Send();
    Send();
    CALLED_FAKE(SocketFake_Socket, ONCE);
}

TEST(SolidSyslogUdpSender, SecondSendDoesNotResolve)
{
    Send();
    Send();
    CALLED_FAKE(SocketFake_GetAddrInfo, ONCE);
}

TEST(SolidSyslogUdpSender, SendtoCalledWithSocketFd)
{
    Send();
    LONGS_EQUAL(SocketFake_SocketFd(), SocketFake_LastSendtoFd());
}

TEST(SolidSyslogUdpSender, DisconnectAfterSendClosesSocket)
{
    Send();
    SolidSyslogSender_Disconnect(sender);
    CALLED_FAKE(SocketFake_Close, ONCE);
}

TEST(SolidSyslogUdpSender, DisconnectIsIdempotent)
{
    Send();
    SolidSyslogSender_Disconnect(sender);
    SolidSyslogSender_Disconnect(sender);
    CALLED_FAKE(SocketFake_Close, ONCE);
}

TEST(SolidSyslogUdpSender, SendAfterDisconnectReopensSocket)
{
    Send();
    SolidSyslogSender_Disconnect(sender);
    Send();
    CALLED_FAKE(SocketFake_Socket, TWICE);
}

TEST(SolidSyslogUdpSender, SendAfterDisconnectResolves)
{
    Send();
    SolidSyslogSender_Disconnect(sender);
    Send();
    CALLED_FAKE(SocketFake_GetAddrInfo, TWICE);
}

TEST(SolidSyslogUdpSender, DisconnectWithoutSendDoesNotClose)
{
    SolidSyslogSender_Disconnect(sender);
    CALLED_FAKE(SocketFake_Close, NEVER);
}

TEST(SolidSyslogUdpSender, EndpointVersionChangeBetweenSendsTriggersReconnect)
{
    Send();
    endpointVersion = 1;
    Send();
    CALLED_FAKE(SocketFake_Socket, TWICE);
}

TEST(SolidSyslogUdpSender, EndpointVersionChangeUsesNewPortOnReconnect)
{
    Send();
    endpointVersion = 1;
    endpointGetPort = GetAlternatePort;
    Send();
    LONGS_EQUAL(TEST_ALTERNATE_PORT, SocketFake_LastPort());
}

IGNORE_TEST(SolidSyslogUdpSender, HappyPathOnly)

{
    // Error handling not yet implemented — see Epic #31
    //   Create with NULL config returns NULL
    //   Create with valid config returns non-NULL sender
    //   Destroy with NULL sender does nothing, does not crash
    //   Send called with NULL buffer does nothing, does not crash
    //   Send called with zero length does nothing, does not crash
    //   socket() returning -1 handled gracefully — Create returns NULL or Send is a no-op
    //   Unreachable host does not crash
    //
    // Address resolution strategy (getaddrinfo vs inet_pton, malloc policy, ADR) — see Story #34
}

// Destroy tests manage their own sender lifetime — no teardown Destroy needed.
// clang-format off
TEST_GROUP(SolidSyslogUdpSenderDestroy)
{
    struct SolidSyslogResolver* resolver = nullptr;
    struct SolidSyslogDatagram* datagram = nullptr;
    struct SolidSyslogUdpSenderConfig config;

    void setup() override
    {
        SocketFake_Reset();
        endpointGetHost = GetDefaultHost;
        endpointVersion = 0;
        endpointGetPort = GetDefaultPort;
        resolver        = SolidSyslogGetAddrInfoResolver_Create();
        datagram        = SolidSyslogPosixDatagram_Create();
        // cppcheck-suppress unreadVariable -- used in test bodies; cppcheck does not model CppUTest macros
        config = {resolver, datagram, TestEndpoint, TestEndpointVersion};
    }

    void teardown() override
    {
        SolidSyslogPosixDatagram_Destroy();
        SolidSyslogGetAddrInfoResolver_Destroy();
    }

    void CreateAndDestroy() const
    {
        SolidSyslogUdpSender_Create(&config);
        SolidSyslogUdpSender_Destroy();
    }
};

// clang-format on

TEST(SolidSyslogUdpSenderDestroy, DestroyWithoutSendDoesNotClose)
{
    CreateAndDestroy();
    CALLED_FAKE(SocketFake_Close, NEVER);
}

TEST(SolidSyslogUdpSenderDestroy, DestroyAfterSendClosesSocket)
{
    struct SolidSyslogSender* sender = SolidSyslogUdpSender_Create(&config);
    SolidSyslogSender_Send(sender, TEST_MESSAGE, TEST_MESSAGE_LEN);
    SolidSyslogUdpSender_Destroy();
    CALLED_FAKE(SocketFake_Close, ONCE);
    LONGS_EQUAL(SocketFake_SocketFd(), SocketFake_LastClosedFd());
}

TEST(SolidSyslogUdpSenderDestroy, DestroyAfterDisconnectDoesNotDoubleClose)
{
    struct SolidSyslogSender* sender = SolidSyslogUdpSender_Create(&config);
    SolidSyslogSender_Send(sender, TEST_MESSAGE, TEST_MESSAGE_LEN);
    SolidSyslogSender_Disconnect(sender);
    SolidSyslogUdpSender_Destroy();
    CALLED_FAKE(SocketFake_Close, ONCE);
}

TEST(SolidSyslogUdpSenderDestroy, SimpleScenario)
{
    struct SolidSyslogSender* sender = SolidSyslogUdpSender_Create(&config);
    SolidSyslogSender_Send(sender, TEST_MESSAGE, TEST_MESSAGE_LEN);
    SolidSyslogUdpSender_Destroy();

    CALLED_FAKE(SocketFake_Socket, ONCE);
    LONGS_EQUAL(AF_INET, SocketFake_SocketDomain());
    LONGS_EQUAL(SOCK_DGRAM, SocketFake_SocketType());
    CALLED_FAKE(SocketFake_Sendto, ONCE);
    LONGS_EQUAL(AF_INET, SocketFake_LastAddrFamily());
    LONGS_EQUAL(TEST_DEFAULT_PORT, SocketFake_LastPort());
    CALLED_FAKE(SocketFake_Close, ONCE);
}

// clang-format off
TEST_GROUP(SolidSyslogUdpSenderConfig)
{
    // cppcheck-suppress unreadVariable -- assigned in CreateSender; cppcheck does not model CppUTest macros
    const char* (*getHostFn)(void) = GetDefaultHost; // NOLINT(modernize-redundant-void-arg) -- C idiom
    // cppcheck-suppress unreadVariable -- assigned in CreateSender; cppcheck does not model CppUTest macros
    int (*getPortFn)(void) = GetDefaultPort; // NOLINT(modernize-redundant-void-arg) -- C idiom
    // cppcheck-suppress constVariablePointer -- Send requires non-const self; false positive from macro expansion
    // cppcheck-suppress unreadVariable -- used across TEST_GROUP methods; cppcheck does not model CppUTest macros
    struct SolidSyslogSender* sender = nullptr;

    void setup() override
    {
        SocketFake_Reset();
        SpyGetPortCallCount = 0;
        SpyGetHostCallCount = 0;
        endpointGetHost  = GetDefaultHost;
        endpointVersion  = 0;
        endpointGetPort  = GetDefaultPort;
    }

    void teardown() override
    {
        SolidSyslogUdpSender_Destroy();
        SolidSyslogPosixDatagram_Destroy();
        SolidSyslogGetAddrInfoResolver_Destroy();
    }

    void CreateSender()
    {
        endpointGetHost = getHostFn;
        endpointGetPort = getPortFn;
        struct SolidSyslogResolver*       resolver = SolidSyslogGetAddrInfoResolver_Create();
        struct SolidSyslogDatagram*       datagram = SolidSyslogPosixDatagram_Create();
        struct SolidSyslogUdpSenderConfig config   = {resolver, datagram, TestEndpoint, TestEndpointVersion};
        sender                                     = SolidSyslogUdpSender_Create(&config);
    }

    void Send() const
    {
        SolidSyslogSender_Send(sender, TEST_MESSAGE, TEST_MESSAGE_LEN);
    }
};

// clang-format on

TEST(SolidSyslogUdpSenderConfig, GetPortCalledOnFirstSend)
{
    getPortFn = SpyGetPort;
    CreateSender();
    CALLED_FUNCTION(SpyGetPort, NEVER);
    Send();
    CALLED_FUNCTION(SpyGetPort, ONCE);
}

TEST(SolidSyslogUdpSenderConfig, GetPortNotCalledOnSecondSend)
{
    getPortFn = SpyGetPort;
    CreateSender();
    Send();
    Send();
    CALLED_FUNCTION(SpyGetPort, ONCE);
}

TEST(SolidSyslogUdpSenderConfig, SendtoCalledWithConfiguredPort)
{
    getPortFn = GetAlternatePort;
    CreateSender();
    Send();
    LONGS_EQUAL(TEST_ALTERNATE_PORT, SocketFake_LastPort());
}

TEST(SolidSyslogUdpSenderConfig, GetHostCalledOnFirstSend)
{
    getHostFn = SpyGetHost;
    CreateSender();
    CALLED_FUNCTION(SpyGetHost, NEVER);
    Send();
    CALLED_FUNCTION(SpyGetHost, ONCE);
}

TEST(SolidSyslogUdpSenderConfig, GetHostNotCalledOnSecondSend)
{
    getHostFn = SpyGetHost;
    CreateSender();
    Send();
    Send();
    CALLED_FUNCTION(SpyGetHost, ONCE);
}

TEST(SolidSyslogUdpSenderConfig, GetAddrInfoCalledWithHostnameFromGetHost)
{
    getHostFn = SpyGetHost;
    CreateSender();
    Send();
    CALLED_FAKE(SocketFake_GetAddrInfo, ONCE);
    STRCMP_EQUAL(TEST_DEFAULT_HOST, SocketFake_LastGetAddrInfoHostname());
}

TEST(SolidSyslogUdpSenderConfig, SendtoCalledWithResolvedAddress)
{
    CreateSender();
    Send();
    STRCMP_EQUAL(TEST_DEFAULT_HOST, SocketFake_LastAddrAsString());
}

// clang-format off
TEST_GROUP(SolidSyslogUdpSenderFailure)
{
    struct SolidSyslogResolver*       resolver = nullptr;
    struct SolidSyslogDatagram*       datagram = nullptr;
    struct SolidSyslogUdpSenderConfig config;
    // cppcheck-suppress constVariablePointer -- Send requires non-const self; false positive from macro expansion
    // cppcheck-suppress unreadVariable -- assigned in CreateSender; cppcheck does not model CppUTest macros
    struct SolidSyslogSender*         sender = nullptr;

    void setup() override
    {
        SocketFake_Reset();
        endpointGetHost = GetDefaultHost;
        endpointVersion = 0;
        endpointGetPort = GetDefaultPort;
    }

    void teardown() override
    {
        SolidSyslogUdpSender_Destroy();
        SolidSyslogPosixDatagram_Destroy();
        SolidSyslogGetAddrInfoResolver_Destroy();
    }

    void CreateSender()
    {
        resolver = SolidSyslogGetAddrInfoResolver_Create();
        datagram = SolidSyslogPosixDatagram_Create();
        config   = {resolver, datagram, TestEndpoint, TestEndpointVersion};
        // cppcheck-suppress unreadVariable -- read by tests; cppcheck does not model CppUTest macros
        sender   = SolidSyslogUdpSender_Create(&config);
    }
};

// clang-format on

TEST(SolidSyslogUdpSenderFailure, SendReturnsFalseWhenResolverFails)
{
    SocketFake_SetGetAddrInfoFails(true);
    CreateSender();
    CHECK_FALSE(SolidSyslogSender_Send(sender, TEST_MESSAGE, TEST_MESSAGE_LEN));
}

TEST(SolidSyslogUdpSenderFailure, SendReturnsFalseWhenSocketFails)
{
    SocketFake_SetSocketFails(true);
    CreateSender();
    CHECK_FALSE(SolidSyslogSender_Send(sender, TEST_MESSAGE, TEST_MESSAGE_LEN));
}

TEST(SolidSyslogUdpSenderFailure, DoesNotResolveWhenSocketFails)
{
    SocketFake_SetSocketFails(true);
    CreateSender();
    SolidSyslogSender_Send(sender, TEST_MESSAGE, TEST_MESSAGE_LEN);
    CALLED_FAKE(SocketFake_GetAddrInfo, NEVER);
}

TEST(SolidSyslogUdpSenderFailure, SendDoesNotCallSendtoWhenResolverFailed)
{
    SocketFake_SetGetAddrInfoFails(true);
    CreateSender();
    SolidSyslogSender_Send(sender, TEST_MESSAGE, TEST_MESSAGE_LEN);
    CALLED_FAKE(SocketFake_Sendto, NEVER);
}

TEST(SolidSyslogUdpSenderFailure, SendReturnsTrueWhenResolverAndSocketSucceed)
{
    CreateSender();
    CHECK_TRUE(SolidSyslogSender_Send(sender, TEST_MESSAGE, TEST_MESSAGE_LEN));
}

TEST(SolidSyslogUdpSenderFailure, SendRecoversAfterTransientResolveFailure)
{
    SocketFake_SetGetAddrInfoFails(true);
    CreateSender();
    CHECK_FALSE(SolidSyslogSender_Send(sender, TEST_MESSAGE, TEST_MESSAGE_LEN));
    SocketFake_SetGetAddrInfoFails(false);
    CHECK_TRUE(SolidSyslogSender_Send(sender, TEST_MESSAGE, TEST_MESSAGE_LEN));
}

TEST(SolidSyslogUdpSenderFailure, NoEndpointConfiguredSendsToPortZero)
{
    resolver                                           = SolidSyslogGetAddrInfoResolver_Create();
    datagram                                           = SolidSyslogPosixDatagram_Create();
    struct SolidSyslogUdpSenderConfig configNoEndpoint = {resolver, datagram, nullptr, nullptr};
    sender                                             = SolidSyslogUdpSender_Create(&configNoEndpoint);
    SolidSyslogSender_Send(sender, TEST_MESSAGE, TEST_MESSAGE_LEN);
    CALLED_FAKE(SocketFake_Sendto, ONCE);
    LONGS_EQUAL(0, SocketFake_LastPort());
}

// clang-format off
TEST_GROUP(SolidSyslogUdpSenderRetry)
{
    struct SolidSyslogResolver* resolver = nullptr;
    struct SolidSyslogDatagram* datagram = nullptr;
    // cppcheck-suppress constVariablePointer -- Send requires non-const self; false positive from macro expansion
    // cppcheck-suppress unreadVariable -- used across TEST_GROUP methods; cppcheck does not model CppUTest macros
    struct SolidSyslogSender* sender = nullptr;

    void setup() override
    {
        SocketFake_Reset();
        endpointGetHost = GetDefaultHost;
        endpointVersion = 0;
        endpointGetPort = GetDefaultPort;
        resolver        = SolidSyslogGetAddrInfoResolver_Create();
        datagram        = DatagramFake_Create();
        struct SolidSyslogUdpSenderConfig config = {resolver, datagram, TestEndpoint, TestEndpointVersion};
        // cppcheck-suppress unreadVariable -- read by tests; cppcheck does not model CppUTest lifecycle
        sender = SolidSyslogUdpSender_Create(&config);
    }

    void teardown() override
    {
        SolidSyslogUdpSender_Destroy();
        DatagramFake_Destroy(datagram);
        SolidSyslogGetAddrInfoResolver_Destroy();
    }
};

// clang-format on

TEST(SolidSyslogUdpSenderRetry, SuccessfulSendDoesNotQueryMaxPayload)
{
    SolidSyslogSender_Send(sender, TEST_MESSAGE, TEST_MESSAGE_LEN);
    CALLED_FAKE_ON(DatagramFake_MaxPayload, datagram, NEVER);
}

TEST(SolidSyslogUdpSenderRetry, OversizeQueriesMaxPayloadAndRetries)
{
    DatagramFake_SetSendResult(datagram, 0, SOLIDSYSLOG_DATAGRAM_OVERSIZE);
    DatagramFake_SetSendResult(datagram, 1, SOLIDSYSLOG_DATAGRAM_SENT);
    DatagramFake_SetMaxPayload(datagram, 3);
    SolidSyslogSender_Send(sender, TEST_MESSAGE, TEST_MESSAGE_LEN);
    CALLED_FAKE_ON(DatagramFake_MaxPayload, datagram, ONCE);
    CALLED_FAKE_ON(DatagramFake_Send, datagram, TWICE);
}

TEST(SolidSyslogUdpSenderRetry, OversizeRetryTrimsBufferToMaxPayload)
{
    DatagramFake_SetSendResult(datagram, 0, SOLIDSYSLOG_DATAGRAM_OVERSIZE);
    DatagramFake_SetSendResult(datagram, 1, SOLIDSYSLOG_DATAGRAM_SENT);
    DatagramFake_SetMaxPayload(datagram, 3);
    SolidSyslogSender_Send(sender, TEST_MESSAGE, TEST_MESSAGE_LEN);
    LONGS_EQUAL(3, DatagramFake_SendSize(datagram, 1));
}

TEST(SolidSyslogUdpSenderRetry, OversizeRetrySucceedsReturnsTrue)
{
    DatagramFake_SetSendResult(datagram, 0, SOLIDSYSLOG_DATAGRAM_OVERSIZE);
    DatagramFake_SetSendResult(datagram, 1, SOLIDSYSLOG_DATAGRAM_SENT);
    DatagramFake_SetMaxPayload(datagram, 3);
    CHECK_TRUE(SolidSyslogSender_Send(sender, TEST_MESSAGE, TEST_MESSAGE_LEN));
}

/* Buffer "ab" + é (0xC3 0xA9): MaxPayload=3 cuts mid-é at the start byte,
 * walk-back drops the partial codepoint and trims to 2 bytes. */
TEST(SolidSyslogUdpSenderRetry, OversizeRetryWalksBackToCodepointBoundary)
{
    const uint8_t payload[] = {0x61, 0x62, 0xC3, 0xA9};
    DatagramFake_SetSendResult(datagram, 0, SOLIDSYSLOG_DATAGRAM_OVERSIZE);
    DatagramFake_SetSendResult(datagram, 1, SOLIDSYSLOG_DATAGRAM_SENT);
    DatagramFake_SetMaxPayload(datagram, 3);
    SolidSyslogSender_Send(sender, payload, sizeof(payload));
    LONGS_EQUAL(2, DatagramFake_SendSize(datagram, 1));
}

/* Double-OVERSIZE means the kernel disagreed with its own reported
 * MaxPayload — impossible-shouldn't-happen but if it did, returning
 * false would loop the buffered algorithm forever on an undeliverable.
 * Swallow: drop the message and return true so the caller moves on. */
TEST(SolidSyslogUdpSenderRetry, DoubleOversizeReturnsTrueToAvoidPermanentLoop)
{
    DatagramFake_SetSendResult(datagram, 0, SOLIDSYSLOG_DATAGRAM_OVERSIZE);
    DatagramFake_SetSendResult(datagram, 1, SOLIDSYSLOG_DATAGRAM_OVERSIZE);
    DatagramFake_SetMaxPayload(datagram, 3);
    CHECK_TRUE(SolidSyslogSender_Send(sender, TEST_MESSAGE, TEST_MESSAGE_LEN));
}

TEST(SolidSyslogUdpSenderRetry, DoubleOversizeDoesNotSendThird)
{
    DatagramFake_SetSendResult(datagram, 0, SOLIDSYSLOG_DATAGRAM_OVERSIZE);
    DatagramFake_SetSendResult(datagram, 1, SOLIDSYSLOG_DATAGRAM_OVERSIZE);
    DatagramFake_SetMaxPayload(datagram, 3);
    SolidSyslogSender_Send(sender, TEST_MESSAGE, TEST_MESSAGE_LEN);
    CALLED_FAKE_ON(DatagramFake_Send, datagram, TWICE);
}

TEST(SolidSyslogUdpSenderRetry, ZeroMaxPayloadSkipsRetrySend)
{
    DatagramFake_SetSendResult(datagram, 0, SOLIDSYSLOG_DATAGRAM_OVERSIZE);
    DatagramFake_SetMaxPayload(datagram, 0);
    SolidSyslogSender_Send(sender, TEST_MESSAGE, TEST_MESSAGE_LEN);
    CALLED_FAKE_ON(DatagramFake_Send, datagram, ONCE);
}

/* Trimmed length 0 means the message physically can't fit the path —
 * looping won't help, so we swallow and report success. The Buffered/
 * Service algorithm discards rather than retrying forever. */
TEST(SolidSyslogUdpSenderRetry, ZeroMaxPayloadReturnsTrueToAvoidPermanentLoop)
{
    DatagramFake_SetSendResult(datagram, 0, SOLIDSYSLOG_DATAGRAM_OVERSIZE);
    DatagramFake_SetMaxPayload(datagram, 0);
    CHECK_TRUE(SolidSyslogSender_Send(sender, TEST_MESSAGE, TEST_MESSAGE_LEN));
}

/* Retry sendto failing with non-OVERSIZE error (e.g. ECONNREFUSED on
 * connected UDP) is a TRANSIENT condition — return false so the
 * Buffered/Service algorithm keeps the message for retry. */
TEST(SolidSyslogUdpSenderRetry, RetryFailedNonOversizeReturnsFalse)
{
    DatagramFake_SetSendResult(datagram, 0, SOLIDSYSLOG_DATAGRAM_OVERSIZE);
    DatagramFake_SetSendResult(datagram, 1, SOLIDSYSLOG_DATAGRAM_FAILED);
    DatagramFake_SetMaxPayload(datagram, 3);
    CHECK_FALSE(SolidSyslogSender_Send(sender, TEST_MESSAGE, TEST_MESSAGE_LEN));
}

TEST(SolidSyslogUdpSenderRetry, MaxPayloadLargerThanMessageCapsTrimToMessageSize)
{
    /* MaxPayload > size can happen when the kernel reports EMSGSIZE for
     * a transient reason but the path MTU it surfaces is larger than
     * the original message. The trim must cap at the message size to
     * avoid reading past the buffer. */
    const uint8_t payload[] = {0x61, 0x62, 0x63};
    DatagramFake_SetSendResult(datagram, 0, SOLIDSYSLOG_DATAGRAM_OVERSIZE);
    DatagramFake_SetSendResult(datagram, 1, SOLIDSYSLOG_DATAGRAM_SENT);
    DatagramFake_SetMaxPayload(datagram, 9999);
    SolidSyslogSender_Send(sender, payload, sizeof(payload));
    LONGS_EQUAL(sizeof(payload), DatagramFake_SendSize(datagram, 1));
}

TEST(SolidSyslogUdpSenderRetry, NonOversizeFailureDoesNotRetry)
{
    DatagramFake_SetSendResult(datagram, 0, SOLIDSYSLOG_DATAGRAM_FAILED);
    SolidSyslogSender_Send(sender, TEST_MESSAGE, TEST_MESSAGE_LEN);
    CALLED_FAKE_ON(DatagramFake_Send, datagram, ONCE);
    CALLED_FAKE_ON(DatagramFake_MaxPayload, datagram, NEVER);
}

TEST(SolidSyslogUdpSenderRetry, NonOversizeFailureReturnsFalse)
{
    DatagramFake_SetSendResult(datagram, 0, SOLIDSYSLOG_DATAGRAM_FAILED);
    CHECK_FALSE(SolidSyslogSender_Send(sender, TEST_MESSAGE, TEST_MESSAGE_LEN));
}
