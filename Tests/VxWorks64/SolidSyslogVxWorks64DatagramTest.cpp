#include "TestUtils.h"
#include "CppUTest/TestHarness.h"

using namespace CososoTesting;

#include "vxWorks.h"

#include <errno.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include "ConfigLockFake.h"
#include "ErrorHandlerFake.h"
#include "SolidSyslogAddress.h"
#include "SolidSyslogDatagram.h"
#include "SolidSyslogDatagramDefinition.h"
#include "SolidSyslogErrorCategory.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogTunables.h"
#include "SolidSyslogUdpPayload.h"
#include "SolidSyslogVxWorks64Address.h"
#include "SolidSyslogVxWorks64AddressPrivate.h"
#include "SolidSyslogVxWorks64Datagram.h"
#include "SolidSyslogVxWorks64DatagramErrors.h"
#include "VxWorks64IoFake.h"
#include "VxWorks64NetFake.h"

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

static const char TEST_MESSAGE[] = "hello";
// Longer than any payload limit the datagram reports, so a test can send one byte past it.
static const char TEST_LONG_RECORD[2048] = {0};

// clang-format off
TEST_GROUP(SolidSyslogVxWorks64Datagram)
{
    struct SolidSyslogDatagram* datagram = nullptr;
    struct SolidSyslogAddress* address   = nullptr;

    void setup() override
    {
        VxWorks64NetFake_Reset();
        VxWorks64IoFake_Reset();
        datagram = SolidSyslogVxWorks64Datagram_Create();
        address  = SolidSyslogVxWorks64Address_Create();
        struct sockaddr_in* sin = SolidSyslogVxWorks64Address_AsSockaddrIn(address);
        sin->sin_family         = AF_INET;
        sin->sin_port           = htons(6514U);
        sin->sin_addr.s_addr    = htonl(0x0A00000AU);
    }

    void teardown() override
    {
        SolidSyslogVxWorks64Address_Destroy(address);
        SolidSyslogVxWorks64Datagram_Destroy(datagram);
    }

    [[nodiscard]] enum SolidSyslogDatagramSendResult OpenAndSend() const
    {
        SolidSyslogDatagram_Open(datagram);
        return SolidSyslogDatagram_SendTo(datagram, TEST_MESSAGE, sizeof(TEST_MESSAGE) - 1U, address);
    }

    [[nodiscard]] enum SolidSyslogDatagramSendResult OpenAndSendRecordOf(size_t size) const
    {
        CHECK(size <= sizeof(TEST_LONG_RECORD));
        SolidSyslogDatagram_Open(datagram);
        return SolidSyslogDatagram_SendTo(datagram, TEST_LONG_RECORD, size, address);
    }
};

// clang-format on

TEST(SolidSyslogVxWorks64Datagram, OpenMakesOneSocket)
{
    SolidSyslogDatagram_Open(datagram);

    CALLED_FAKE(VxWorks64NetFake_Socket, ONCE);
}

TEST(SolidSyslogVxWorks64Datagram, OpenAsksForAnIpv4DatagramSocket)
{
    SolidSyslogDatagram_Open(datagram);

    LONGS_EQUAL(AF_INET, VxWorks64NetFake_LastSocketDomain());
    LONGS_EQUAL(SOCK_DGRAM, VxWorks64NetFake_LastSocketType());
    LONGS_EQUAL(0, VxWorks64NetFake_LastSocketProtocol());
}

TEST(SolidSyslogVxWorks64Datagram, OpenFailsWhenTheStackCannotMakeASocket)
{
    VxWorks64NetFake_SetSocketFails(true);

    CHECK_FALSE(SolidSyslogDatagram_Open(datagram));
}

TEST(SolidSyslogVxWorks64Datagram, OpenSucceedsWhenTheStackMakesASocket)
{
    CHECK_TRUE(SolidSyslogDatagram_Open(datagram));
}

TEST(SolidSyslogVxWorks64Datagram, SendToSendsTheRecord)
{
    (void) OpenAndSend();

    CALLED_FAKE(VxWorks64NetFake_Sendto, ONCE);
    POINTERS_EQUAL(TEST_MESSAGE, VxWorks64NetFake_LastSendtoBuf());
    LONGS_EQUAL(5, VxWorks64NetFake_LastSendtoLen());
}

TEST(SolidSyslogVxWorks64Datagram, SendToUsesTheSocketOpenMade)
{
    (void) OpenAndSend();

    LONGS_EQUAL(VxWorks64NetFake_SocketFd(), VxWorks64NetFake_LastSendtoFd());
}

TEST(SolidSyslogVxWorks64Datagram, SendToAddressesTheResolvedDestination)
{
    (void) OpenAndSend();

    POINTERS_EQUAL(SolidSyslogVxWorks64Address_AsConstSockaddrIn(address), VxWorks64NetFake_LastSendtoTo());
    LONGS_EQUAL(sizeof(struct sockaddr_in), VxWorks64NetFake_LastSendtoToLen());
}

TEST(SolidSyslogVxWorks64Datagram, SendToReportsSentWhenTheStackTakesTheRecord)
{
    LONGS_EQUAL(SOLIDSYSLOG_DATAGRAM_SEND_RESULT_SENT, OpenAndSend());
}

TEST(SolidSyslogVxWorks64Datagram, SendToReportsOversizeWhenTheStackRefusesTheSize)
{
    VxWorks64NetFake_FailSendtoWithErrno(EMSGSIZE);

    LONGS_EQUAL(SOLIDSYSLOG_DATAGRAM_SEND_RESULT_OVERSIZE, OpenAndSend());
}

TEST(SolidSyslogVxWorks64Datagram, SendToReportsFailedForAnyOtherRefusal)
{
    VxWorks64NetFake_FailSendtoWithErrno(ENETUNREACH);

    LONGS_EQUAL(SOLIDSYSLOG_DATAGRAM_SEND_RESULT_FAILED, OpenAndSend());
}

TEST(SolidSyslogVxWorks64Datagram, SendToRefusesARecordLargerThanMaxPayloadWithoutSendingIt)
{
    LONGS_EQUAL(
        SOLIDSYSLOG_DATAGRAM_SEND_RESULT_OVERSIZE,
        OpenAndSendRecordOf(SolidSyslogDatagram_MaxPayload(datagram) + 1U)
    );
    CALLED_FAKE(VxWorks64NetFake_Sendto, NEVER);
}

TEST(SolidSyslogVxWorks64Datagram, SendToSendsARecordOfExactlyMaxPayload)
{
    LONGS_EQUAL(SOLIDSYSLOG_DATAGRAM_SEND_RESULT_SENT, OpenAndSendRecordOf(SolidSyslogDatagram_MaxPayload(datagram)));
    CALLED_FAKE(VxWorks64NetFake_Sendto, ONCE);
}

TEST(SolidSyslogVxWorks64Datagram, SendToPassesNoFlags)
{
    (void) OpenAndSend();

    LONGS_EQUAL(0, VxWorks64NetFake_LastSendtoFlags());
}

TEST(SolidSyslogVxWorks64Datagram, MaxPayloadIsTheUnknownPathPayload)
{
    SolidSyslogDatagram_Open(datagram);

    LONGS_EQUAL(SolidSyslogUdpPayload_UnknownPath(false), SolidSyslogDatagram_MaxPayload(datagram));
}

TEST(SolidSyslogVxWorks64Datagram, CloseClosesTheSocketOpenMade)
{
    SolidSyslogDatagram_Open(datagram);

    SolidSyslogDatagram_Close(datagram);

    CALLED_FAKE(VxWorks64IoFake_Close, ONCE);
    LONGS_EQUAL(VxWorks64NetFake_SocketFd(), VxWorks64IoFake_LastClosedFd());
}

// The Datagram contract: Close is idempotent and safe on an unopened datagram,
// because the caller's failure paths call it.
TEST(SolidSyslogVxWorks64Datagram, CloseOnAnUnopenedDatagramClosesNothing)
{
    SolidSyslogDatagram_Close(datagram);

    CALLED_FAKE(VxWorks64IoFake_Close, NEVER);
}

TEST(SolidSyslogVxWorks64Datagram, CloseTwiceClosesOnce)
{
    SolidSyslogDatagram_Open(datagram);

    SolidSyslogDatagram_Close(datagram);
    SolidSyslogDatagram_Close(datagram);

    CALLED_FAKE(VxWorks64IoFake_Close, ONCE);
}

TEST(SolidSyslogVxWorks64Datagram, DestroyClosesAnOpenSocket)
{
    SolidSyslogDatagram_Open(datagram);

    SolidSyslogVxWorks64Datagram_Destroy(datagram);
    datagram = nullptr;

    CALLED_FAKE(VxWorks64IoFake_Close, ONCE);
}

TEST(SolidSyslogVxWorks64Datagram, SendToAfterDestroySendsNothing)
{
    SolidSyslogDatagram_Open(datagram);
    struct SolidSyslogDatagram* stale = datagram;
    SolidSyslogVxWorks64Datagram_Destroy(datagram);
    datagram = nullptr;

    (void) SolidSyslogDatagram_SendTo(stale, TEST_MESSAGE, sizeof(TEST_MESSAGE) - 1U, address);

    CALLED_FAKE(VxWorks64NetFake_Sendto, NEVER);
}

// clang-format off
TEST_GROUP(SolidSyslogVxWorks64DatagramPool)
{
    struct SolidSyslogDatagram* pooled[SOLIDSYSLOG_DATAGRAM_POOL_SIZE] = {};
    struct SolidSyslogDatagram* overflow                                     = nullptr;

    void teardown() override
    {
        for (auto* handle : pooled)
        {
            if (handle != nullptr)
            {
                SolidSyslogVxWorks64Datagram_Destroy(handle);
            }
        }
        if (overflow != nullptr)
        {
            SolidSyslogVxWorks64Datagram_Destroy(overflow);
        }
        ConfigLockFake_Uninstall();
    }

    void FillPool()
    {
        for (auto*& slot : pooled)
        {
            slot = SolidSyslogVxWorks64Datagram_Create();
        }
    }
};

// clang-format on

TEST(SolidSyslogVxWorks64DatagramPool, FillingPoolThenOverflowReturnsDistinctFallback)
{
    FillPool();

    overflow = SolidSyslogVxWorks64Datagram_Create();

    CHECK_IS_FALLBACK(overflow, pooled);
}

TEST(SolidSyslogVxWorks64DatagramPool, ExhaustedCreateReportsError)
{
    ErrorHandlerFake_Install(nullptr);
    FillPool();

    overflow = SolidSyslogVxWorks64Datagram_Create();

    CHECK_ERROR_REPORTED_ONCE(
        SOLIDSYSLOG_SEVERITY_CRITICAL,
        &SolidSyslogVxWorks64DatagramErrorSource,
        SOLIDSYSLOG_CAT_POOL_EXHAUSTED,
        SOLIDSYSLOG_DATAGRAM_ERROR_POOL_EXHAUSTED
    );
}

TEST(SolidSyslogVxWorks64DatagramPool, FallbackSendToReturnsSent)
{
    FillPool();
    overflow = SolidSyslogVxWorks64Datagram_Create();

    LONGS_EQUAL(SOLIDSYSLOG_DATAGRAM_SEND_RESULT_SENT, SolidSyslogDatagram_SendTo(overflow, "x", 1, nullptr));
}

TEST(SolidSyslogVxWorks64DatagramPool, CreateAcquiresAndReleasesConfigLockOnFirstFreeSlot)
{
    ConfigLockFake_Install();

    pooled[0] = SolidSyslogVxWorks64Datagram_Create();

    CALLED_FAKE(ConfigLockFake_Lock, ONCE);
    CALLED_FAKE(ConfigLockFake_Unlock, ONCE);
}

TEST(SolidSyslogVxWorks64DatagramPool, CreateLocksOncePerSlotProbedWhenPoolIsFull)
{
    FillPool();
    ConfigLockFake_Install();

    overflow = SolidSyslogVxWorks64Datagram_Create();

    LONGS_EQUAL(SOLIDSYSLOG_DATAGRAM_POOL_SIZE, ConfigLockFake_LockCallCount());
    LONGS_EQUAL(SOLIDSYSLOG_DATAGRAM_POOL_SIZE, ConfigLockFake_UnlockCallCount());
}

TEST(SolidSyslogVxWorks64DatagramPool, DestroyOfPooledHandleLocksOnce)
{
    pooled[0] = SolidSyslogVxWorks64Datagram_Create();
    ConfigLockFake_Install();

    SolidSyslogVxWorks64Datagram_Destroy(pooled[0]);
    pooled[0] = nullptr;

    CALLED_FAKE(ConfigLockFake_Lock, ONCE);
    CALLED_FAKE(ConfigLockFake_Unlock, ONCE);
}

TEST(SolidSyslogVxWorks64DatagramPool, DestroyOfUnknownHandleDoesNotLock)
{
    ConfigLockFake_Install();
    struct SolidSyslogDatagram stranger = {};

    SolidSyslogVxWorks64Datagram_Destroy(&stranger);

    CALLED_FAKE(ConfigLockFake_Lock, NEVER);
    CALLED_FAKE(ConfigLockFake_Unlock, NEVER);
}

TEST(SolidSyslogVxWorks64DatagramPool, DestroyOfUnknownHandleReportsWarning)
{
    ErrorHandlerFake_Install(nullptr);
    struct SolidSyslogDatagram stranger = {};

    SolidSyslogVxWorks64Datagram_Destroy(&stranger);

    CHECK_ERROR_REPORTED_ONCE(
        SOLIDSYSLOG_SEVERITY_WARNING,
        &SolidSyslogVxWorks64DatagramErrorSource,
        SOLIDSYSLOG_CAT_UNKNOWN_DESTROY,
        SOLIDSYSLOG_DATAGRAM_ERROR_UNKNOWN_DESTROY
    );
}

TEST(SolidSyslogVxWorks64DatagramPool, DestroyOfStaleHandleReportsWarning)
{
    pooled[0] = SolidSyslogVxWorks64Datagram_Create();
    SolidSyslogVxWorks64Datagram_Destroy(pooled[0]);
    ErrorHandlerFake_Install(nullptr);

    SolidSyslogVxWorks64Datagram_Destroy(pooled[0]);
    pooled[0] = nullptr;

    CHECK_ERROR_REPORTED_ONCE(
        SOLIDSYSLOG_SEVERITY_WARNING,
        &SolidSyslogVxWorks64DatagramErrorSource,
        SOLIDSYSLOG_CAT_UNKNOWN_DESTROY,
        SOLIDSYSLOG_DATAGRAM_ERROR_UNKNOWN_DESTROY
    );
}
