#include "TestUtils.h"
#include "CppUTest/TestHarness.h"

using namespace CososoTesting;

#include "lwip/sockets.h"

#include <cerrno>
#include <cstdint>

#include "ConfigLockFake.h"
#include "ErrorHandlerFake.h"
#include "LwipSocketsFake.h"
#include "SolidSyslogDatagram.h"
#include "SolidSyslogLwipSocketAddress.h"
#include "SolidSyslogLwipSocketAddressPrivate.h"
#include "SolidSyslogErrorCategory.h"
#include "SolidSyslogLwipSocketDatagram.h"
#include "SolidSyslogLwipSocketDatagramErrors.h"
#include "SolidSyslogNullDatagram.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogTunables.h"
#include "SolidSyslogUdpPayload.h"

// clang-format off
// 6514 rather than 514: 514 is 0x0202, so host and network order are the same
// bytes and an assertion on the port would hold whichever the adapter wrote.
static const char     TEST_PAYLOAD[]    = "<14>1 message";
static const size_t   TEST_PAYLOAD_SIZE = sizeof(TEST_PAYLOAD) - 1U;
static const uint16_t TEST_PORT         = 6514U;
static const uint32_t TEST_IPV4         = 0xC000020AU;
static const int      TEST_DESCRIPTOR   = 7;
// clang-format on

// clang-format off
TEST_GROUP(SolidSyslogLwipSocketDatagram)
{
    struct SolidSyslogDatagram* datagram = nullptr;
    struct SolidSyslogAddress*  address  = nullptr;

    void setup() override
    {
        LwipSocketsFake_Reset();
        datagram = SolidSyslogLwipSocketDatagram_Create();
        address  = SolidSyslogLwipSocketAddress_Create();
        struct sockaddr_in* sin = SolidSyslogLwipSocketAddress_AsSockaddrIn(address);
        sin->sin_family         = AF_INET;
        sin->sin_port           = lwip_htons(TEST_PORT);
        sin->sin_addr.s_addr    = lwip_htonl(TEST_IPV4);
        // Installed after the pool draws above, so an event count starts clean.
        ErrorHandlerFake_Install(nullptr);
    }

    void teardown() override
    {
        SolidSyslogLwipSocketAddress_Destroy(address);
        SolidSyslogLwipSocketDatagram_Destroy(datagram);
    }

    enum SolidSyslogDatagramSendResult SendTo() const
    {
        return SolidSyslogDatagram_SendTo(datagram, TEST_PAYLOAD, TEST_PAYLOAD_SIZE, address);
    }
};

// clang-format on

TEST(SolidSyslogLwipSocketDatagram, OpenCreatesAnIpv4UdpSocket)
{
    CHECK_TRUE(SolidSyslogDatagram_Open(datagram));

    LONGS_EQUAL(1U, LwipSocketsFake_SocketCallCount());
    LONGS_EQUAL(AF_INET, LwipSocketsFake_LastSocketDomain());
    LONGS_EQUAL(SOCK_DGRAM, LwipSocketsFake_LastSocketType());
    LONGS_EQUAL(0, LwipSocketsFake_LastSocketProtocol());
}

TEST(SolidSyslogLwipSocketDatagram, SendToWritesThePayloadToTheAddressOnTheOpenSocket)
{
    LwipSocketsFake_SetSocketResult(TEST_DESCRIPTOR);
    CHECK_TRUE(SolidSyslogDatagram_Open(datagram));

    LONGS_EQUAL(SOLIDSYSLOG_DATAGRAM_SEND_RESULT_SENT, SendTo());

    LONGS_EQUAL(1U, LwipSocketsFake_SendToCallCount());
    LONGS_EQUAL(TEST_DESCRIPTOR, LwipSocketsFake_LastSendToSocket());
    MEMCMP_EQUAL(TEST_PAYLOAD, LwipSocketsFake_LastSendToPayload(), TEST_PAYLOAD_SIZE);
    LONGS_EQUAL(TEST_PAYLOAD_SIZE, LwipSocketsFake_LastSendToSize());
    LONGS_EQUAL(0, LwipSocketsFake_LastSendToFlags());
    LONGS_EQUAL(lwip_htons(TEST_PORT), LwipSocketsFake_LastSendToAddress()->sin_port);
    LONGS_EQUAL(lwip_htonl(TEST_IPV4), LwipSocketsFake_LastSendToAddress()->sin_addr.s_addr);
    LONGS_EQUAL(sizeof(struct sockaddr_in), LwipSocketsFake_LastSendToAddressLength());
}

TEST(SolidSyslogLwipSocketDatagram, CloseClosesTheOpenSocket)
{
    LwipSocketsFake_SetSocketResult(TEST_DESCRIPTOR);
    CHECK_TRUE(SolidSyslogDatagram_Open(datagram));

    SolidSyslogDatagram_Close(datagram);

    LONGS_EQUAL(1U, LwipSocketsFake_CloseCallCount());
    LONGS_EQUAL(TEST_DESCRIPTOR, LwipSocketsFake_LastClosedSocket());
}

TEST(SolidSyslogLwipSocketDatagram, MaxPayloadIsTheUnknownPathAnswerBecauseTheStackCannotReportAPathMtu)
{
    CHECK_TRUE(SolidSyslogDatagram_Open(datagram));

    LONGS_EQUAL(SolidSyslogUdpPayload_UnknownPath(false), SolidSyslogDatagram_MaxPayload(datagram));
}

TEST(SolidSyslogLwipSocketDatagram, OpenIsRefusedWhenTheStackWillNotGiveASocket)
{
    LwipSocketsFake_SetSocketResult(-1);

    CHECK_FALSE(SolidSyslogDatagram_Open(datagram));
}

TEST(SolidSyslogLwipSocketDatagram, SendToReportsOversizeWhenTheDatagramIsTooLongForTheStack)
{
    CHECK_TRUE(SolidSyslogDatagram_Open(datagram));
    LwipSocketsFake_SetSendToFailure(EMSGSIZE);

    LONGS_EQUAL(SOLIDSYSLOG_DATAGRAM_SEND_RESULT_OVERSIZE, SendTo());
}

TEST(SolidSyslogLwipSocketDatagram, SendToReportsFailureWhenTheStackHasNoBufferForIt)
{
    CHECK_TRUE(SolidSyslogDatagram_Open(datagram));
    LwipSocketsFake_SetSendToFailure(ENOBUFS);

    LONGS_EQUAL(SOLIDSYSLOG_DATAGRAM_SEND_RESULT_FAILED, SendTo());
}

// clang-format off
TEST_GROUP(SolidSyslogLwipSocketDatagramPool)
{
    struct SolidSyslogDatagram* pooled[SOLIDSYSLOG_DATAGRAM_POOL_SIZE] = {};
    struct SolidSyslogDatagram* overflow                               = nullptr;

    void teardown() override
    {
        for (auto* handle : pooled)
        {
            if (handle != nullptr)
            {
                SolidSyslogLwipSocketDatagram_Destroy(handle);
            }
        }
        ConfigLockFake_Uninstall();
    }

    void FillPool()
    {
        for (auto*& slot : pooled)
        {
            slot = SolidSyslogLwipSocketDatagram_Create();
        }
    }
};

// clang-format on

TEST(SolidSyslogLwipSocketDatagramPool, FillingPoolThenOverflowReturnsTheNullDatagram)
{
    FillPool();

    overflow = SolidSyslogLwipSocketDatagram_Create();

    POINTERS_EQUAL(SolidSyslogNullDatagram_Get(), overflow);
}

TEST(SolidSyslogLwipSocketDatagramPool, ExhaustedCreateReportsError)
{
    ErrorHandlerFake_Install(nullptr);
    FillPool();

    overflow = SolidSyslogLwipSocketDatagram_Create();

    CHECK_ERROR_REPORTED_ONCE(
        SOLIDSYSLOG_SEVERITY_CRITICAL,
        &SolidSyslogLwipSocketDatagramErrorSource,
        SOLIDSYSLOG_CAT_POOL_EXHAUSTED,
        SOLIDSYSLOG_DATAGRAM_ERROR_POOL_EXHAUSTED
    );
}

TEST(SolidSyslogLwipSocketDatagramPool, CreateAcquiresAndReleasesConfigLockOnFirstFreeSlot)
{
    ConfigLockFake_Install();

    pooled[0] = SolidSyslogLwipSocketDatagram_Create();

    CALLED_FAKE(ConfigLockFake_Lock, ONCE);
    CALLED_FAKE(ConfigLockFake_Unlock, ONCE);
}

TEST(SolidSyslogLwipSocketDatagramPool, CreateLocksOncePerSlotProbedWhenPoolIsFull)
{
    FillPool();
    ConfigLockFake_Install();

    overflow = SolidSyslogLwipSocketDatagram_Create();

    LONGS_EQUAL(SOLIDSYSLOG_DATAGRAM_POOL_SIZE, ConfigLockFake_LockCallCount());
    LONGS_EQUAL(SOLIDSYSLOG_DATAGRAM_POOL_SIZE, ConfigLockFake_UnlockCallCount());
}

TEST(SolidSyslogLwipSocketDatagramPool, DestroyOfPooledHandleLocksOnce)
{
    pooled[0] = SolidSyslogLwipSocketDatagram_Create();
    ConfigLockFake_Install();

    SolidSyslogLwipSocketDatagram_Destroy(pooled[0]);
    pooled[0] = nullptr;

    CALLED_FAKE(ConfigLockFake_Lock, ONCE);
    CALLED_FAKE(ConfigLockFake_Unlock, ONCE);
}

TEST(SolidSyslogLwipSocketDatagramPool, DestroyOfUnknownHandleDoesNotLock)
{
    ConfigLockFake_Install();
    char stranger = 0;

    SolidSyslogLwipSocketDatagram_Destroy((struct SolidSyslogDatagram*) &stranger);

    CALLED_FAKE(ConfigLockFake_Lock, NEVER);
    CALLED_FAKE(ConfigLockFake_Unlock, NEVER);
}

TEST(SolidSyslogLwipSocketDatagramPool, DestroyOfUnknownHandleReportsWarning)
{
    ErrorHandlerFake_Install(nullptr);
    char stranger = 0;

    SolidSyslogLwipSocketDatagram_Destroy((struct SolidSyslogDatagram*) &stranger);

    CHECK_ERROR_REPORTED_ONCE(
        SOLIDSYSLOG_SEVERITY_WARNING,
        &SolidSyslogLwipSocketDatagramErrorSource,
        SOLIDSYSLOG_CAT_UNKNOWN_DESTROY,
        SOLIDSYSLOG_DATAGRAM_ERROR_UNKNOWN_DESTROY
    );
}

TEST(SolidSyslogLwipSocketDatagramPool, DestroyOfStaleHandleReportsWarning)
{
    pooled[0] = SolidSyslogLwipSocketDatagram_Create();
    SolidSyslogLwipSocketDatagram_Destroy(pooled[0]);
    ErrorHandlerFake_Install(nullptr);

    SolidSyslogLwipSocketDatagram_Destroy(pooled[0]);
    pooled[0] = nullptr;

    CHECK_ERROR_REPORTED_ONCE(
        SOLIDSYSLOG_SEVERITY_WARNING,
        &SolidSyslogLwipSocketDatagramErrorSource,
        SOLIDSYSLOG_CAT_UNKNOWN_DESTROY,
        SOLIDSYSLOG_DATAGRAM_ERROR_UNKNOWN_DESTROY
    );
}
