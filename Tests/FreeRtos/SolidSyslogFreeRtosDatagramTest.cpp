#include "TestUtils.h"
#include "CppUTest/TestHarness.h"

using namespace CososoTesting; // NOLINT(google-build-using-namespace) -- test-file scope only; brings NEVER/ONCE/TWICE/THRICE into scope for the CALLED_*
    // macros

#include "ConfigLockFake.h"
#include "ErrorHandlerFake.h"
#include "SolidSyslogAddress.h"
#include "SolidSyslogDatagram.h"
#include "SolidSyslogDatagramDefinition.h"
#include "SolidSyslogErrorMessages.h"
#include "SolidSyslogFreeRtosDatagram.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogTunables.h"
#include "SolidSyslogUdpPayload.h"

#include "FreeRtosArpFake.h"
#include "FreeRtosSocketsFake.h"
#include "FreeRtosTaskFake.h"

#include "FreeRTOS.h"
#include "FreeRTOS_IP.h"
#include "FreeRTOS_Sockets.h"

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

static const uint16_t TEST_PORT = 514;

TEST_GROUP(SolidSyslogFreeRtosDatagram)
{
    struct SolidSyslogDatagram* datagram = nullptr;
    SolidSyslogAddressStorage addrStorage{};
    struct SolidSyslogAddress* addr = nullptr;

    void setup() override
    {
        FreeRtosSocketsFake_Reset();
        FreeRtosArpFake_Reset();
        FreeRtosTaskFake_Reset();
        datagram = SolidSyslogFreeRtosDatagram_Create();

        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast) -- char-type aliasing into platform layout, storage is intptr_t-aligned
        auto* sin = reinterpret_cast<struct freertos_sockaddr*>(&addrStorage);
        sin->sin_family = FREERTOS_AF_INET;
        sin->sin_port = FreeRTOS_htons(TEST_PORT);
        sin->sin_address.ulIP_IPv4 = FreeRTOS_inet_addr_quick(127, 0, 0, 1);
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast) -- platform-layout cast, see above
        addr = reinterpret_cast<struct SolidSyslogAddress*>(&addrStorage);
    }

    void teardown() override
    {
        SolidSyslogFreeRtosDatagram_Destroy(datagram);
    }

    void openAndSendOnce() const
    {
        SolidSyslogDatagram_Open(datagram);
        SolidSyslogDatagram_SendTo(datagram, "x", 1, addr);
    }
};

TEST(SolidSyslogFreeRtosDatagram, CreateReturnsNonNullDatagram)
{
    CHECK(datagram != nullptr);
}

TEST(SolidSyslogFreeRtosDatagram, OpenCreatesUdpSocket)
{
    SolidSyslogDatagram_Open(datagram);
    CALLED_FAKE(FreeRtosSocketsFake_Socket, ONCE);
    LONGS_EQUAL(FREERTOS_AF_INET, FreeRtosSocketsFake_LastSocketDomain());
    LONGS_EQUAL(FREERTOS_SOCK_DGRAM, FreeRtosSocketsFake_LastSocketType());
    LONGS_EQUAL(FREERTOS_IPPROTO_UDP, FreeRtosSocketsFake_LastSocketProtocol());
}

TEST(SolidSyslogFreeRtosDatagram, OpenReturnsTrueOnSuccess)
{
    CHECK_TRUE(SolidSyslogDatagram_Open(datagram));
}

TEST(SolidSyslogFreeRtosDatagram, OpenReturnsFalseWhenSocketFails)
{
    FreeRtosSocketsFake_SetSocketFails(true);
    CHECK_FALSE(SolidSyslogDatagram_Open(datagram));
}

TEST(SolidSyslogFreeRtosDatagram, OpenIsIdempotent)
{
    SolidSyslogDatagram_Open(datagram);
    CHECK_TRUE(SolidSyslogDatagram_Open(datagram));
    CALLED_FAKE(FreeRtosSocketsFake_Socket, ONCE);
}

TEST(SolidSyslogFreeRtosDatagram, MaxPayloadReturnsIpv6SafeDefault)
{
    LONGS_EQUAL(SOLIDSYSLOG_UDP_IPV6_SAFE_PAYLOAD, SolidSyslogDatagram_MaxPayload(datagram));
}

TEST(SolidSyslogFreeRtosDatagram, SendToFailsBeforeOpen)
{
    enum SolidSyslogDatagramSendResult result = SolidSyslogDatagram_SendTo(datagram, "x", 1, addr);
    LONGS_EQUAL(SOLIDSYSLOG_DATAGRAM_SEND_RESULT_FAILED, result);
    CALLED_FAKE(FreeRtosSocketsFake_Sendto, NEVER);
}

TEST(SolidSyslogFreeRtosDatagram, SendToFailsWhenSendtoErrors)
{
    SolidSyslogDatagram_Open(datagram);
    FreeRtosSocketsFake_SetSendtoFails(true);
    LONGS_EQUAL(SOLIDSYSLOG_DATAGRAM_SEND_RESULT_FAILED, SolidSyslogDatagram_SendTo(datagram, "x", 1, addr));
}

TEST(SolidSyslogFreeRtosDatagram, CloseClosesSocket)
{
    SolidSyslogDatagram_Open(datagram);
    SolidSyslogDatagram_Close(datagram);
    CALLED_FAKE(FreeRtosSocketsFake_Closesocket, ONCE);
    POINTERS_EQUAL(FreeRtosSocketsFake_LastSocketReturned(), FreeRtosSocketsFake_LastClosesocketSocket());
}

TEST(SolidSyslogFreeRtosDatagram, SendToFailsAfterClose)
{
    SolidSyslogDatagram_Open(datagram);
    SolidSyslogDatagram_Close(datagram);
    enum SolidSyslogDatagramSendResult result = SolidSyslogDatagram_SendTo(datagram, "x", 1, addr);
    LONGS_EQUAL(SOLIDSYSLOG_DATAGRAM_SEND_RESULT_FAILED, result);
    CALLED_FAKE(FreeRtosSocketsFake_Sendto, NEVER);
}

TEST(SolidSyslogFreeRtosDatagram, DestroyClosesOpenSocket)
{
    SolidSyslogDatagram_Open(datagram);
    SolidSyslogFreeRtosDatagram_Destroy(datagram);
    datagram = nullptr;
    CALLED_FAKE(FreeRtosSocketsFake_Closesocket, ONCE);
}

TEST(SolidSyslogFreeRtosDatagram, CloseIsIdempotent)
{
    SolidSyslogDatagram_Open(datagram);
    SolidSyslogDatagram_Close(datagram);
    SolidSyslogDatagram_Close(datagram);
    CALLED_FAKE(FreeRtosSocketsFake_Closesocket, ONCE);
}

TEST(SolidSyslogFreeRtosDatagram, CloseWithoutOpenIsNoOp)
{
    SolidSyslogDatagram_Close(datagram);
    CALLED_FAKE(FreeRtosSocketsFake_Closesocket, NEVER);
}

TEST(SolidSyslogFreeRtosDatagram, DestroyAfterCloseDoesNotCloseAgain)
{
    SolidSyslogDatagram_Open(datagram);
    SolidSyslogDatagram_Close(datagram);
    SolidSyslogFreeRtosDatagram_Destroy(datagram);
    datagram = nullptr;
    CALLED_FAKE(FreeRtosSocketsFake_Closesocket, ONCE);
}

TEST(SolidSyslogFreeRtosDatagram, SendToSendsBufferToDestinationAfterOpen)
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
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast) -- platform-layout cast, see setup
    POINTERS_EQUAL(
        reinterpret_cast<const struct freertos_sockaddr*>(addr),
        FreeRtosSocketsFake_LastSendtoDestination()
    );
    LONGS_EQUAL(sizeof(struct freertos_sockaddr), FreeRtosSocketsFake_LastSendtoDestinationLength());
}

TEST(SolidSyslogFreeRtosDatagram, SendToChecksIfIpIsInArpCache)
{
    openAndSendOnce();

    CALLED_FAKE(FreeRtosArpFake_IsIpInArpCache, ONCE);
    LONGS_EQUAL(FreeRTOS_inet_addr_quick(127, 0, 0, 1), FreeRtosArpFake_LastIsIpInArpCacheArg());
}

TEST(SolidSyslogFreeRtosDatagram, SendToFiresArpProbeOnCacheMiss)
{
    openAndSendOnce();

    CALLED_FAKE(FreeRtosArpFake_OutputArpRequest, ONCE);
    LONGS_EQUAL(FreeRTOS_inet_addr_quick(127, 0, 0, 1), FreeRtosArpFake_LastOutputArpRequestArg());
}

TEST(SolidSyslogFreeRtosDatagram, SendToYieldsAfterArpProbeOnCacheMiss)
{
    openAndSendOnce();

    CALLED_FAKE(FreeRtosTaskFake_VTaskDelay, ONCE);
}

TEST(SolidSyslogFreeRtosDatagram, SendToSkipsArpProbeAndYieldOnCacheHit)
{
    SolidSyslogDatagram_Open(datagram);
    FreeRtosArpFake_SetCacheHit(true);

    SolidSyslogDatagram_SendTo(datagram, "x", 1, addr);

    CALLED_FAKE(FreeRtosArpFake_OutputArpRequest, NEVER);
    CALLED_FAKE(FreeRtosTaskFake_VTaskDelay, NEVER);
    CALLED_FAKE(FreeRtosSocketsFake_Sendto, ONCE);
}

TEST(SolidSyslogFreeRtosDatagram, SendToReChecksArpCacheOnEachCall)
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

TEST(SolidSyslogFreeRtosDatagram, SendToForwardsLengthVerbatim)
{
    static const char TEST_BUFFER[1024] = {};
    SolidSyslogDatagram_Open(datagram);

    SolidSyslogDatagram_SendTo(datagram, TEST_BUFFER, 1, addr);
    LONGS_EQUAL(1, FreeRtosSocketsFake_LastSendtoLength());

    SolidSyslogDatagram_SendTo(datagram, TEST_BUFFER, 1232, addr);
    LONGS_EQUAL(1232, FreeRtosSocketsFake_LastSendtoLength());
}

// clang-format off
TEST_GROUP(SolidSyslogFreeRtosDatagramPool)
{
    // cppcheck-suppress constVariable -- assigned in test bodies; cppcheck does not model CppUTest lifecycle
    struct SolidSyslogDatagram* pooled[SOLIDSYSLOG_FREE_RTOS_DATAGRAM_POOL_SIZE] = {};
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
                SolidSyslogFreeRtosDatagram_Destroy(handle);
            }
        }
        // cppcheck-suppress knownConditionTrueFalse -- assigned in test bodies; cppcheck does not model CppUTest lifecycle
        if (overflow != nullptr)
        {
            SolidSyslogFreeRtosDatagram_Destroy(overflow);
        }
        ConfigLockFake_Uninstall();
        ErrorHandlerFake_Uninstall();
    }

    void FillPool()
    {
        for (auto*& slot : pooled)
        {
            slot = SolidSyslogFreeRtosDatagram_Create();
        }
    }
};

// clang-format on

TEST(SolidSyslogFreeRtosDatagramPool, FillingPoolThenOverflowReturnsDistinctFallback)
{
    FillPool();

    overflow = SolidSyslogFreeRtosDatagram_Create();

    CHECK_IS_FALLBACK(overflow, pooled);
}

TEST(SolidSyslogFreeRtosDatagramPool, ExhaustedCreateReportsError)
{
    ErrorHandlerFake_Install(nullptr);
    FillPool();

    overflow = SolidSyslogFreeRtosDatagram_Create();

    CALLED_FAKE(ErrorHandlerFake_Handle, ONCE);
    LONGS_EQUAL(SOLIDSYSLOG_SEVERITY_ERROR, ErrorHandlerFake_LastSeverity());
    STRCMP_EQUAL(SOLIDSYSLOG_ERROR_MSG_FREERTOSDATAGRAM_POOL_EXHAUSTED, ErrorHandlerFake_LastMessage());
}

TEST(SolidSyslogFreeRtosDatagramPool, FallbackVtableMethodsAreNoOps)
{
    FillPool();
    overflow = SolidSyslogFreeRtosDatagram_Create();

    /* NullDatagram's Open returns true so caller success paths are not
     * tripped; no underlying FreeRTOS_socket is created. */
    CHECK_TRUE(SolidSyslogDatagram_Open(overflow));
    CALLED_FAKE(FreeRtosSocketsFake_Socket, NEVER);
    SolidSyslogDatagram_Close(overflow);
}

TEST(SolidSyslogFreeRtosDatagramPool, CreateAcquiresAndReleasesConfigLockOnFirstFreeSlot)
{
    ConfigLockFake_Install();

    pooled[0] = SolidSyslogFreeRtosDatagram_Create();

    CALLED_FAKE(ConfigLockFake_Lock, ONCE);
    CALLED_FAKE(ConfigLockFake_Unlock, ONCE);
}

TEST(SolidSyslogFreeRtosDatagramPool, CreateLocksOncePerSlotProbedWhenPoolIsFull)
{
    FillPool();
    ConfigLockFake_Install();

    overflow = SolidSyslogFreeRtosDatagram_Create();

    LONGS_EQUAL(SOLIDSYSLOG_FREE_RTOS_DATAGRAM_POOL_SIZE, ConfigLockFake_LockCallCount());
    LONGS_EQUAL(SOLIDSYSLOG_FREE_RTOS_DATAGRAM_POOL_SIZE, ConfigLockFake_UnlockCallCount());
}

TEST(SolidSyslogFreeRtosDatagramPool, DestroyOfPooledHandleLocksOnce)
{
    pooled[0] = SolidSyslogFreeRtosDatagram_Create();
    ConfigLockFake_Install();

    SolidSyslogFreeRtosDatagram_Destroy(pooled[0]);
    pooled[0] = nullptr;

    CALLED_FAKE(ConfigLockFake_Lock, ONCE);
    CALLED_FAKE(ConfigLockFake_Unlock, ONCE);
}

TEST(SolidSyslogFreeRtosDatagramPool, DestroyOfUnknownHandleDoesNotLock)
{
    ConfigLockFake_Install();
    struct SolidSyslogDatagram stranger = {};

    SolidSyslogFreeRtosDatagram_Destroy(&stranger);

    CALLED_FAKE(ConfigLockFake_Lock, NEVER);
    CALLED_FAKE(ConfigLockFake_Unlock, NEVER);
}

TEST(SolidSyslogFreeRtosDatagramPool, DestroyOfUnknownHandleReportsWarning)
{
    ErrorHandlerFake_Install(nullptr);
    struct SolidSyslogDatagram stranger = {};

    SolidSyslogFreeRtosDatagram_Destroy(&stranger);

    CALLED_FAKE(ErrorHandlerFake_Handle, ONCE);
    LONGS_EQUAL(SOLIDSYSLOG_SEVERITY_WARNING, ErrorHandlerFake_LastSeverity());
    STRCMP_EQUAL(SOLIDSYSLOG_ERROR_MSG_FREERTOSDATAGRAM_UNKNOWN_DESTROY, ErrorHandlerFake_LastMessage());
}

TEST(SolidSyslogFreeRtosDatagramPool, DestroyOfStaleHandleReportsWarning)
{
    pooled[0] = SolidSyslogFreeRtosDatagram_Create();
    SolidSyslogFreeRtosDatagram_Destroy(pooled[0]);
    ErrorHandlerFake_Install(nullptr);

    SolidSyslogFreeRtosDatagram_Destroy(pooled[0]);
    pooled[0] = nullptr;

    CALLED_FAKE(ErrorHandlerFake_Handle, ONCE);
    LONGS_EQUAL(SOLIDSYSLOG_SEVERITY_WARNING, ErrorHandlerFake_LastSeverity());
    STRCMP_EQUAL(SOLIDSYSLOG_ERROR_MSG_FREERTOSDATAGRAM_UNKNOWN_DESTROY, ErrorHandlerFake_LastMessage());
}
