#include <netinet/in.h>
#include <stddef.h>
#include <sys/socket.h>
#include <array>
#include <cstdint>

#include "DatagramFake.h"
#include "ErrorHandlerFake.h"
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

class TEST_SolidSyslogUdpSenderRetry_DoubleOversizeDoesNotSendThird_Test;
class TEST_SolidSyslogUdpSenderRetry_NonOversizeFailureDoesNotRetry_Test;
class TEST_SolidSyslogUdpSenderRetry_OversizeQueriesMaxPayloadAndRetries_Test;
class TEST_SolidSyslogUdpSenderRetry_SuccessfulSendDoesNotQueryMaxPayload_Test;
class TEST_SolidSyslogUdpSenderRetry_ZeroMaxPayloadSkipsRetrySend_Test;

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

// Endpoint stubs — file-scope because TestEndpoint is a free function that
// the sender invokes via udp->config.Endpoint(). Tests mutate these globals
// between Sends to drive endpoint-changed and callback-spy scenarios; the
// TEST_BASE resets them in setup so groups don't leak state between tests.
static const char* (*endpointGetHost)() = GetDefaultHost;
static int (*endpointGetPort)() = GetDefaultPort;
static uint32_t endpointVersion = 0;

static void TestEndpoint(struct SolidSyslogEndpoint* endpoint)
{
    SolidSyslogFormatter_BoundedString(endpoint->Host, endpointGetHost(), SOLIDSYSLOG_MAX_HOST_SIZE);
    endpoint->Port = (uint16_t) endpointGetPort();
}

static uint32_t TestEndpointVersion() // NOLINT(modernize-use-trailing-return-type)
{
    return endpointVersion;
}

/* Shared fixture for every UdpSender test group. Lifts the SocketFake reset,
 * endpoint-stub reset, resolver/datagram create+destroy, and the Send helpers
 * out of every group. Derived groups choose between PosixDatagram (real) and
 * DatagramFake (Retry-only) and add any group-specific state. */
// clang-format off
TEST_BASE(UdpSenderTestBase)
{
    struct SolidSyslogResolver* resolver = nullptr;
    struct SolidSyslogDatagram* datagram = nullptr;
    SolidSyslogUdpSenderConfig  config{};
    struct SolidSyslogSender*   sender = nullptr;

    static void resetEndpointStubs()
    {
        endpointGetHost     = GetDefaultHost;
        endpointGetPort     = GetDefaultPort;
        endpointVersion     = 0;
        SpyGetHostCallCount = 0;
        SpyGetPortCallCount = 0;
    }

    void setupFakesWithPosixDatagram()
    {
        SocketFake_Reset();
        resetEndpointStubs();
        resolver = SolidSyslogGetAddrInfoResolver_Create();
        datagram = SolidSyslogPosixDatagram_Create();
        config   = {resolver, datagram, TestEndpoint, TestEndpointVersion};
    }

    void setupFakesWithDatagramFake()
    {
        SocketFake_Reset();
        resetEndpointStubs();
        resolver = SolidSyslogGetAddrInfoResolver_Create();
        datagram = DatagramFake_Create();
        config   = {resolver, datagram, TestEndpoint, TestEndpointVersion};
    }

    static void teardownFakesWithPosixDatagram()
    {
        SolidSyslogPosixDatagram_Destroy();
        SolidSyslogGetAddrInfoResolver_Destroy();
    }

    void teardownFakesWithDatagramFake() const
    {
        DatagramFake_Destroy(datagram);
        SolidSyslogGetAddrInfoResolver_Destroy();
    }

    // NOLINTNEXTLINE(modernize-use-nodiscard) -- many test bodies intentionally discard the return
    bool Send() const
    {
        return Send(TEST_MESSAGE, TEST_MESSAGE_LEN);
    }

    // NOLINTNEXTLINE(modernize-use-nodiscard) -- many test bodies intentionally discard the return
    bool Send(const void* buffer, size_t size) const
    {
        return SolidSyslogSender_Send(sender, buffer, size);
    }
};

// clang-format on

// clang-format off
TEST_GROUP_BASE(SolidSyslogUdpSender, UdpSenderTestBase)
{
    void setup() override
    {
        setupFakesWithPosixDatagram();
        sender = SolidSyslogUdpSender_Create(&config);
    }

    void teardown() override
    {
        SolidSyslogUdpSender_Destroy(sender);
        teardownFakesWithPosixDatagram();
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
    CHECK_TRUE(Send());
}

TEST(SolidSyslogUdpSender, SendReturnsFalseOnSendtoFailure)
{
    SocketFake_SetSendtoFails(true);
    CHECK_FALSE(Send());
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

TEST(SolidSyslogUdpSender, ZeroLengthSendPassesThrough)
{
    Send(TEST_MESSAGE, 0);
    CALLED_FAKE(SocketFake_Sendto, ONCE);
    LONGS_EQUAL(0, SocketFake_LastLen());
}

// Destroy tests manage their own sender lifetime — base teardown does
// not call _Destroy because tests already did.
// clang-format off
TEST_GROUP_BASE(SolidSyslogUdpSenderDestroy, UdpSenderTestBase)
{
    void setup() override
    {
        setupFakesWithPosixDatagram();
    }

    void teardown() override
    {
        teardownFakesWithPosixDatagram();
    }

    void CreateAndDestroy() const
    {
        struct SolidSyslogSender* localSender = SolidSyslogUdpSender_Create(&config);
        SolidSyslogUdpSender_Destroy(localSender);
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
    sender = SolidSyslogUdpSender_Create(&config);
    Send();
    SolidSyslogUdpSender_Destroy(sender);
    CALLED_FAKE(SocketFake_Close, ONCE);
    LONGS_EQUAL(SocketFake_SocketFd(), SocketFake_LastClosedFd());
}

TEST(SolidSyslogUdpSenderDestroy, DestroyAfterDisconnectDoesNotDoubleClose)
{
    sender = SolidSyslogUdpSender_Create(&config);
    Send();
    SolidSyslogSender_Disconnect(sender);
    SolidSyslogUdpSender_Destroy(sender);
    CALLED_FAKE(SocketFake_Close, ONCE);
}

TEST(SolidSyslogUdpSenderDestroy, SimpleScenario)
{
    sender = SolidSyslogUdpSender_Create(&config);
    Send();
    SolidSyslogUdpSender_Destroy(sender);

    CALLED_FAKE(SocketFake_Socket, ONCE);
    LONGS_EQUAL(AF_INET, SocketFake_SocketDomain());
    LONGS_EQUAL(SOCK_DGRAM, SocketFake_SocketType());
    CALLED_FAKE(SocketFake_Sendto, ONCE);
    LONGS_EQUAL(AF_INET, SocketFake_LastAddrFamily());
    LONGS_EQUAL(TEST_DEFAULT_PORT, SocketFake_LastPort());
    CALLED_FAKE(SocketFake_Close, ONCE);
}

// clang-format off
TEST_GROUP_BASE(SolidSyslogUdpSenderConfig, UdpSenderTestBase)
{
    void setup() override
    {
        setupFakesWithPosixDatagram();
        sender = SolidSyslogUdpSender_Create(&config);
    }

    void teardown() override
    {
        SolidSyslogUdpSender_Destroy(sender);
        teardownFakesWithPosixDatagram();
    }
};

// clang-format on

TEST(SolidSyslogUdpSenderConfig, GetPortCalledOnFirstSend)
{
    endpointGetPort = SpyGetPort;
    CALLED_FUNCTION(SpyGetPort, NEVER);
    Send();
    CALLED_FUNCTION(SpyGetPort, ONCE);
}

TEST(SolidSyslogUdpSenderConfig, GetPortNotCalledOnSecondSend)
{
    endpointGetPort = SpyGetPort;
    Send();
    Send();
    CALLED_FUNCTION(SpyGetPort, ONCE);
}

TEST(SolidSyslogUdpSenderConfig, SendtoCalledWithConfiguredPort)
{
    endpointGetPort = GetAlternatePort;
    Send();
    LONGS_EQUAL(TEST_ALTERNATE_PORT, SocketFake_LastPort());
}

TEST(SolidSyslogUdpSenderConfig, GetHostCalledOnFirstSend)
{
    endpointGetHost = SpyGetHost;
    CALLED_FUNCTION(SpyGetHost, NEVER);
    Send();
    CALLED_FUNCTION(SpyGetHost, ONCE);
}

TEST(SolidSyslogUdpSenderConfig, GetHostNotCalledOnSecondSend)
{
    endpointGetHost = SpyGetHost;
    Send();
    Send();
    CALLED_FUNCTION(SpyGetHost, ONCE);
}

TEST(SolidSyslogUdpSenderConfig, GetAddrInfoCalledWithHostnameFromGetHost)
{
    endpointGetHost = SpyGetHost;
    Send();
    CALLED_FAKE(SocketFake_GetAddrInfo, ONCE);
    STRCMP_EQUAL(TEST_DEFAULT_HOST, SocketFake_LastGetAddrInfoHostname());
}

TEST(SolidSyslogUdpSenderConfig, SendtoCalledWithResolvedAddress)
{
    Send();
    STRCMP_EQUAL(TEST_DEFAULT_HOST, SocketFake_LastAddrAsString());
}

// Failure tests defer sender creation so fault flags can be set first
// (resolver/datagram allocation doesn't trigger fakes; sockets are hit
// at Send time).
// clang-format off
TEST_GROUP_BASE(SolidSyslogUdpSenderFailure, UdpSenderTestBase)
{
    void setup() override
    {
        setupFakesWithPosixDatagram();
    }

    void teardown() override
    {
        SolidSyslogUdpSender_Destroy(sender);
        teardownFakesWithPosixDatagram();
    }

    void CreateSender()
    {
        sender = SolidSyslogUdpSender_Create(&config);
    }
};

// clang-format on

TEST(SolidSyslogUdpSenderFailure, SendReturnsFalseWhenResolverFails)
{
    SocketFake_SetGetAddrInfoFails(true);
    CreateSender();
    CHECK_FALSE(Send());
}

TEST(SolidSyslogUdpSenderFailure, SendReturnsFalseWhenSocketFails)
{
    SocketFake_SetSocketFails(true);
    CreateSender();
    CHECK_FALSE(Send());
}

TEST(SolidSyslogUdpSenderFailure, DoesNotResolveWhenSocketFails)
{
    SocketFake_SetSocketFails(true);
    CreateSender();
    Send();
    CALLED_FAKE(SocketFake_GetAddrInfo, NEVER);
}

TEST(SolidSyslogUdpSenderFailure, SendDoesNotCallSendtoWhenResolverFailed)
{
    SocketFake_SetGetAddrInfoFails(true);
    CreateSender();
    Send();
    CALLED_FAKE(SocketFake_Sendto, NEVER);
}

TEST(SolidSyslogUdpSenderFailure, SendReturnsTrueWhenResolverAndSocketSucceed)
{
    CreateSender();
    CHECK_TRUE(Send());
}

TEST(SolidSyslogUdpSenderFailure, SendRecoversAfterTransientResolveFailure)
{
    SocketFake_SetGetAddrInfoFails(true);
    CreateSender();
    CHECK_FALSE(Send());
    SocketFake_SetGetAddrInfoFails(false);
    CHECK_TRUE(Send());
}

// NOLINTBEGIN(cppcoreguidelines-macro-usage) -- macros preserve __FILE__/__LINE__ in test failure output
#define CALLED_DATAGRAM_SEND(times) CALLED_FAKE_ON(DatagramFake_Send, datagram, times)
#define CALLED_DATAGRAM_MAX_PAYLOAD(times) CALLED_FAKE_ON(DatagramFake_MaxPayload, datagram, times)
// NOLINTEND(cppcoreguidelines-macro-usage)

// clang-format off
TEST_GROUP_BASE(SolidSyslogUdpSenderRetry, UdpSenderTestBase)
{
    void setup() override
    {
        setupFakesWithDatagramFake();
        sender = SolidSyslogUdpSender_Create(&config);
    }

    void teardown() override
    {
        SolidSyslogUdpSender_Destroy(sender);
        teardownFakesWithDatagramFake();
    }

    void firstSendReturns(enum SolidSyslogDatagramSendResult result) const
    {
        DatagramFake_SetSendResult(datagram, 0, result);
    }

    void retrySendReturns(enum SolidSyslogDatagramSendResult result) const
    {
        DatagramFake_SetSendResult(datagram, 1, result);
    }

    void maxPayload(size_t bytes) const
    {
        DatagramFake_SetMaxPayload(datagram, bytes);
    }

    [[nodiscard]] size_t retrySendSize() const
    {
        return DatagramFake_SendSize(datagram, 1);
    }
};

// clang-format on

TEST(SolidSyslogUdpSenderRetry, SuccessfulSendDoesNotQueryMaxPayload)
{
    Send();
    CALLED_DATAGRAM_MAX_PAYLOAD(NEVER);
}

TEST(SolidSyslogUdpSenderRetry, OversizeQueriesMaxPayloadAndRetries)
{
    firstSendReturns(SOLIDSYSLOG_DATAGRAM_SEND_RESULT_OVERSIZE);
    retrySendReturns(SOLIDSYSLOG_DATAGRAM_SEND_RESULT_SENT);
    maxPayload(3);
    Send();
    CALLED_DATAGRAM_MAX_PAYLOAD(ONCE);
    CALLED_DATAGRAM_SEND(TWICE);
}

TEST(SolidSyslogUdpSenderRetry, OversizeRetryTrimsBufferToMaxPayload)
{
    firstSendReturns(SOLIDSYSLOG_DATAGRAM_SEND_RESULT_OVERSIZE);
    retrySendReturns(SOLIDSYSLOG_DATAGRAM_SEND_RESULT_SENT);
    maxPayload(3);
    Send();
    LONGS_EQUAL(3, retrySendSize());
}

TEST(SolidSyslogUdpSenderRetry, OversizeRetrySucceedsReturnsTrue)
{
    firstSendReturns(SOLIDSYSLOG_DATAGRAM_SEND_RESULT_OVERSIZE);
    retrySendReturns(SOLIDSYSLOG_DATAGRAM_SEND_RESULT_SENT);
    maxPayload(3);
    CHECK_TRUE(Send());
}

/* Buffer "ab" + é (0xC3 0xA9): MaxPayload=3 cuts mid-é at the start byte,
 * walk-back drops the partial codepoint and trims to 2 bytes. */
TEST(SolidSyslogUdpSenderRetry, OversizeRetryWalksBackToCodepointBoundary)
{
    const uint8_t payload[] = {0x61, 0x62, 0xC3, 0xA9};
    firstSendReturns(SOLIDSYSLOG_DATAGRAM_SEND_RESULT_OVERSIZE);
    retrySendReturns(SOLIDSYSLOG_DATAGRAM_SEND_RESULT_SENT);
    maxPayload(3);
    Send(payload, sizeof(payload));
    LONGS_EQUAL(2, retrySendSize());
}

/* Double-OVERSIZE means the kernel disagreed with its own reported
 * MaxPayload — impossible-shouldn't-happen but if it did, returning
 * false would loop the buffered algorithm forever on an undeliverable.
 * Swallow: drop the message and return true so the caller moves on. */
TEST(SolidSyslogUdpSenderRetry, DoubleOversizeReturnsTrueToAvoidPermanentLoop)
{
    firstSendReturns(SOLIDSYSLOG_DATAGRAM_SEND_RESULT_OVERSIZE);
    retrySendReturns(SOLIDSYSLOG_DATAGRAM_SEND_RESULT_OVERSIZE);
    maxPayload(3);
    CHECK_TRUE(Send());
}

TEST(SolidSyslogUdpSenderRetry, DoubleOversizeDoesNotSendThird)
{
    firstSendReturns(SOLIDSYSLOG_DATAGRAM_SEND_RESULT_OVERSIZE);
    retrySendReturns(SOLIDSYSLOG_DATAGRAM_SEND_RESULT_OVERSIZE);
    maxPayload(3);
    Send();
    CALLED_DATAGRAM_SEND(TWICE);
}

TEST(SolidSyslogUdpSenderRetry, ZeroMaxPayloadSkipsRetrySend)
{
    firstSendReturns(SOLIDSYSLOG_DATAGRAM_SEND_RESULT_OVERSIZE);
    maxPayload(0);
    Send();
    CALLED_DATAGRAM_SEND(ONCE);
}

/* Trimmed length 0 means the message physically can't fit the path —
 * looping won't help, so we swallow and report success. The Buffered/
 * Service algorithm discards rather than retrying forever. */
TEST(SolidSyslogUdpSenderRetry, ZeroMaxPayloadReturnsTrueToAvoidPermanentLoop)
{
    firstSendReturns(SOLIDSYSLOG_DATAGRAM_SEND_RESULT_OVERSIZE);
    maxPayload(0);
    CHECK_TRUE(Send());
}

/* Retry sendto failing with non-OVERSIZE error (e.g. ECONNREFUSED on
 * connected UDP) is a TRANSIENT condition — return false so the
 * Buffered/Service algorithm keeps the message for retry. */
TEST(SolidSyslogUdpSenderRetry, RetryFailedNonOversizeReturnsFalse)
{
    firstSendReturns(SOLIDSYSLOG_DATAGRAM_SEND_RESULT_OVERSIZE);
    retrySendReturns(SOLIDSYSLOG_DATAGRAM_SEND_RESULT_FAILED);
    maxPayload(3);
    CHECK_FALSE(Send());
}

TEST(SolidSyslogUdpSenderRetry, MaxPayloadLargerThanMessageCapsTrimToMessageSize)
{
    /* MaxPayload > size can happen when the kernel reports EMSGSIZE for
     * a transient reason but the path MTU it surfaces is larger than
     * the original message. The trim must cap at the message size to
     * avoid reading past the buffer. */
    const uint8_t payload[] = {0x61, 0x62, 0x63};
    firstSendReturns(SOLIDSYSLOG_DATAGRAM_SEND_RESULT_OVERSIZE);
    retrySendReturns(SOLIDSYSLOG_DATAGRAM_SEND_RESULT_SENT);
    maxPayload(9999);
    Send(payload, sizeof(payload));
    LONGS_EQUAL(sizeof(payload), retrySendSize());
}

TEST(SolidSyslogUdpSenderRetry, NonOversizeFailureDoesNotRetry)
{
    firstSendReturns(SOLIDSYSLOG_DATAGRAM_SEND_RESULT_FAILED);
    Send();
    CALLED_DATAGRAM_SEND(ONCE);
    CALLED_DATAGRAM_MAX_PAYLOAD(NEVER);
}

TEST(SolidSyslogUdpSenderRetry, NonOversizeFailureReturnsFalse)
{
    firstSendReturns(SOLIDSYSLOG_DATAGRAM_SEND_RESULT_FAILED);
    CHECK_FALSE(Send());
}

// clang-format off
TEST_GROUP_BASE(SolidSyslogUdpSenderBadSetup, UdpSenderTestBase)
{
    int sentinel = 0;

    void setup() override
    {
        setupFakesWithPosixDatagram();
        ErrorHandlerFake_Install(&sentinel);
    }

    void teardown() override
    {
        SolidSyslogUdpSender_Destroy(sender);
        teardownFakesWithPosixDatagram();
        ErrorHandlerFake_Uninstall();
    }
};

// clang-format on

TEST(SolidSyslogUdpSenderBadSetup, CreateWithNullConfigReportsError)
{
    SolidSyslogUdpSender_Create(nullptr);
    CHECK_REPORTED_ERROR("SolidSyslogUdpSender_Create called with NULL config");
}

TEST(SolidSyslogUdpSenderBadSetup, SendOnBadSetupSenderReturnsTrue)
{
    sender = SolidSyslogUdpSender_Create(nullptr);
    CHECK_TRUE(Send());
}

TEST(SolidSyslogUdpSenderBadSetup, DisconnectOnBadSetupSenderDoesNotCrash)
{
    sender = SolidSyslogUdpSender_Create(nullptr);
    SolidSyslogSender_Disconnect(sender);
}

TEST(SolidSyslogUdpSenderBadSetup, CreateWithNullResolverReportsError)
{
    config.Resolver = nullptr;
    SolidSyslogUdpSender_Create(&config);
    CHECK_REPORTED_ERROR("SolidSyslogUdpSender_Create config.Resolver is NULL");
}

TEST(SolidSyslogUdpSenderBadSetup, CreateWithNullDatagramReportsError)
{
    config.Datagram = nullptr;
    SolidSyslogUdpSender_Create(&config);
    CHECK_REPORTED_ERROR("SolidSyslogUdpSender_Create config.Datagram is NULL");
}

TEST(SolidSyslogUdpSenderBadSetup, CreateWithNullEndpointReportsError)
{
    config.Endpoint = nullptr;
    SolidSyslogUdpSender_Create(&config);
    CHECK_REPORTED_ERROR("SolidSyslogUdpSender_Create config.Endpoint is NULL");
}

TEST(SolidSyslogUdpSenderBadSetup, NullEndpointVersionIsOptional)
{
    config.EndpointVersion = nullptr;
    sender = SolidSyslogUdpSender_Create(&config);
    CHECK_NOTHING_REPORTED();
    CHECK_TRUE(Send());
}

TEST(SolidSyslogUdpSenderBadSetup, SendWithNullBufferReportsErrorAndDoesNotSend)
{
    sender = SolidSyslogUdpSender_Create(&config);
    CHECK_FALSE(Send(nullptr, 5));
    CHECK_REPORTED_ERROR("SolidSyslogUdpSender_Send called with NULL buffer");
    CALLED_FAKE(SocketFake_Sendto, NEVER);
}

// Pool tests — prove SOLIDSYSLOG_UDP_SENDER_POOL_SIZE caps live instances
// and overflow falls back to the shared SolidSyslogNullSender. Generic
// pool mechanics (lock counts, per-probe locking, stale-handle warning)
// are covered by SolidSyslogPoolAllocatorTest.cpp.

// clang-format off
TEST_GROUP_BASE(SolidSyslogUdpSenderPool, UdpSenderTestBase)
{
    struct SolidSyslogSender* pooled[SOLIDSYSLOG_UDP_SENDER_POOL_SIZE] = {};
    struct SolidSyslogSender* overflow                                  = nullptr;

    void setup() override
    {
        setupFakesWithPosixDatagram();
    }

    void teardown() override
    {
        for (auto* handle : pooled)
        {
            if (handle != nullptr)
            {
                SolidSyslogUdpSender_Destroy(handle);
            }
        }
        if (overflow != nullptr)
        {
            SolidSyslogUdpSender_Destroy(overflow);
        }
        teardownFakesWithPosixDatagram();
    }

    struct SolidSyslogSender* MakeSender()
    {
        return SolidSyslogUdpSender_Create(&config);
    }

    void FillPool()
    {
        for (auto*& slot : pooled)
        {
            slot = MakeSender();
        }
    }
};

// clang-format on

TEST(SolidSyslogUdpSenderPool, FillingPoolThenOverflowReturnsDistinctFallback)
{
    FillPool();

    overflow = MakeSender();

    CHECK_TEXT(overflow != nullptr, "Fallback handle was nullptr");
    for (auto* slot : pooled)
    {
        CHECK_TEXT(slot != nullptr, "pool slot was nullptr (FillPool failed?)");
        CHECK_TEXT(overflow != slot, "Fallback handle collided with a pool slot");
    }
}
