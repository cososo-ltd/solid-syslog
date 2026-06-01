#include "TestUtils.h"
#include "CppUTest/TestHarness.h"

using namespace CososoTesting;
#include "ConfigLockFake.h"
#include "ErrorHandlerFake.h"
#include "SolidSyslogDatagram.h"
#include "SolidSyslogDatagramDefinition.h"
#include "SolidSyslogPosixAddress.h"
#include "SolidSyslogPosixAddressPrivate.h"
#include "SolidSyslogPosixDatagram.h"
#include "SolidSyslogPosixDatagramErrors.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogTunables.h"
#include "SolidSyslogUdpPayload.h"
#include "SocketFake.h"
#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <netinet/in.h>
#include <sys/socket.h>

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

TEST_GROUP(SolidSyslogPosixDatagram)
{
    struct SolidSyslogDatagram* datagram = nullptr;
    struct SolidSyslogAddress* addr = nullptr;

    void setup() override
    {
        SocketFake_Reset();
        datagram        = SolidSyslogPosixDatagram_Create();
        addr            = SolidSyslogPosixAddress_Create();
        struct sockaddr_in* sin = SolidSyslogPosixAddress_AsSockaddrIn(addr);
        sin->sin_family = AF_INET;
        sin->sin_port   = htons(TEST_PORT);
        inet_pton(AF_INET, TEST_ADDRESS, &sin->sin_addr);
    }

    void teardown() override
    {
        SolidSyslogPosixAddress_Destroy(addr);
        SolidSyslogPosixDatagram_Destroy(datagram);
    }
};

// clang-format on

TEST(SolidSyslogPosixDatagram, CreateDestroyWorksWithoutCrashing)
{
}

TEST(SolidSyslogPosixDatagram, OpenCallsSocketOnce)
{
    SolidSyslogDatagram_Open(datagram);
    CALLED_FAKE(SocketFake_Socket, ONCE);
}

TEST(SolidSyslogPosixDatagram, OpenCallsSocketWithAF_INET)
{
    SolidSyslogDatagram_Open(datagram);
    LONGS_EQUAL(AF_INET, SocketFake_SocketDomain());
}

TEST(SolidSyslogPosixDatagram, OpenCallsSocketWithSOCK_DGRAM)
{
    SolidSyslogDatagram_Open(datagram);
    LONGS_EQUAL(SOCK_DGRAM, SocketFake_SocketType());
}

TEST(SolidSyslogPosixDatagram, OpenReturnsFalseWhenSocketFails)
{
    SocketFake_SetSocketFails(true);
    CHECK_FALSE(SolidSyslogDatagram_Open(datagram));
}

TEST(SolidSyslogPosixDatagram, OpenReturnsTrueOnSuccess)
{
    CHECK_TRUE(SolidSyslogDatagram_Open(datagram));
}

TEST(SolidSyslogPosixDatagram, SendToCallsSendtoOnce)
{
    SolidSyslogDatagram_Open(datagram);
    SolidSyslogDatagram_SendTo(datagram, TEST_MESSAGE, TEST_MESSAGE_LEN, addr);
    CALLED_FAKE(SocketFake_Sendto, ONCE);
}

TEST(SolidSyslogPosixDatagram, SendToPassesBuffer)
{
    SolidSyslogDatagram_Open(datagram);
    SolidSyslogDatagram_SendTo(datagram, TEST_MESSAGE, TEST_MESSAGE_LEN, addr);
    STRCMP_EQUAL(TEST_MESSAGE, SocketFake_LastBufAsString());
}

TEST(SolidSyslogPosixDatagram, SendToPassesLength)
{
    SolidSyslogDatagram_Open(datagram);
    SolidSyslogDatagram_SendTo(datagram, TEST_MESSAGE, TEST_MESSAGE_LEN, addr);
    LONGS_EQUAL(TEST_MESSAGE_LEN, SocketFake_LastLen());
}

TEST(SolidSyslogPosixDatagram, SendToPassesFlagsZero)
{
    SolidSyslogDatagram_Open(datagram);
    SolidSyslogDatagram_SendTo(datagram, TEST_MESSAGE, TEST_MESSAGE_LEN, addr);
    LONGS_EQUAL(0, SocketFake_LastFlags());
}

TEST(SolidSyslogPosixDatagram, SendToPassesSocketFd)
{
    SolidSyslogDatagram_Open(datagram);
    SolidSyslogDatagram_SendTo(datagram, TEST_MESSAGE, TEST_MESSAGE_LEN, addr);
    LONGS_EQUAL(SocketFake_SocketFd(), SocketFake_LastSendtoFd());
}

TEST(SolidSyslogPosixDatagram, SendToPassesAddressPort)
{
    SolidSyslogDatagram_Open(datagram);
    SolidSyslogDatagram_SendTo(datagram, TEST_MESSAGE, TEST_MESSAGE_LEN, addr);
    LONGS_EQUAL(TEST_PORT, SocketFake_LastPort());
}

TEST(SolidSyslogPosixDatagram, SendToPassesAddressHost)
{
    SolidSyslogDatagram_Open(datagram);
    SolidSyslogDatagram_SendTo(datagram, TEST_MESSAGE, TEST_MESSAGE_LEN, addr);
    STRCMP_EQUAL(TEST_ADDRESS, SocketFake_LastAddrAsString());
}

TEST(SolidSyslogPosixDatagram, SendToPassesAddrlenOfSockaddrIn)
{
    SolidSyslogDatagram_Open(datagram);
    SolidSyslogDatagram_SendTo(datagram, TEST_MESSAGE, TEST_MESSAGE_LEN, addr);
    LONGS_EQUAL(sizeof(struct sockaddr_in), SocketFake_LastAddrLen());
}

TEST(SolidSyslogPosixDatagram, SendToReturnsSentOnSuccess)
{
    SolidSyslogDatagram_Open(datagram);
    LONGS_EQUAL(
        SOLIDSYSLOG_DATAGRAM_SEND_RESULT_SENT,
        SolidSyslogDatagram_SendTo(datagram, TEST_MESSAGE, TEST_MESSAGE_LEN, addr)
    );
}

TEST(SolidSyslogPosixDatagram, SendToReturnsFailedOnSendtoFailure)
{
    SolidSyslogDatagram_Open(datagram);
    SocketFake_SetSendtoFails(true);
    LONGS_EQUAL(
        SOLIDSYSLOG_DATAGRAM_SEND_RESULT_FAILED,
        SolidSyslogDatagram_SendTo(datagram, TEST_MESSAGE, TEST_MESSAGE_LEN, addr)
    );
}

TEST(SolidSyslogPosixDatagram, CloseCallsCloseOnce)
{
    SolidSyslogDatagram_Open(datagram);
    SolidSyslogDatagram_Close(datagram);
    CALLED_FAKE(SocketFake_Close, ONCE);
}

TEST(SolidSyslogPosixDatagram, CloseCalledWithSocketFd)
{
    SolidSyslogDatagram_Open(datagram);
    SolidSyslogDatagram_Close(datagram);
    LONGS_EQUAL(SocketFake_SocketFd(), SocketFake_LastClosedFd());
}

TEST(SolidSyslogPosixDatagram, MaxPayloadFallsBackToIpv6SafePayload)
{
    LONGS_EQUAL(SOLIDSYSLOG_UDP_IPV6_SAFE_PAYLOAD, SolidSyslogDatagram_MaxPayload(datagram));
}

TEST(SolidSyslogPosixDatagram, OpenDoesNotConnect)
{
    SolidSyslogDatagram_Open(datagram);
    CALLED_FAKE(SocketFake_Connect, NEVER);
}

TEST(SolidSyslogPosixDatagram, SendToConnectsOnFirstCall)
{
    SolidSyslogDatagram_Open(datagram);
    SolidSyslogDatagram_SendTo(datagram, TEST_MESSAGE, TEST_MESSAGE_LEN, addr);
    CALLED_FAKE(SocketFake_Connect, ONCE);
}

TEST(SolidSyslogPosixDatagram, SendToConnectsOnceAcrossMultipleCalls)
{
    SolidSyslogDatagram_Open(datagram);
    SolidSyslogDatagram_SendTo(datagram, TEST_MESSAGE, TEST_MESSAGE_LEN, addr);
    SolidSyslogDatagram_SendTo(datagram, TEST_MESSAGE, TEST_MESSAGE_LEN, addr);
    CALLED_FAKE(SocketFake_Connect, ONCE);
}

TEST(SolidSyslogPosixDatagram, FirstSendEnablesPmtuDiscovery)
{
    SolidSyslogDatagram_Open(datagram);
    SolidSyslogDatagram_SendTo(datagram, TEST_MESSAGE, TEST_MESSAGE_LEN, addr);
    CHECK_TRUE(SocketFake_HasSetSockOpt(IPPROTO_IP, IP_MTU_DISCOVER));
}

TEST(SolidSyslogPosixDatagram, SendToReturnsOversizeOnEmsgsize)
{
    SolidSyslogDatagram_Open(datagram);
    SocketFake_FailNextSendtoWithErrno(EMSGSIZE);
    LONGS_EQUAL(
        SOLIDSYSLOG_DATAGRAM_SEND_RESULT_OVERSIZE,
        SolidSyslogDatagram_SendTo(datagram, TEST_MESSAGE, TEST_MESSAGE_LEN, addr)
    );
}

TEST(SolidSyslogPosixDatagram, MaxPayloadAfterConnectQueriesIpMtu)
{
    SolidSyslogDatagram_Open(datagram);
    SolidSyslogDatagram_SendTo(datagram, TEST_MESSAGE, TEST_MESSAGE_LEN, addr);
    SolidSyslogDatagram_MaxPayload(datagram);
    CALLED_FAKE(SocketFake_GetSockOpt, ONCE);
}

TEST(SolidSyslogPosixDatagram, MaxPayloadConvertsIpMtuViaFromMtu)
{
    SolidSyslogDatagram_Open(datagram);
    SolidSyslogDatagram_SendTo(datagram, TEST_MESSAGE, TEST_MESSAGE_LEN, addr);
    SocketFake_SetIpMtu(1500);
    LONGS_EQUAL(1472, SolidSyslogDatagram_MaxPayload(datagram));
}

TEST(SolidSyslogPosixDatagram, MaxPayloadFallsBackWhenIpMtuLookupFails)
{
    SolidSyslogDatagram_Open(datagram);
    SolidSyslogDatagram_SendTo(datagram, TEST_MESSAGE, TEST_MESSAGE_LEN, addr);
    SocketFake_SetIpMtuLookupFails(true);
    LONGS_EQUAL(SOLIDSYSLOG_UDP_IPV6_SAFE_PAYLOAD, SolidSyslogDatagram_MaxPayload(datagram));
}

TEST(SolidSyslogPosixDatagram, SendToReturnsFailedWhenConnectFails)
{
    SolidSyslogDatagram_Open(datagram);
    SocketFake_SetConnectFails(true);
    LONGS_EQUAL(
        SOLIDSYSLOG_DATAGRAM_SEND_RESULT_FAILED,
        SolidSyslogDatagram_SendTo(datagram, TEST_MESSAGE, TEST_MESSAGE_LEN, addr)
    );
}

// clang-format off
TEST_GROUP(SolidSyslogPosixDatagramPool)
{
    struct SolidSyslogDatagram* pooled[SOLIDSYSLOG_DATAGRAM_POOL_SIZE] = {};
    struct SolidSyslogDatagram* overflow                                     = nullptr;

    void teardown() override
    {
        for (auto* handle : pooled)
        {
            if (handle != nullptr)
            {
                SolidSyslogPosixDatagram_Destroy(handle);
            }
        }
        if (overflow != nullptr)
        {
            SolidSyslogPosixDatagram_Destroy(overflow);
        }
        ConfigLockFake_Uninstall();
    }

    void FillPool()
    {
        for (auto*& slot : pooled)
        {
            slot = SolidSyslogPosixDatagram_Create();
        }
    }
};

// clang-format on

TEST(SolidSyslogPosixDatagramPool, FillingPoolThenOverflowReturnsDistinctFallback)
{
    FillPool();

    overflow = SolidSyslogPosixDatagram_Create();

    CHECK_IS_FALLBACK(overflow, pooled);
}

TEST(SolidSyslogPosixDatagramPool, ExhaustedCreateReportsError)
{
    ErrorHandlerFake_Install(nullptr);
    FillPool();

    overflow = SolidSyslogPosixDatagram_Create();

    CALLED_FAKE(ErrorHandlerFake_Handle, ONCE);
    LONGS_EQUAL(SOLIDSYSLOG_SEVERITY_ERROR, ErrorHandlerFake_LastSeverity());
    POINTERS_EQUAL(&PosixDatagramErrorSource, ErrorHandlerFake_LastSource());
    UNSIGNED_LONGS_EQUAL(POSIXDATAGRAM_ERROR_POOL_EXHAUSTED, ErrorHandlerFake_LastCode());
}

TEST(SolidSyslogPosixDatagramPool, FallbackSendToReturnsSent)
{
    FillPool();
    overflow = SolidSyslogPosixDatagram_Create();

    LONGS_EQUAL(SOLIDSYSLOG_DATAGRAM_SEND_RESULT_SENT, SolidSyslogDatagram_SendTo(overflow, "x", 1, nullptr));
}

TEST(SolidSyslogPosixDatagramPool, CreateAcquiresAndReleasesConfigLockOnFirstFreeSlot)
{
    ConfigLockFake_Install();

    pooled[0] = SolidSyslogPosixDatagram_Create();

    CALLED_FAKE(ConfigLockFake_Lock, ONCE);
    CALLED_FAKE(ConfigLockFake_Unlock, ONCE);
}

TEST(SolidSyslogPosixDatagramPool, CreateLocksOncePerSlotProbedWhenPoolIsFull)
{
    FillPool();
    ConfigLockFake_Install();

    overflow = SolidSyslogPosixDatagram_Create();

    LONGS_EQUAL(SOLIDSYSLOG_DATAGRAM_POOL_SIZE, ConfigLockFake_LockCallCount());
    LONGS_EQUAL(SOLIDSYSLOG_DATAGRAM_POOL_SIZE, ConfigLockFake_UnlockCallCount());
}

TEST(SolidSyslogPosixDatagramPool, DestroyOfPooledHandleLocksOnce)
{
    pooled[0] = SolidSyslogPosixDatagram_Create();
    ConfigLockFake_Install();

    SolidSyslogPosixDatagram_Destroy(pooled[0]);
    pooled[0] = nullptr;

    CALLED_FAKE(ConfigLockFake_Lock, ONCE);
    CALLED_FAKE(ConfigLockFake_Unlock, ONCE);
}

TEST(SolidSyslogPosixDatagramPool, DestroyOfUnknownHandleDoesNotLock)
{
    ConfigLockFake_Install();
    struct SolidSyslogDatagram stranger = {};

    SolidSyslogPosixDatagram_Destroy(&stranger);

    CALLED_FAKE(ConfigLockFake_Lock, NEVER);
    CALLED_FAKE(ConfigLockFake_Unlock, NEVER);
}

TEST(SolidSyslogPosixDatagramPool, DestroyOfUnknownHandleReportsWarning)
{
    ErrorHandlerFake_Install(nullptr);
    struct SolidSyslogDatagram stranger = {};

    SolidSyslogPosixDatagram_Destroy(&stranger);

    CALLED_FAKE(ErrorHandlerFake_Handle, ONCE);
    LONGS_EQUAL(SOLIDSYSLOG_SEVERITY_WARNING, ErrorHandlerFake_LastSeverity());
    POINTERS_EQUAL(&PosixDatagramErrorSource, ErrorHandlerFake_LastSource());
    UNSIGNED_LONGS_EQUAL(POSIXDATAGRAM_ERROR_UNKNOWN_DESTROY, ErrorHandlerFake_LastCode());
}

TEST(SolidSyslogPosixDatagramPool, DestroyOfStaleHandleReportsWarning)
{
    pooled[0] = SolidSyslogPosixDatagram_Create();
    SolidSyslogPosixDatagram_Destroy(pooled[0]);
    ErrorHandlerFake_Install(nullptr);

    SolidSyslogPosixDatagram_Destroy(pooled[0]);
    pooled[0] = nullptr;

    CALLED_FAKE(ErrorHandlerFake_Handle, ONCE);
    LONGS_EQUAL(SOLIDSYSLOG_SEVERITY_WARNING, ErrorHandlerFake_LastSeverity());
    POINTERS_EQUAL(&PosixDatagramErrorSource, ErrorHandlerFake_LastSource());
    UNSIGNED_LONGS_EQUAL(POSIXDATAGRAM_ERROR_UNKNOWN_DESTROY, ErrorHandlerFake_LastCode());
}
