#include <stddef.h>
#include <stdint.h>

#include "TestUtils.h"
#include "CppUTest/TestHarness.h"

using namespace CososoTesting;

#include "ConfigLockFake.h"
#include "ErrorHandlerFake.h"
#include "FreeRTOS.h"
#include "FreeRTOS_ARP.h"
#include "FreeRTOS_IP.h"
#include "FreeRTOS_Sockets.h"
#include "FreeRtosArpFake.h"
#include "FreeRtosSocketsFake.h"
#include "FreeRtosTaskFake.h"
#include "SolidSyslogDatagram.h"
#include "SolidSyslogDatagramCategories.h"
#include "SolidSyslogDatagramDefinition.h"
#include "SolidSyslogDatagramErrors.h"
#include "SolidSyslogErrorCategory.h"
#include "SolidSyslogPlusTcpAddress.h"
#include "SolidSyslogPlusTcpAddressPrivate.h"
#include "SolidSyslogPlusTcpDatagram.h"
#include "SolidSyslogPlusTcpDatagramErrors.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogTunables.h"
#include "SolidSyslogError.h"

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

static const uint16_t TEST_PORT = 514;

static size_t FakeGetPayloadLimit_ReturnValue;
static void* FakeGetPayloadLimit_LastContext;

static void FakeGetPayloadLimit_Reset()
{
    FakeGetPayloadLimit_ReturnValue = 0U;
    FakeGetPayloadLimit_LastContext = nullptr;
}

static size_t FakeGetPayloadLimit(void* context)
{
    FakeGetPayloadLimit_LastContext = context;
    return FakeGetPayloadLimit_ReturnValue;
}

TEST_GROUP(SolidSyslogPlusTcpDatagram)
{
    struct SolidSyslogDatagram* datagram = nullptr;
    struct SolidSyslogAddress* addr = nullptr;

    void setup() override
    {
        FreeRtosSocketsFake_Reset();
        FreeRtosArpFake_Reset();
        FreeRtosTaskFake_Reset();
        FreeRtosArpFake_SetCacheHit(true);
        datagram = SolidSyslogPlusTcpDatagram_Create(nullptr);
        addr = SolidSyslogPlusTcpAddress_Create();
        struct freertos_sockaddr* sin = SolidSyslogPlusTcpAddress_AsFreertosSockaddr(addr);
        sin->sin_family = FREERTOS_AF_INET;
        sin->sin_port = FreeRTOS_htons(TEST_PORT);
        sin->sin_address.ulIP_IPv4 = FreeRTOS_inet_addr_quick(127, 0, 0, 1);
    }

    void teardown() override
    {
        SolidSyslogPlusTcpAddress_Destroy(addr);
        SolidSyslogPlusTcpDatagram_Destroy(datagram);
    }

    /* Replace the fixture's datagram with one configured to ask the fake
       getter, with the fake's state cleared. */
    void recreateWithGetter(void* context)
    {
        FakeGetPayloadLimit_Reset();
        const struct SolidSyslogPlusTcpDatagramConfig tuned = {FakeGetPayloadLimit, context};
        SolidSyslogPlusTcpDatagram_Destroy(datagram);
        datagram = SolidSyslogPlusTcpDatagram_Create(&tuned);
    }

    void openAndSendOnce() const
    {
        SolidSyslogDatagram_Open(datagram);
        SolidSyslogDatagram_SendTo(datagram, "x", 1, addr);
    }

    /* The first send to a next hop the stack never resolves. */
    void openAndSendUnresolved() const
    {
        FreeRtosArpFake_SetCacheHit(false);
        openAndSendOnce();
    }
};

TEST(SolidSyslogPlusTcpDatagram, CreateReturnsNonNullDatagram)

{
    CHECK(datagram != nullptr);
}

TEST(SolidSyslogPlusTcpDatagram, OpenCreatesUdpSocket)

{
    SolidSyslogDatagram_Open(datagram);
    CALLED_FAKE(FreeRtosSocketsFake_Socket, ONCE);
    LONGS_EQUAL(FREERTOS_AF_INET, FreeRtosSocketsFake_LastSocketDomain());
    LONGS_EQUAL(FREERTOS_SOCK_DGRAM, FreeRtosSocketsFake_LastSocketType());
    LONGS_EQUAL(FREERTOS_IPPROTO_UDP, FreeRtosSocketsFake_LastSocketProtocol());
}

TEST(SolidSyslogPlusTcpDatagram, OpenReturnsTrueOnSuccess)

{
    CHECK_TRUE(SolidSyslogDatagram_Open(datagram));
}

TEST(SolidSyslogPlusTcpDatagram, OpenReturnsFalseWhenSocketFails)

{
    FreeRtosSocketsFake_SetSocketFails(true);
    CHECK_FALSE(SolidSyslogDatagram_Open(datagram));
}

TEST(SolidSyslogPlusTcpDatagram, OpenIsIdempotent)

{
    SolidSyslogDatagram_Open(datagram);
    CHECK_TRUE(SolidSyslogDatagram_Open(datagram));
    CALLED_FAKE(FreeRtosSocketsFake_Socket, ONCE);
}

TEST(SolidSyslogPlusTcpDatagram, MaxPayloadIsTheTunableWhenTheIntegratorInstallsNoGetter)
{
    LONGS_EQUAL(SOLIDSYSLOG_UDP_IPV4_PAYLOAD_LIMIT, SolidSyslogDatagram_MaxPayload(datagram));
}

TEST(SolidSyslogPlusTcpDatagram, MaxPayloadIsWhatTheGetterStates)
{
    recreateWithGetter(nullptr);
    FakeGetPayloadLimit_ReturnValue = 1472U;

    LONGS_EQUAL(1472, SolidSyslogDatagram_MaxPayload(datagram));
}

/* The getter is asked on every query, never cached, so an end user can retune
   the limit at run time without destroying the datagram. */
TEST(SolidSyslogPlusTcpDatagram, MaxPayloadAsksTheGetterOnEveryQuery)
{
    recreateWithGetter(nullptr);
    FakeGetPayloadLimit_ReturnValue = 1472U;
    (void) SolidSyslogDatagram_MaxPayload(datagram);
    FakeGetPayloadLimit_ReturnValue = 1400U;

    LONGS_EQUAL(1400, SolidSyslogDatagram_MaxPayload(datagram));
}

TEST(SolidSyslogPlusTcpDatagram, MaxPayloadFallsToTheTunableWhenTheGetterAnswersZero)
{
    recreateWithGetter(nullptr);
    FakeGetPayloadLimit_ReturnValue = 0U;

    LONGS_EQUAL(SOLIDSYSLOG_UDP_IPV4_PAYLOAD_LIMIT, SolidSyslogDatagram_MaxPayload(datagram));
}

TEST(SolidSyslogPlusTcpDatagram, MaxPayloadPassesTheContextToTheGetter)
{
    int context = 0;
    recreateWithGetter(&context);
    FakeGetPayloadLimit_ReturnValue = 1472U;

    (void) SolidSyslogDatagram_MaxPayload(datagram);
    POINTERS_EQUAL(&context, FakeGetPayloadLimit_LastContext);
}

TEST(SolidSyslogPlusTcpDatagram, SendToFailsBeforeOpen)

{
    enum SolidSyslogDatagramSendResult result = SolidSyslogDatagram_SendTo(datagram, "x", 1, addr);
    LONGS_EQUAL(SOLIDSYSLOG_DATAGRAM_SEND_RESULT_FAILED, result);
    CALLED_FAKE(FreeRtosSocketsFake_Sendto, NEVER);
}

TEST(SolidSyslogPlusTcpDatagram, SendToFailsWhenSendtoErrors)

{
    SolidSyslogDatagram_Open(datagram);
    FreeRtosSocketsFake_SetSendtoFails(true);
    LONGS_EQUAL(SOLIDSYSLOG_DATAGRAM_SEND_RESULT_FAILED, SolidSyslogDatagram_SendTo(datagram, "x", 1, addr));
}

TEST(SolidSyslogPlusTcpDatagram, CloseClosesSocket)

{
    SolidSyslogDatagram_Open(datagram);
    SolidSyslogDatagram_Close(datagram);
    CALLED_FAKE(FreeRtosSocketsFake_Closesocket, ONCE);
    POINTERS_EQUAL(FreeRtosSocketsFake_LastSocketReturned(), FreeRtosSocketsFake_LastClosesocketSocket());
}

TEST(SolidSyslogPlusTcpDatagram, SendToFailsAfterClose)

{
    SolidSyslogDatagram_Open(datagram);
    SolidSyslogDatagram_Close(datagram);
    enum SolidSyslogDatagramSendResult result = SolidSyslogDatagram_SendTo(datagram, "x", 1, addr);
    LONGS_EQUAL(SOLIDSYSLOG_DATAGRAM_SEND_RESULT_FAILED, result);
    CALLED_FAKE(FreeRtosSocketsFake_Sendto, NEVER);
}

TEST(SolidSyslogPlusTcpDatagram, DestroyClosesOpenSocket)

{
    SolidSyslogDatagram_Open(datagram);
    SolidSyslogPlusTcpDatagram_Destroy(datagram);
    datagram = nullptr;
    CALLED_FAKE(FreeRtosSocketsFake_Closesocket, ONCE);
}

TEST(SolidSyslogPlusTcpDatagram, CloseIsIdempotent)

{
    SolidSyslogDatagram_Open(datagram);
    SolidSyslogDatagram_Close(datagram);
    SolidSyslogDatagram_Close(datagram);
    CALLED_FAKE(FreeRtosSocketsFake_Closesocket, ONCE);
}

TEST(SolidSyslogPlusTcpDatagram, CloseWithoutOpenIsNoOp)

{
    SolidSyslogDatagram_Close(datagram);
    CALLED_FAKE(FreeRtosSocketsFake_Closesocket, NEVER);
}

TEST(SolidSyslogPlusTcpDatagram, DestroyAfterCloseDoesNotCloseAgain)

{
    SolidSyslogDatagram_Open(datagram);
    SolidSyslogDatagram_Close(datagram);
    SolidSyslogPlusTcpDatagram_Destroy(datagram);
    datagram = nullptr;
    CALLED_FAKE(FreeRtosSocketsFake_Closesocket, ONCE);
}

TEST(SolidSyslogPlusTcpDatagram, SendToSendsBufferToDestinationAfterOpen)

{
    static const char TEST_MESSAGE[] = "hello";
    static const size_t TEST_MESSAGE_LEN = sizeof(TEST_MESSAGE) - 1;

    SolidSyslogDatagram_Open(datagram);
    enum SolidSyslogDatagramSendResult result =
        SolidSyslogDatagram_SendTo(datagram, TEST_MESSAGE, TEST_MESSAGE_LEN, addr);

    LONGS_EQUAL(SOLIDSYSLOG_DATAGRAM_SEND_RESULT_SENT, result);
    CALLED_FAKE(FreeRtosSocketsFake_Sendto, ONCE);
    POINTERS_EQUAL(FreeRtosSocketsFake_LastSocketReturned(), FreeRtosSocketsFake_LastSendtoSocket());
    POINTERS_EQUAL(TEST_MESSAGE, FreeRtosSocketsFake_LastSendtoBuffer());
    LONGS_EQUAL(TEST_MESSAGE_LEN, FreeRtosSocketsFake_LastSendtoLength());
    LONGS_EQUAL(0, FreeRtosSocketsFake_LastSendtoFlags());
    POINTERS_EQUAL(
        SolidSyslogPlusTcpAddress_AsConstFreertosSockaddr(addr),
        FreeRtosSocketsFake_LastSendtoDestination()
    );
    LONGS_EQUAL(sizeof(struct freertos_sockaddr), FreeRtosSocketsFake_LastSendtoDestinationLength());
}

TEST(SolidSyslogPlusTcpDatagram, SendToLooksUpTheNextHopForTheDestination)
{
    openAndSendOnce();

    CALLED_FAKE(FreeRtosArpFake_GetCacheEntry, ONCE);
    LONGS_EQUAL(FreeRTOS_inet_addr_quick(127, 0, 0, 1), FreeRtosArpFake_LastGetCacheEntryArg());
}

TEST(SolidSyslogPlusTcpDatagram, SendToFiresArpProbeOnCacheMiss)
{
    openAndSendUnresolved();

    CALLED_FAKE(FreeRtosArpFake_OutputArpRequest, ONCE);
    LONGS_EQUAL(FreeRTOS_inet_addr_quick(127, 0, 0, 1), FreeRtosArpFake_LastOutputArpRequestArg());
}

TEST(SolidSyslogPlusTcpDatagram, SendToChecksForTheReplyEveryTick)
{
    openAndSendUnresolved();

    LONGS_EQUAL(1, FreeRtosTaskFake_LastVTaskDelayTicks());
}

TEST(SolidSyslogPlusTcpDatagram, SendToKeepsCheckingForTheResolveWait)
{
    openAndSendUnresolved();

    LONGS_EQUAL(pdMS_TO_TICKS(SOLIDSYSLOG_DATAGRAM_RESOLVE_WAIT_MS), FreeRtosTaskFake_VTaskDelayCallCount());
}

TEST(SolidSyslogPlusTcpDatagram, SendToReportsFailedWhenTheNextHopDoesNotResolve)
{
    FreeRtosArpFake_SetCacheHit(false);
    SolidSyslogDatagram_Open(datagram);

    LONGS_EQUAL(SOLIDSYSLOG_DATAGRAM_SEND_RESULT_FAILED, SolidSyslogDatagram_SendTo(datagram, "x", 1, addr));
}

TEST(SolidSyslogPlusTcpDatagram, SendToSendsOnceTheNextHopResolvesDuringTheWait)
{
    FreeRtosArpFake_SetLookupsUntilHit(3);
    SolidSyslogDatagram_Open(datagram);

    LONGS_EQUAL(SOLIDSYSLOG_DATAGRAM_SEND_RESULT_SENT, SolidSyslogDatagram_SendTo(datagram, "x", 1, addr));
    CALLED_FAKE(FreeRtosSocketsFake_Sendto, ONCE);
    LONGS_EQUAL(3, FreeRtosTaskFake_VTaskDelayCallCount());
}

TEST(SolidSyslogPlusTcpDatagram, SendToSendsNothingWhenTheNextHopDoesNotResolve)
{
    openAndSendUnresolved();

    CALLED_FAKE(FreeRtosSocketsFake_Sendto, NEVER);
}

TEST(SolidSyslogPlusTcpDatagram, AFailedResolutionReportsTheUnresolvedNextHopAndTheLookupResult)
{
    ErrorHandlerFake_Install(nullptr);
    openAndSendUnresolved();

    CALLED_FAKE(ErrorHandlerFake_Handle, TWICE);
    CHECK_ERROR_EVENT_AT(
        0,
        SOLIDSYSLOG_SEVERITY_WARNING,
        &SolidSyslogPlusTcpDatagramErrorSource,
        SOLIDSYSLOG_CAT_DATAGRAM_NEXT_HOP_UNRESOLVED,
        SOLIDSYSLOG_DATAGRAM_ERROR_NEXT_HOP_UNRESOLVED
    );
    CHECK_ERROR_EVENT_AT(
        1,
        SOLIDSYSLOG_SEVERITY_WARNING,
        &SolidSyslogPlusTcpDatagramErrorSource,
        SOLIDSYSLOG_CAT_NATIVE_ERROR,
        eARPCacheMiss
    );
}

TEST(SolidSyslogPlusTcpDatagram, WhileResolutionKeepsFailingASendDoesNotWait)
{
    openAndSendUnresolved();

    SolidSyslogDatagram_SendTo(datagram, "x", 1, addr);

    LONGS_EQUAL(pdMS_TO_TICKS(SOLIDSYSLOG_DATAGRAM_RESOLVE_WAIT_MS), FreeRtosTaskFake_VTaskDelayCallCount());
}

TEST(SolidSyslogPlusTcpDatagram, WhileResolutionKeepsFailingASendStillAsksForTheNextHop)
{
    openAndSendUnresolved();

    SolidSyslogDatagram_SendTo(datagram, "x", 1, addr);

    CALLED_FAKE(FreeRtosArpFake_OutputArpRequest, TWICE);
}

TEST(SolidSyslogPlusTcpDatagram, OnceAResolutionSucceedsAgainTheNextWaits)
{
    openAndSendUnresolved();
    FreeRtosArpFake_SetCacheHit(true);
    SolidSyslogDatagram_SendTo(datagram, "x", 1, addr);

    FreeRtosArpFake_SetCacheHit(false);
    SolidSyslogDatagram_SendTo(datagram, "x", 1, addr);

    LONGS_EQUAL(2 * pdMS_TO_TICKS(SOLIDSYSLOG_DATAGRAM_RESOLVE_WAIT_MS), FreeRtosTaskFake_VTaskDelayCallCount());
}

TEST(SolidSyslogPlusTcpDatagram, AnEntryStillWaitingForItsReplyIsWaitedForAndAskedAbout)
{
    FreeRtosArpFake_SetCacheHit(false);
    FreeRtosArpFake_SetReplyPending(true);
    SolidSyslogDatagram_Open(datagram);

    LONGS_EQUAL(SOLIDSYSLOG_DATAGRAM_SEND_RESULT_FAILED, SolidSyslogDatagram_SendTo(datagram, "x", 1, addr));
    LONGS_EQUAL(pdMS_TO_TICKS(SOLIDSYSLOG_DATAGRAM_RESOLVE_WAIT_MS), FreeRtosTaskFake_VTaskDelayCallCount());
    CALLED_FAKE(FreeRtosArpFake_OutputArpRequest, ONCE);
}

TEST(SolidSyslogPlusTcpDatagram, AnEntryStillWaitingForItsReplyIsReportedAsPending)
{
    ErrorHandlerFake_Install(nullptr);
    FreeRtosArpFake_SetCacheHit(false);
    FreeRtosArpFake_SetReplyPending(true);

    openAndSendOnce();

    CHECK_ERROR_EVENT_AT(
        1,
        SOLIDSYSLOG_SEVERITY_WARNING,
        &SolidSyslogPlusTcpDatagramErrorSource,
        SOLIDSYSLOG_CAT_NATIVE_ERROR,
        eCantSendPacket
    );
}

TEST(SolidSyslogPlusTcpDatagram, ADestinationOffTheSubnetAsksForTheGatewayTheStackNames)
{
    FreeRtosArpFake_SetGateway(FreeRTOS_inet_addr_quick(10, 0, 2, 2));
    openAndSendUnresolved();

    LONGS_EQUAL(FreeRTOS_inet_addr_quick(10, 0, 2, 2), FreeRtosArpFake_LastOutputArpRequestArg());
}

TEST(SolidSyslogPlusTcpDatagram, AFailureStraightAfterAnotherIsNotReportedAgain)
{
    ErrorHandlerFake_Install(nullptr);
    openAndSendUnresolved();

    SolidSyslogDatagram_SendTo(datagram, "x", 1, addr);

    CALLED_FAKE(ErrorHandlerFake_Handle, TWICE);
}

TEST(SolidSyslogPlusTcpDatagram, AFailureAfterARecoveryIsReportedAgain)
{
    ErrorHandlerFake_Install(nullptr);
    openAndSendUnresolved();
    FreeRtosArpFake_SetCacheHit(true);
    SolidSyslogDatagram_SendTo(datagram, "x", 1, addr);

    FreeRtosArpFake_SetCacheHit(false);
    SolidSyslogDatagram_SendTo(datagram, "x", 1, addr);

    LONGS_EQUAL(4, ErrorHandlerFake_HandleCallCount());
}

TEST(SolidSyslogPlusTcpDatagram, SendToSkipsArpProbeAndYieldOnCacheHit)
{
    openAndSendOnce();

    CALLED_FAKE(FreeRtosArpFake_OutputArpRequest, NEVER);
    CALLED_FAKE(FreeRtosTaskFake_VTaskDelay, NEVER);
    CALLED_FAKE(FreeRtosSocketsFake_Sendto, ONCE);
}

TEST(SolidSyslogPlusTcpDatagram, SendToReChecksArpCacheOnEachCall)

{
    SolidSyslogDatagram_Open(datagram);

    FreeRtosArpFake_SetCacheHit(true);
    SolidSyslogDatagram_SendTo(datagram, "x", 1, addr);
    CALLED_FAKE(FreeRtosArpFake_OutputArpRequest, NEVER);

    FreeRtosArpFake_SetCacheHit(false);
    SolidSyslogDatagram_SendTo(datagram, "x", 1, addr);
    CALLED_FAKE(FreeRtosArpFake_OutputArpRequest, ONCE);

    FreeRtosArpFake_SetCacheHit(true);
    SolidSyslogDatagram_SendTo(datagram, "x", 1, addr);
    CALLED_FAKE(FreeRtosArpFake_OutputArpRequest, ONCE);
}

TEST(SolidSyslogPlusTcpDatagram, SendToForwardsLengthVerbatim)

{
    static const char TEST_BUFFER[1024] = {};
    SolidSyslogDatagram_Open(datagram);

    SolidSyslogDatagram_SendTo(datagram, TEST_BUFFER, 1, addr);
    LONGS_EQUAL(1, FreeRtosSocketsFake_LastSendtoLength());

    SolidSyslogDatagram_SendTo(datagram, TEST_BUFFER, 1232, addr);
    LONGS_EQUAL(1232, FreeRtosSocketsFake_LastSendtoLength());
}

// clang-format off
TEST_GROUP(SolidSyslogPlusTcpDatagramPool)
{
    struct SolidSyslogDatagram* pooled[SOLIDSYSLOG_DATAGRAM_POOL_SIZE] = {};
    struct SolidSyslogDatagram* overflow                                         = nullptr;

    void setup() override
    {
        FreeRtosSocketsFake_Reset();
    }

    void teardown() override
    {
        for (auto* handle : pooled)
        {
            if (handle != nullptr)
            {
                SolidSyslogPlusTcpDatagram_Destroy(handle);
            }
        }
        if (overflow != nullptr)
        {
            SolidSyslogPlusTcpDatagram_Destroy(overflow);
        }
        ConfigLockFake_Uninstall();
    }

    void FillPool()
    {
        for (auto*& slot : pooled)
        {
            slot = SolidSyslogPlusTcpDatagram_Create(nullptr);
        }
    }
};

// clang-format on

TEST(SolidSyslogPlusTcpDatagramPool, FillingPoolThenOverflowReturnsDistinctFallback)

{
    FillPool();

    overflow = SolidSyslogPlusTcpDatagram_Create(nullptr);

    CHECK_IS_FALLBACK(overflow, pooled);
}

TEST(SolidSyslogPlusTcpDatagramPool, ExhaustedCreateReportsError)

{
    ErrorHandlerFake_Install(nullptr);
    FillPool();

    overflow = SolidSyslogPlusTcpDatagram_Create(nullptr);

    CHECK_ERROR_REPORTED_ONCE(
        SOLIDSYSLOG_SEVERITY_CRITICAL,
        &SolidSyslogPlusTcpDatagramErrorSource,
        SOLIDSYSLOG_CAT_POOL_EXHAUSTED,
        SOLIDSYSLOG_DATAGRAM_ERROR_POOL_EXHAUSTED
    );
}

TEST(SolidSyslogPlusTcpDatagramPool, FallbackVtableMethodsAreNoOps)

{
    FillPool();
    overflow = SolidSyslogPlusTcpDatagram_Create(nullptr);
    struct SolidSyslogAddress* localAddr = SolidSyslogPlusTcpAddress_Create();
    FreeRtosSocketsFake_Reset();

    /* NullDatagram's Open returns true so caller success paths are not
     * tripped; no underlying FreeRTOS_socket is created. */
    CHECK_TRUE(SolidSyslogDatagram_Open(overflow));
    SolidSyslogDatagram_SendTo(overflow, "x", 1, localAddr);
    SolidSyslogDatagram_Close(overflow);

    SolidSyslogPlusTcpAddress_Destroy(localAddr);

    CALLED_FAKE(FreeRtosSocketsFake_Socket, NEVER);
    CALLED_FAKE(FreeRtosSocketsFake_Sendto, NEVER);
    CALLED_FAKE(FreeRtosSocketsFake_Closesocket, NEVER);
}

TEST(SolidSyslogPlusTcpDatagramPool, CreateAcquiresAndReleasesConfigLockOnFirstFreeSlot)

{
    ConfigLockFake_Install();

    pooled[0] = SolidSyslogPlusTcpDatagram_Create(nullptr);

    CALLED_FAKE(ConfigLockFake_Lock, ONCE);
    CALLED_FAKE(ConfigLockFake_Unlock, ONCE);
}

TEST(SolidSyslogPlusTcpDatagramPool, CreateLocksOncePerSlotProbedWhenPoolIsFull)

{
    FillPool();
    ConfigLockFake_Install();

    overflow = SolidSyslogPlusTcpDatagram_Create(nullptr);

    LONGS_EQUAL(SOLIDSYSLOG_DATAGRAM_POOL_SIZE, ConfigLockFake_LockCallCount());
    LONGS_EQUAL(SOLIDSYSLOG_DATAGRAM_POOL_SIZE, ConfigLockFake_UnlockCallCount());
}

TEST(SolidSyslogPlusTcpDatagramPool, DestroyOfPooledHandleLocksOnce)

{
    pooled[0] = SolidSyslogPlusTcpDatagram_Create(nullptr);
    ConfigLockFake_Install();

    SolidSyslogPlusTcpDatagram_Destroy(pooled[0]);
    pooled[0] = nullptr;

    CALLED_FAKE(ConfigLockFake_Lock, ONCE);
    CALLED_FAKE(ConfigLockFake_Unlock, ONCE);
}

TEST(SolidSyslogPlusTcpDatagramPool, DestroyOfUnknownHandleDoesNotLock)

{
    ConfigLockFake_Install();
    struct SolidSyslogDatagram stranger = {};

    SolidSyslogPlusTcpDatagram_Destroy(&stranger);

    CALLED_FAKE(ConfigLockFake_Lock, NEVER);
    CALLED_FAKE(ConfigLockFake_Unlock, NEVER);
}

TEST(SolidSyslogPlusTcpDatagramPool, DestroyOfUnknownHandleReportsWarning)

{
    ErrorHandlerFake_Install(nullptr);
    struct SolidSyslogDatagram stranger = {};

    SolidSyslogPlusTcpDatagram_Destroy(&stranger);

    CHECK_ERROR_REPORTED_ONCE(
        SOLIDSYSLOG_SEVERITY_WARNING,
        &SolidSyslogPlusTcpDatagramErrorSource,
        SOLIDSYSLOG_CAT_UNKNOWN_DESTROY,
        SOLIDSYSLOG_DATAGRAM_ERROR_UNKNOWN_DESTROY
    );
}

TEST(SolidSyslogPlusTcpDatagramPool, DestroyOfStaleHandleReportsWarning)

{
    pooled[0] = SolidSyslogPlusTcpDatagram_Create(nullptr);
    SolidSyslogPlusTcpDatagram_Destroy(pooled[0]);
    ErrorHandlerFake_Install(nullptr);

    SolidSyslogPlusTcpDatagram_Destroy(pooled[0]);
    pooled[0] = nullptr;

    CHECK_ERROR_REPORTED_ONCE(
        SOLIDSYSLOG_SEVERITY_WARNING,
        &SolidSyslogPlusTcpDatagramErrorSource,
        SOLIDSYSLOG_CAT_UNKNOWN_DESTROY,
        SOLIDSYSLOG_DATAGRAM_ERROR_UNKNOWN_DESTROY
    );
}

TEST(SolidSyslogPlusTcpDatagram, TheErrorSourceNamesItsDetailsThroughDatagramErrors)
{
    CHECK(SolidSyslogDatagramErrors_Name == SolidSyslogPlusTcpDatagramErrorSource.DetailName);
}
