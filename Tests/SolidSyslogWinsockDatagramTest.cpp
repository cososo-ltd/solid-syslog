#include "TestUtils.h"
#include "CppUTest/TestHarness.h"

using namespace CososoTesting;
#include "ConfigLockFake.h"
#include "ErrorHandlerFake.h"
#include "SolidSyslogWinsockAddress.h"
#include "SolidSyslogWinsockAddressPrivate.h"
#include "SolidSyslogDatagram.h"
#include "SolidSyslogDatagramDefinition.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogTunables.h"
#include "SolidSyslogUdpPayload.h"
#include "SolidSyslogWinsockDatagram.h"
#include "SolidSyslogWinsockDatagramErrors.h"
#include "SolidSyslogWinsockDatagramInternal.h"
#include "WinsockFake.h"
#include <cstdint>
#include <winsock2.h>
#include <ws2tcpip.h>

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

// clang-format off
static const char* const TEST_MESSAGE     = "hello";
static const size_t      TEST_MESSAGE_LEN = 5;
static const char* const TEST_ADDRESS     = "127.0.0.1";
static const int         TEST_PORT        = 514;

TEST_GROUP(SolidSyslogWinsockDatagram)
{
    struct SolidSyslogDatagram* datagram = nullptr;
    struct SolidSyslogAddress* addr = nullptr;

    void setup() override
    {
        WinsockFake_Reset();
        UT_PTR_SET(Winsock_socket, WinsockFake_socket);
        UT_PTR_SET(Winsock_sendto, WinsockFake_sendto);
        UT_PTR_SET(Winsock_closesocket, WinsockFake_closesocket);
        UT_PTR_SET(Winsock_connect, WinsockFake_connect);
        UT_PTR_SET(Winsock_setsockopt, WinsockFake_setsockopt);
        UT_PTR_SET(Winsock_getsockopt, WinsockFake_getsockopt);
        datagram                = SolidSyslogWinsockDatagram_Create();
        addr                    = SolidSyslogWinsockAddress_Create();
        struct sockaddr_in* sin = SolidSyslogWinsockAddress_AsSockaddrIn(addr);
        sin->sin_family         = AF_INET;
        sin->sin_port           = htons(TEST_PORT);
        inet_pton(AF_INET, TEST_ADDRESS, &sin->sin_addr);
    }

    void teardown() override
    {
        SolidSyslogWinsockAddress_Destroy(addr);
        SolidSyslogWinsockDatagram_Destroy(datagram);
    }
};

// clang-format on

TEST(SolidSyslogWinsockDatagram, CreateDestroyWorksWithoutCrashing)
{
}

TEST(SolidSyslogWinsockDatagram, OpenCallsSocketOnce)
{
    SolidSyslogDatagram_Open(datagram);
    CALLED_FAKE(WinsockFake_Socket, ONCE);
}

TEST(SolidSyslogWinsockDatagram, OpenCallsSocketWithAF_INET)
{
    SolidSyslogDatagram_Open(datagram);
    LONGS_EQUAL(AF_INET, WinsockFake_SocketDomain());
}

TEST(SolidSyslogWinsockDatagram, OpenCallsSocketWithSOCK_DGRAM)
{
    SolidSyslogDatagram_Open(datagram);
    LONGS_EQUAL(SOCK_DGRAM, WinsockFake_SocketType());
}

TEST(SolidSyslogWinsockDatagram, OpenReturnsFalseWhenSocketFails)
{
    WinsockFake_SetSocketFails(true);
    CHECK_FALSE(SolidSyslogDatagram_Open(datagram));
}

TEST(SolidSyslogWinsockDatagram, OpenReturnsTrueOnSuccess)
{
    CHECK_TRUE(SolidSyslogDatagram_Open(datagram));
}

TEST(SolidSyslogWinsockDatagram, SendToCallsSendtoOnce)
{
    SolidSyslogDatagram_Open(datagram);
    SolidSyslogDatagram_SendTo(datagram, TEST_MESSAGE, TEST_MESSAGE_LEN, addr);
    CALLED_FAKE(WinsockFake_Sendto, ONCE);
}

TEST(SolidSyslogWinsockDatagram, SendToPassesBuffer)
{
    SolidSyslogDatagram_Open(datagram);
    SolidSyslogDatagram_SendTo(datagram, TEST_MESSAGE, TEST_MESSAGE_LEN, addr);
    STRCMP_EQUAL(TEST_MESSAGE, WinsockFake_LastBufAsString());
}

TEST(SolidSyslogWinsockDatagram, SendToPassesLength)
{
    SolidSyslogDatagram_Open(datagram);
    SolidSyslogDatagram_SendTo(datagram, TEST_MESSAGE, TEST_MESSAGE_LEN, addr);
    LONGS_EQUAL(TEST_MESSAGE_LEN, WinsockFake_LastLen());
}

TEST(SolidSyslogWinsockDatagram, SendToPassesFlagsZero)
{
    SolidSyslogDatagram_Open(datagram);
    SolidSyslogDatagram_SendTo(datagram, TEST_MESSAGE, TEST_MESSAGE_LEN, addr);
    LONGS_EQUAL(0, WinsockFake_LastFlags());
}

TEST(SolidSyslogWinsockDatagram, SendToPassesSocketFd)
{
    SolidSyslogDatagram_Open(datagram);
    SolidSyslogDatagram_SendTo(datagram, TEST_MESSAGE, TEST_MESSAGE_LEN, addr);
    CHECK(WinsockFake_SocketFd() == WinsockFake_LastSendtoFd());
}

TEST(SolidSyslogWinsockDatagram, SendToPassesAddressPort)
{
    SolidSyslogDatagram_Open(datagram);
    SolidSyslogDatagram_SendTo(datagram, TEST_MESSAGE, TEST_MESSAGE_LEN, addr);
    LONGS_EQUAL(TEST_PORT, WinsockFake_LastPort());
}

TEST(SolidSyslogWinsockDatagram, SendToPassesAddressHost)
{
    SolidSyslogDatagram_Open(datagram);
    SolidSyslogDatagram_SendTo(datagram, TEST_MESSAGE, TEST_MESSAGE_LEN, addr);
    STRCMP_EQUAL(TEST_ADDRESS, WinsockFake_LastAddrAsString());
}

TEST(SolidSyslogWinsockDatagram, SendToPassesAddrlenOfSockaddrIn)
{
    SolidSyslogDatagram_Open(datagram);
    SolidSyslogDatagram_SendTo(datagram, TEST_MESSAGE, TEST_MESSAGE_LEN, addr);
    LONGS_EQUAL((int) sizeof(struct sockaddr_in), WinsockFake_LastAddrLen());
}

TEST(SolidSyslogWinsockDatagram, SendToReturnsSentOnSuccess)
{
    SolidSyslogDatagram_Open(datagram);
    LONGS_EQUAL(
        SOLIDSYSLOG_DATAGRAM_SEND_RESULT_SENT,
        SolidSyslogDatagram_SendTo(datagram, TEST_MESSAGE, TEST_MESSAGE_LEN, addr)
    );
}

TEST(SolidSyslogWinsockDatagram, SendToReturnsFailedOnSendtoFailure)
{
    SolidSyslogDatagram_Open(datagram);
    WinsockFake_SetSendtoFails(true);
    LONGS_EQUAL(
        SOLIDSYSLOG_DATAGRAM_SEND_RESULT_FAILED,
        SolidSyslogDatagram_SendTo(datagram, TEST_MESSAGE, TEST_MESSAGE_LEN, addr)
    );
}

TEST(SolidSyslogWinsockDatagram, CloseCallsCloseOnce)
{
    SolidSyslogDatagram_Open(datagram);
    SolidSyslogDatagram_Close(datagram);
    CALLED_FAKE(WinsockFake_Close, ONCE);
}

TEST(SolidSyslogWinsockDatagram, CloseCalledWithSocketFd)
{
    SolidSyslogDatagram_Open(datagram);
    SolidSyslogDatagram_Close(datagram);
    CHECK(WinsockFake_SocketFd() == WinsockFake_LastClosedFd());
}

TEST(SolidSyslogWinsockDatagram, MaxPayloadFallsBackToIpv6SafePayload)
{
    LONGS_EQUAL(SOLIDSYSLOG_UDP_IPV6_SAFE_PAYLOAD, SolidSyslogDatagram_MaxPayload(datagram));
}

TEST(SolidSyslogWinsockDatagram, OpenDoesNotConnect)
{
    SolidSyslogDatagram_Open(datagram);
    CALLED_FAKE(WinsockFake_Connect, NEVER);
}

TEST(SolidSyslogWinsockDatagram, SendToConnectsOnFirstCall)
{
    SolidSyslogDatagram_Open(datagram);
    SolidSyslogDatagram_SendTo(datagram, TEST_MESSAGE, TEST_MESSAGE_LEN, addr);
    CALLED_FAKE(WinsockFake_Connect, ONCE);
}

TEST(SolidSyslogWinsockDatagram, SendToConnectsOnceAcrossMultipleCalls)
{
    SolidSyslogDatagram_Open(datagram);
    SolidSyslogDatagram_SendTo(datagram, TEST_MESSAGE, TEST_MESSAGE_LEN, addr);
    SolidSyslogDatagram_SendTo(datagram, TEST_MESSAGE, TEST_MESSAGE_LEN, addr);
    CALLED_FAKE(WinsockFake_Connect, ONCE);
}

TEST(SolidSyslogWinsockDatagram, FirstSendEnablesPmtuDiscovery)
{
    SolidSyslogDatagram_Open(datagram);
    SolidSyslogDatagram_SendTo(datagram, TEST_MESSAGE, TEST_MESSAGE_LEN, addr);
    CHECK_TRUE(WinsockFake_HasSetSockOpt(IPPROTO_IP, IP_MTU_DISCOVER));
}

TEST(SolidSyslogWinsockDatagram, SendToReturnsOversizeOnWsaemsgsize)
{
    SolidSyslogDatagram_Open(datagram);
    WinsockFake_FailNextSendtoWithLastError(WSAEMSGSIZE);
    LONGS_EQUAL(
        SOLIDSYSLOG_DATAGRAM_SEND_RESULT_OVERSIZE,
        SolidSyslogDatagram_SendTo(datagram, TEST_MESSAGE, TEST_MESSAGE_LEN, addr)
    );
}

TEST(SolidSyslogWinsockDatagram, SendToReturnsFailedWhenConnectFails)
{
    SolidSyslogDatagram_Open(datagram);
    WinsockFake_SetConnectFails(true);
    LONGS_EQUAL(
        SOLIDSYSLOG_DATAGRAM_SEND_RESULT_FAILED,
        SolidSyslogDatagram_SendTo(datagram, TEST_MESSAGE, TEST_MESSAGE_LEN, addr)
    );
}

TEST(SolidSyslogWinsockDatagram, MaxPayloadAfterConnectQueriesIpMtu)
{
    SolidSyslogDatagram_Open(datagram);
    SolidSyslogDatagram_SendTo(datagram, TEST_MESSAGE, TEST_MESSAGE_LEN, addr);
    SolidSyslogDatagram_MaxPayload(datagram);
    CALLED_FAKE(WinsockFake_GetSockOpt, ONCE);
}

TEST(SolidSyslogWinsockDatagram, MaxPayloadConvertsIpMtuViaFromMtu)
{
    SolidSyslogDatagram_Open(datagram);
    SolidSyslogDatagram_SendTo(datagram, TEST_MESSAGE, TEST_MESSAGE_LEN, addr);
    WinsockFake_SetIpMtu(1500);
    LONGS_EQUAL(1472, SolidSyslogDatagram_MaxPayload(datagram));
}

TEST(SolidSyslogWinsockDatagram, MaxPayloadFallsBackWhenIpMtuLookupFails)
{
    SolidSyslogDatagram_Open(datagram);
    SolidSyslogDatagram_SendTo(datagram, TEST_MESSAGE, TEST_MESSAGE_LEN, addr);
    WinsockFake_SetIpMtuLookupFails(true);
    LONGS_EQUAL(SOLIDSYSLOG_UDP_IPV6_SAFE_PAYLOAD, SolidSyslogDatagram_MaxPayload(datagram));
}

// clang-format off
TEST_GROUP(SolidSyslogWinsockDatagramPool)
{
    struct SolidSyslogDatagram* pooled[SOLIDSYSLOG_WINSOCK_DATAGRAM_POOL_SIZE] = {};
    struct SolidSyslogDatagram* overflow                                       = nullptr;

    void teardown() override
    {
        for (auto* handle : pooled)
        {
            if (handle != nullptr)
            {
                SolidSyslogWinsockDatagram_Destroy(handle);
            }
        }
        if (overflow != nullptr)
        {
            SolidSyslogWinsockDatagram_Destroy(overflow);
        }
        ConfigLockFake_Uninstall();
    }

    void FillPool()
    {
        for (auto*& slot : pooled)
        {
            slot = SolidSyslogWinsockDatagram_Create();
        }
    }
};

// clang-format on

TEST(SolidSyslogWinsockDatagramPool, FillingPoolThenOverflowReturnsDistinctFallback)
{
    FillPool();

    overflow = SolidSyslogWinsockDatagram_Create();

    CHECK_IS_FALLBACK(overflow, pooled);
}

TEST(SolidSyslogWinsockDatagramPool, ExhaustedCreateReportsError)
{
    ErrorHandlerFake_Install(nullptr);
    FillPool();

    overflow = SolidSyslogWinsockDatagram_Create();

    CALLED_FAKE(ErrorHandlerFake_Handle, ONCE);
    LONGS_EQUAL(SOLIDSYSLOG_SEVERITY_ERROR, ErrorHandlerFake_LastSeverity());
    POINTERS_EQUAL(&WinsockDatagramErrorSource, ErrorHandlerFake_LastSource());
    UNSIGNED_LONGS_EQUAL(WINSOCKDATAGRAM_ERROR_POOL_EXHAUSTED, ErrorHandlerFake_LastCode());
}

TEST(SolidSyslogWinsockDatagramPool, FallbackSendToIsNoOp)
{
    FillPool();
    overflow = SolidSyslogWinsockDatagram_Create();
    struct SolidSyslogAddress* nullAddr = SolidSyslogWinsockAddress_Create();
    struct sockaddr_in* sin = SolidSyslogWinsockAddress_AsSockaddrIn(nullAddr);
    sin->sin_family = AF_INET;
    sin->sin_port = htons(514);

    LONGS_EQUAL(SOLIDSYSLOG_DATAGRAM_SEND_RESULT_SENT, SolidSyslogDatagram_SendTo(overflow, "x", 1, nullAddr));

    SolidSyslogWinsockAddress_Destroy(nullAddr);
}

TEST(SolidSyslogWinsockDatagramPool, CreateAcquiresAndReleasesConfigLockOnFirstFreeSlot)
{
    ConfigLockFake_Install();

    pooled[0] = SolidSyslogWinsockDatagram_Create();

    CALLED_FAKE(ConfigLockFake_Lock, ONCE);
    CALLED_FAKE(ConfigLockFake_Unlock, ONCE);
}

TEST(SolidSyslogWinsockDatagramPool, CreateLocksOncePerSlotProbedWhenPoolIsFull)
{
    FillPool();
    ConfigLockFake_Install();

    overflow = SolidSyslogWinsockDatagram_Create();

    LONGS_EQUAL(SOLIDSYSLOG_WINSOCK_DATAGRAM_POOL_SIZE, ConfigLockFake_LockCallCount());
    LONGS_EQUAL(SOLIDSYSLOG_WINSOCK_DATAGRAM_POOL_SIZE, ConfigLockFake_UnlockCallCount());
}

TEST(SolidSyslogWinsockDatagramPool, DestroyOfPooledHandleLocksOnce)
{
    pooled[0] = SolidSyslogWinsockDatagram_Create();
    ConfigLockFake_Install();

    SolidSyslogWinsockDatagram_Destroy(pooled[0]);
    pooled[0] = nullptr;

    CALLED_FAKE(ConfigLockFake_Lock, ONCE);
    CALLED_FAKE(ConfigLockFake_Unlock, ONCE);
}

TEST(SolidSyslogWinsockDatagramPool, DestroyOfUnknownHandleDoesNotLock)
{
    ConfigLockFake_Install();
    struct SolidSyslogDatagram stranger = {};

    SolidSyslogWinsockDatagram_Destroy(&stranger);

    CALLED_FAKE(ConfigLockFake_Lock, NEVER);
    CALLED_FAKE(ConfigLockFake_Unlock, NEVER);
}

TEST(SolidSyslogWinsockDatagramPool, DestroyOfUnknownHandleReportsWarning)
{
    ErrorHandlerFake_Install(nullptr);
    struct SolidSyslogDatagram stranger = {};

    SolidSyslogWinsockDatagram_Destroy(&stranger);

    CALLED_FAKE(ErrorHandlerFake_Handle, ONCE);
    LONGS_EQUAL(SOLIDSYSLOG_SEVERITY_WARNING, ErrorHandlerFake_LastSeverity());
    POINTERS_EQUAL(&WinsockDatagramErrorSource, ErrorHandlerFake_LastSource());
    UNSIGNED_LONGS_EQUAL(WINSOCKDATAGRAM_ERROR_UNKNOWN_DESTROY, ErrorHandlerFake_LastCode());
}

TEST(SolidSyslogWinsockDatagramPool, DestroyOfStaleHandleReportsWarning)
{
    pooled[0] = SolidSyslogWinsockDatagram_Create();
    SolidSyslogWinsockDatagram_Destroy(pooled[0]);
    ErrorHandlerFake_Install(nullptr);

    SolidSyslogWinsockDatagram_Destroy(pooled[0]);
    pooled[0] = nullptr;

    CALLED_FAKE(ErrorHandlerFake_Handle, ONCE);
    LONGS_EQUAL(SOLIDSYSLOG_SEVERITY_WARNING, ErrorHandlerFake_LastSeverity());
    POINTERS_EQUAL(&WinsockDatagramErrorSource, ErrorHandlerFake_LastSource());
    UNSIGNED_LONGS_EQUAL(WINSOCKDATAGRAM_ERROR_UNKNOWN_DESTROY, ErrorHandlerFake_LastCode());
}
