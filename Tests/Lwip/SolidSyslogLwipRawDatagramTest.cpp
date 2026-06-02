#include <stdint.h>
#include <string.h>

#include "TestUtils.h"
#include "CppUTest/TestHarness.h"

using namespace CososoTesting;

#include "ConfigLockFake.h"
#include "ErrorHandlerFake.h"
#include "LwipFakeMarshalGuard.h"
#include "LwipPbufFake.h"
#include "LwipUdpFake.h"
#include "SolidSyslogDatagram.h"
#include "SolidSyslogDatagramDefinition.h"
#include "SolidSyslogErrorCategory.h"
#include "SolidSyslogLwipRawAddress.h"
#include "SolidSyslogLwipRawAddressPrivate.h"
#include "SolidSyslogLwipRawDatagram.h"
#include "SolidSyslogLwipRawDatagramErrors.h"
#include "SolidSyslogNullDatagram.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogTunables.h"
#include "SolidSyslogUdpPayload.h"
#include "lwip/err.h"
#include "lwip/ip4_addr.h"
#include "lwip/pbuf.h"

static const uint16_t TEST_PORT = 514;

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

// Asserts the most recent ErrorHandlerFake call matched (severity, source, code).
// Use after the act-phase of a test that expects exactly one SolidSyslog_Error call.
#define CHECK_REPORTED(severity, source, expectedCategory, code)                   \
    do                                                                             \
    {                                                                              \
        CALLED_FAKE(ErrorHandlerFake_Handle, ONCE);                                \
        LONGS_EQUAL((severity), ErrorHandlerFake_LastSeverity());                  \
        POINTERS_EQUAL(&(source), ErrorHandlerFake_LastSource());                  \
        UNSIGNED_LONGS_EQUAL((expectedCategory), ErrorHandlerFake_LastCategory()); \
        UNSIGNED_LONGS_EQUAL((code), ErrorHandlerFake_LastDetail());               \
    } while (0)

/* Shared fixture: every Datagram lifecycle test needs the fake reset, a fresh
 * datagram + address handle pair, teardown of both, and the leak invariant
 * — every udp_pcb handed out by udp_new must come back via udp_remove by the
 * end of the test. TEST_GROUP_BASE keeps Created-only vs Opened groups
 * sharing this boilerplate. */
// clang-format off
TEST_BASE(LwipRawDatagramTestBase)
{
    struct SolidSyslogDatagram* datagram = nullptr;
    struct SolidSyslogAddress* address = nullptr;
    /* Shared scratch buffer for SendTo tests. Sized to MaxPayload so the
     * largest-length test (sendBytes(SOLIDSYSLOG_UDP_IPV6_SAFE_PAYLOAD))
     * stays in-bounds. Content is irrelevant — payload-pointer-identity
     * and length are the observable surface. */
    char sendBuffer[SOLIDSYSLOG_UDP_IPV6_SAFE_PAYLOAD] = {};

    void createFakesAndHandles()
    {
        LwipUdpFake_Reset();
        LwipPbufFake_Reset();
        LwipFakeMarshalGuard_Reset();
        SolidSyslogLwipRaw_SetMarshal(LwipFakeMarshalGuard_TrackingMarshal);
        datagram = SolidSyslogLwipRawDatagram_Create();
        address = SolidSyslogLwipRawAddress_Create();
        struct SolidSyslogLwipRawAddress* lwipAddress = SolidSyslogLwipRawAddress_As(address);
        IP4_ADDR(&lwipAddress->Ip, 127, 0, 0, 1);
        lwipAddress->Port = TEST_PORT;
    }

    void destroyHandlesAndCheckNoLeak() const
    {
        SolidSyslogLwipRawAddress_Destroy(address);
        SolidSyslogLwipRawDatagram_Destroy(datagram);
        LONGS_EQUAL_TEXT(0, LwipUdpFake_OutstandingPcbCount(), "leaked udp_pcb past teardown");
        LONGS_EQUAL_TEXT(0, LwipPbufFake_OutstandingPbufCount(), "leaked pbuf past teardown");
        LwipFakeMarshalGuard_CheckNoBreach();
        SolidSyslogLwipRaw_SetMarshal(nullptr);
    }

    /* SendTo against the shared buffer + address. Default length 1 — most
     * tests don't care; tests asserting length pass it explicitly. */
    enum SolidSyslogDatagramSendResult sendBytes(size_t length = 1U)
    {
        return SolidSyslogDatagram_SendTo(datagram, sendBuffer, length, address);
    }
};

TEST_GROUP_BASE(SolidSyslogLwipRawDatagram, LwipRawDatagramTestBase)
{
    void setup() override
    {
        createFakesAndHandles();
    }

    void teardown() override
    {
        destroyHandlesAndCheckNoLeak();
    }
};

TEST_GROUP_BASE(SolidSyslogLwipRawDatagramOpen, LwipRawDatagramTestBase)
{
    void setup() override
    {
        createFakesAndHandles();
        SolidSyslogDatagram_Open(datagram);
    }

    void teardown() override
    {
        destroyHandlesAndCheckNoLeak();
    }
};
// clang-format on

/* ------------------------------------------------------------------
 * Created-but-not-opened tests
 * ----------------------------------------------------------------*/

TEST(SolidSyslogLwipRawDatagram, CreateReturnsNonNullDatagram)
{
    CHECK(datagram != nullptr);
}

TEST(SolidSyslogLwipRawDatagram, DestroyReleasesSlotToPool)
{
    SolidSyslogLwipRawDatagram_Destroy(datagram);

    datagram = SolidSyslogLwipRawDatagram_Create();

    CHECK(datagram != SolidSyslogNullDatagram_Get());
}

TEST(SolidSyslogLwipRawDatagram, OpenSucceeds)
{
    CHECK_TRUE(SolidSyslogDatagram_Open(datagram));
}

TEST(SolidSyslogLwipRawDatagram, CloseSucceedsWithoutOpen)
{
    SolidSyslogDatagram_Close(datagram);
}

TEST(SolidSyslogLwipRawDatagram, OpenCallsUdpNew)
{
    SolidSyslogDatagram_Open(datagram);

    CALLED_FAKE(LwipUdpFake_UdpNew, ONCE);
}

TEST(SolidSyslogLwipRawDatagram, CloseWithoutOpenIsNoOp)
{
    SolidSyslogDatagram_Close(datagram);

    CALLED_FAKE(LwipUdpFake_UdpRemove, NEVER);
}

TEST(SolidSyslogLwipRawDatagram, OpenReturnsFalseWhenUdpNewFails)
{
    LwipUdpFake_SetUdpNewFails(true);

    CHECK_FALSE(SolidSyslogDatagram_Open(datagram));
}

TEST(SolidSyslogLwipRawDatagram, MaxPayloadReturnsIpv6SafeDefault)
{
    LONGS_EQUAL(SOLIDSYSLOG_UDP_IPV6_SAFE_PAYLOAD, SolidSyslogDatagram_MaxPayload(datagram));
}

TEST(SolidSyslogLwipRawDatagram, SendToFailsBeforeOpen)
{
    LONGS_EQUAL(SOLIDSYSLOG_DATAGRAM_SEND_RESULT_FAILED, sendBytes());
    CALLED_FAKE(LwipPbufFake_PbufAlloc, NEVER);
    CALLED_FAKE(LwipUdpFake_UdpSendto, NEVER);
}

/* ------------------------------------------------------------------
 * Already-opened tests (Open lifted into the group's setup)
 * ----------------------------------------------------------------*/

TEST(SolidSyslogLwipRawDatagramOpen, CloseCallsUdpRemoveOnOpenPcb)
{
    SolidSyslogDatagram_Close(datagram);

    CALLED_FAKE(LwipUdpFake_UdpRemove, ONCE);
    POINTERS_EQUAL(LwipUdpFake_LastUdpRemovePcb(), LwipUdpFake_LastUdpNewReturned());
}

TEST(SolidSyslogLwipRawDatagramOpen, ReopenDoesNotAllocateNewPcb)
{
    CHECK_TRUE(SolidSyslogDatagram_Open(datagram));

    CALLED_FAKE(LwipUdpFake_UdpNew, ONCE);
}

TEST(SolidSyslogLwipRawDatagramOpen, SecondCloseDoesNotRemoveAgain)
{
    SolidSyslogDatagram_Close(datagram);

    SolidSyslogDatagram_Close(datagram);

    CALLED_FAKE(LwipUdpFake_UdpRemove, ONCE);
}

TEST(SolidSyslogLwipRawDatagramOpen, CloseThenOpenAllocatesFreshPcb)
{
    SolidSyslogDatagram_Close(datagram);

    SolidSyslogDatagram_Open(datagram);

    CALLED_FAKE(LwipUdpFake_UdpNew, TWICE);
}

TEST(SolidSyslogLwipRawDatagramOpen, DestroyClosesOpenPcb)
{
    SolidSyslogLwipRawDatagram_Destroy(datagram);
    datagram = nullptr;

    CALLED_FAKE(LwipUdpFake_UdpRemove, ONCE);
}

TEST(SolidSyslogLwipRawDatagramOpen, DestroyAfterCloseDoesNotRemoveAgain)
{
    SolidSyslogDatagram_Close(datagram);

    SolidSyslogLwipRawDatagram_Destroy(datagram);
    datagram = nullptr;

    CALLED_FAKE(LwipUdpFake_UdpRemove, ONCE);
}

TEST(SolidSyslogLwipRawDatagramOpen, SendToSucceeds)
{
    LONGS_EQUAL(SOLIDSYSLOG_DATAGRAM_SEND_RESULT_SENT, sendBytes());
}

TEST(SolidSyslogLwipRawDatagramOpen, SendToCallsUdpSendto)
{
    sendBytes();

    CALLED_FAKE(LwipUdpFake_UdpSendto, ONCE);
}

TEST(SolidSyslogLwipRawDatagramOpen, SendToAllocatesPbuf)
{
    sendBytes();

    CALLED_FAKE(LwipPbufFake_PbufAlloc, ONCE);
}

TEST(SolidSyslogLwipRawDatagramOpen, SendToAllocatesTransportLayerPbufRef)
{
    sendBytes();

    LONGS_EQUAL(PBUF_TRANSPORT, LwipPbufFake_LastAllocLayer());
    LONGS_EQUAL(PBUF_REF, LwipPbufFake_LastAllocType());
}

TEST(SolidSyslogLwipRawDatagramOpen, SendToPbufPayloadPointsAtCallerBuffer)
{
    memcpy(sendBuffer, "hello", 5);

    sendBytes(5);

    POINTERS_EQUAL(sendBuffer, LwipPbufFake_LastAllocReturned()->payload);
}

TEST(SolidSyslogLwipRawDatagramOpen, SendToForwardsAddressIpAndPort)
{
    sendBytes();

    POINTERS_EQUAL(&SolidSyslogLwipRawAddress_AsConst(address)->Ip, LwipUdpFake_LastSendtoIpaddr());
    LONGS_EQUAL(TEST_PORT, LwipUdpFake_LastSendtoPort());
}

TEST(SolidSyslogLwipRawDatagramOpen, SendToForwardsLengthVerbatim)
{
    sendBytes(1);
    LONGS_EQUAL(1, LwipPbufFake_LastAllocLength());

    sendBytes(SOLIDSYSLOG_UDP_IPV6_SAFE_PAYLOAD);
    LONGS_EQUAL(SOLIDSYSLOG_UDP_IPV6_SAFE_PAYLOAD, LwipPbufFake_LastAllocLength());
}

TEST(SolidSyslogLwipRawDatagramOpen, SendToFreesPbufAfterSendto)
{
    sendBytes();

    CALLED_FAKE(LwipPbufFake_PbufFree, ONCE);
}

TEST(SolidSyslogLwipRawDatagramOpen, SendToFailsAfterClose)
{
    SolidSyslogDatagram_Close(datagram);

    LONGS_EQUAL(SOLIDSYSLOG_DATAGRAM_SEND_RESULT_FAILED, sendBytes());
}

TEST(SolidSyslogLwipRawDatagramOpen, SendToFailsWhenPbufAllocFails)
{
    LwipPbufFake_SetPbufAllocFails(true);

    LONGS_EQUAL(SOLIDSYSLOG_DATAGRAM_SEND_RESULT_FAILED, sendBytes());
    CALLED_FAKE(LwipUdpFake_UdpSendto, NEVER);
}

TEST(SolidSyslogLwipRawDatagramOpen, SendToFailsWhenSendtoErrors)
{
    LwipUdpFake_SetUdpSendtoError(ERR_MEM);

    LONGS_EQUAL(SOLIDSYSLOG_DATAGRAM_SEND_RESULT_FAILED, sendBytes());
}

TEST(SolidSyslogLwipRawDatagramOpen, SendToFreesPbufEvenWhenSendtoErrors)
{
    LwipUdpFake_SetUdpSendtoError(ERR_MEM);

    sendBytes();

    CALLED_FAKE(LwipPbufFake_PbufFree, ONCE);
}

// clang-format off
TEST_GROUP(SolidSyslogLwipRawDatagramPool)
{
    struct SolidSyslogDatagram* pooled[SOLIDSYSLOG_DATAGRAM_POOL_SIZE] = {};
    struct SolidSyslogDatagram* overflow                                         = nullptr;

    void setup() override
    {
        LwipUdpFake_Reset();
    }

    void teardown() override
    {
        for (auto* handle : pooled)
        {
            if (handle != nullptr)
            {
                SolidSyslogLwipRawDatagram_Destroy(handle);
            }
        }
        if (overflow != nullptr)
        {
            SolidSyslogLwipRawDatagram_Destroy(overflow);
        }
        ConfigLockFake_Uninstall();
    }

    void FillPool()
    {
        for (auto*& slot : pooled)
        {
            slot = SolidSyslogLwipRawDatagram_Create();
        }
    }
};

// clang-format on

TEST(SolidSyslogLwipRawDatagramPool, FillingPoolThenOverflowReturnsDistinctFallback)
{
    FillPool();

    overflow = SolidSyslogLwipRawDatagram_Create();

    CHECK_IS_FALLBACK(overflow, pooled);
}

TEST(SolidSyslogLwipRawDatagramPool, ExhaustedCreateReportsError)
{
    ErrorHandlerFake_Install(nullptr);
    FillPool();

    overflow = SolidSyslogLwipRawDatagram_Create();

    CHECK_REPORTED(
        SOLIDSYSLOG_SEVERITY_ERROR,
        LwipRawDatagramErrorSource,
        SOLIDSYSLOG_CAT_POOL_EXHAUSTED,
        LWIPRAWDATAGRAM_ERROR_POOL_EXHAUSTED
    );
}

TEST(SolidSyslogLwipRawDatagramPool, FallbackVtableMethodsAreNoOps)
{
    FillPool();
    overflow = SolidSyslogLwipRawDatagram_Create();
    struct SolidSyslogAddress* localAddr = SolidSyslogLwipRawAddress_Create();

    /* NullDatagram's Open returns true so caller success paths are not
     * tripped; SendTo returns SENT; no underlying lwIP API is invoked
     * because the production-side vtable is never wired on the fallback
     * handle. */
    CHECK_TRUE(SolidSyslogDatagram_Open(overflow));
    LONGS_EQUAL(SOLIDSYSLOG_DATAGRAM_SEND_RESULT_SENT, SolidSyslogDatagram_SendTo(overflow, "x", 1, localAddr));
    SolidSyslogDatagram_Close(overflow);

    SolidSyslogLwipRawAddress_Destroy(localAddr);
}

TEST(SolidSyslogLwipRawDatagramPool, CreateAcquiresAndReleasesConfigLockOnFirstFreeSlot)
{
    ConfigLockFake_Install();

    pooled[0] = SolidSyslogLwipRawDatagram_Create();

    CALLED_FAKE(ConfigLockFake_Lock, ONCE);
    CALLED_FAKE(ConfigLockFake_Unlock, ONCE);
}

TEST(SolidSyslogLwipRawDatagramPool, CreateLocksOncePerSlotProbedWhenPoolIsFull)
{
    FillPool();
    ConfigLockFake_Install();

    overflow = SolidSyslogLwipRawDatagram_Create();

    LONGS_EQUAL(SOLIDSYSLOG_DATAGRAM_POOL_SIZE, ConfigLockFake_LockCallCount());
    LONGS_EQUAL(SOLIDSYSLOG_DATAGRAM_POOL_SIZE, ConfigLockFake_UnlockCallCount());
}

TEST(SolidSyslogLwipRawDatagramPool, DestroyOfPooledHandleLocksOnce)
{
    pooled[0] = SolidSyslogLwipRawDatagram_Create();
    ConfigLockFake_Install();

    SolidSyslogLwipRawDatagram_Destroy(pooled[0]);
    pooled[0] = nullptr;

    CALLED_FAKE(ConfigLockFake_Lock, ONCE);
    CALLED_FAKE(ConfigLockFake_Unlock, ONCE);
}

TEST(SolidSyslogLwipRawDatagramPool, DestroyOfUnknownHandleDoesNotLock)
{
    ConfigLockFake_Install();
    struct SolidSyslogDatagram stranger = {};

    SolidSyslogLwipRawDatagram_Destroy(&stranger);

    CALLED_FAKE(ConfigLockFake_Lock, NEVER);
    CALLED_FAKE(ConfigLockFake_Unlock, NEVER);
}

TEST(SolidSyslogLwipRawDatagramPool, DestroyOfUnknownHandleReportsWarning)
{
    ErrorHandlerFake_Install(nullptr);
    struct SolidSyslogDatagram stranger = {};

    SolidSyslogLwipRawDatagram_Destroy(&stranger);

    CHECK_REPORTED(
        SOLIDSYSLOG_SEVERITY_WARNING,
        LwipRawDatagramErrorSource,
        SOLIDSYSLOG_CAT_UNKNOWN_DESTROY,
        LWIPRAWDATAGRAM_ERROR_UNKNOWN_DESTROY
    );
}

TEST(SolidSyslogLwipRawDatagramPool, DestroyOfStaleHandleReportsWarning)
{
    pooled[0] = SolidSyslogLwipRawDatagram_Create();
    SolidSyslogLwipRawDatagram_Destroy(pooled[0]);
    ErrorHandlerFake_Install(nullptr);

    SolidSyslogLwipRawDatagram_Destroy(pooled[0]);
    pooled[0] = nullptr;

    CHECK_REPORTED(
        SOLIDSYSLOG_SEVERITY_WARNING,
        LwipRawDatagramErrorSource,
        SOLIDSYSLOG_CAT_UNKNOWN_DESTROY,
        LWIPRAWDATAGRAM_ERROR_UNKNOWN_DESTROY
    );
}
