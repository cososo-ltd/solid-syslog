#include "TestUtils.h"
#include "CppUTest/TestHarness.h"

using namespace CososoTesting; // NOLINT(google-build-using-namespace) -- test-file scope only; brings NEVER/ONCE/TWICE/THRICE into scope for the CALLED_*
    // macros

#include "ConfigLockFake.h"
#include "ErrorHandlerFake.h"
#include "SolidSyslogErrorMessages.h"
#include "SolidSyslogFreeRtosAddress.h"
#include "SolidSyslogFreeRtosAddressPrivate.h"
#include "SolidSyslogFreeRtosTcpStream.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogStream.h"
#include "SolidSyslogStreamDefinition.h"
#include "SolidSyslogTunables.h"

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
static const char TEST_MESSAGE[] = "hello";
static const size_t TEST_MESSAGE_LEN = sizeof(TEST_MESSAGE) - 1U;
static const BaseType_t TEST_SHORT_WRITE_BYTES = 3;
static const BaseType_t TEST_READ_BYTES = 7;

// clang-format off
TEST_GROUP(SolidSyslogFreeRtosTcpStream)
{
    struct SolidSyslogStream*           stream = nullptr;
    struct SolidSyslogAddress*          addr = nullptr;
    char                                readBuffer[16] = {0};

    void setup() override
    {
        FreeRtosSocketsFake_Reset();
        FreeRtosArpFake_Reset();
        FreeRtosTaskFake_Reset();
        stream                        = SolidSyslogFreeRtosTcpStream_Create();
        addr                          = SolidSyslogFreeRtosAddress_Create();
        struct freertos_sockaddr* sin = SolidSyslogFreeRtosAddress_AsFreertosSockaddr(addr);
        sin->sin_family               = FREERTOS_AF_INET;
        sin->sin_port                 = FreeRTOS_htons(TEST_PORT);
        sin->sin_address.ulIP_IPv4    = FreeRTOS_inet_addr_quick(10, 0, 2, 2);
    }

    void teardown() override
    {
        SolidSyslogFreeRtosAddress_Destroy(addr);
        if (stream != nullptr)
        {
            SolidSyslogFreeRtosTcpStream_Destroy(stream);
        }
    }

    void openStream() const
    {
        SolidSyslogStream_Open(stream, addr);
    }

    SolidSyslogSsize readIntoBuffer()
    {
        return SolidSyslogStream_Read(stream, readBuffer, sizeof(readBuffer));
    }
};

// clang-format on

// NOLINTBEGIN(cppcoreguidelines-macro-usage,cppcoreguidelines-avoid-do-while)
#define CHECK_SOCKET_CLOSED_ONCE()                                                                             \
    do                                                                                                         \
    {                                                                                                          \
        CALLED_FAKE(FreeRtosSocketsFake_Closesocket, ONCE);                                                    \
        POINTERS_EQUAL(FreeRtosSocketsFake_LastSocketReturned(), FreeRtosSocketsFake_LastClosesocketSocket()); \
    } while (0)

// NOLINTEND(cppcoreguidelines-macro-usage,cppcoreguidelines-avoid-do-while)

TEST(SolidSyslogFreeRtosTcpStream, CreateReturnsNonNullStream)
{
    CHECK(stream != nullptr);
}

TEST(SolidSyslogFreeRtosTcpStream, OpenCreatesTcpSocket)
{
    openStream();
    CALLED_FAKE(FreeRtosSocketsFake_Socket, ONCE);
    LONGS_EQUAL(FREERTOS_AF_INET, FreeRtosSocketsFake_LastSocketDomain());
    LONGS_EQUAL(FREERTOS_SOCK_STREAM, FreeRtosSocketsFake_LastSocketType());
    LONGS_EQUAL(FREERTOS_IPPROTO_TCP, FreeRtosSocketsFake_LastSocketProtocol());
}

TEST(SolidSyslogFreeRtosTcpStream, OpenReturnsTrueOnSuccess)
{
    CHECK_TRUE(SolidSyslogStream_Open(stream, addr));
}

TEST(SolidSyslogFreeRtosTcpStream, OpenReturnsFalseWhenSocketFails)
{
    FreeRtosSocketsFake_SetSocketFails(true);
    CHECK_FALSE(SolidSyslogStream_Open(stream, addr));
}

TEST(SolidSyslogFreeRtosTcpStream, OpenChecksIfDestinationIsInArpCache)
{
    openStream();
    CALLED_FAKE(FreeRtosArpFake_IsIpInArpCache, ONCE);
    LONGS_EQUAL(FreeRTOS_inet_addr_quick(10, 0, 2, 2), FreeRtosArpFake_LastIsIpInArpCacheArg());
}

TEST(SolidSyslogFreeRtosTcpStream, OpenFiresArpProbeOnCacheMiss)
{
    openStream();
    CALLED_FAKE(FreeRtosArpFake_OutputArpRequest, ONCE);
    LONGS_EQUAL(FreeRTOS_inet_addr_quick(10, 0, 2, 2), FreeRtosArpFake_LastOutputArpRequestArg());
}

TEST(SolidSyslogFreeRtosTcpStream, OpenYieldsAfterArpProbeOnCacheMiss)
{
    openStream();
    CALLED_FAKE(FreeRtosTaskFake_VTaskDelay, ONCE);
}

TEST(SolidSyslogFreeRtosTcpStream, OpenSkipsArpProbeAndYieldOnCacheHit)
{
    FreeRtosArpFake_SetCacheHit(true);
    openStream();
    CALLED_FAKE(FreeRtosArpFake_OutputArpRequest, NEVER);
    CALLED_FAKE(FreeRtosTaskFake_VTaskDelay, NEVER);
}

TEST(SolidSyslogFreeRtosTcpStream, OpenSetsConnectTimeoutBeforeConnect)
{
    openStream();
    LONGS_EQUAL(pdMS_TO_TICKS(200), FreeRtosSocketsFake_SndTimeoAtConnect());
}

TEST(SolidSyslogFreeRtosTcpStream, OpenSetsRecvTimeoutBeforeConnect)
{
    openStream();
    LONGS_EQUAL(pdMS_TO_TICKS(200), FreeRtosSocketsFake_RcvTimeoAtConnect());
}

TEST(SolidSyslogFreeRtosTcpStream, OpenCallsConnectWithSocketAndAddress)
{
    openStream();
    CALLED_FAKE(FreeRtosSocketsFake_Connect, ONCE);
    POINTERS_EQUAL(FreeRtosSocketsFake_LastSocketReturned(), FreeRtosSocketsFake_LastConnectSocket());
    POINTERS_EQUAL(SolidSyslogFreeRtosAddress_AsConstFreertosSockaddr(addr), FreeRtosSocketsFake_LastConnectAddress());
    LONGS_EQUAL(sizeof(struct freertos_sockaddr), FreeRtosSocketsFake_LastConnectAddressLength());
}

TEST(SolidSyslogFreeRtosTcpStream, OpenClearsSendTimeoutAfterConnect)
{
    openStream();
    LONGS_EQUAL(0, FreeRtosSocketsFake_LastSndTimeoSet());
}

TEST(SolidSyslogFreeRtosTcpStream, OpenClearsRecvTimeoutAfterConnect)
{
    openStream();
    LONGS_EQUAL(0, FreeRtosSocketsFake_LastRcvTimeoSet());
    LONGS_EQUAL(2, FreeRtosSocketsFake_RcvTimeoSetCallCount());
}

TEST(SolidSyslogFreeRtosTcpStream, OpenCallsSetsockoptWithReturnedSocketAndLevelZero)
{
    openStream();
    POINTERS_EQUAL(FreeRtosSocketsFake_LastSocketReturned(), FreeRtosSocketsFake_LastSetsockoptSocket());
    LONGS_EQUAL(0, FreeRtosSocketsFake_LastSetsockoptLevel());
}

TEST(SolidSyslogFreeRtosTcpStream, OpenPassesTickTypeSizedOptionLengthToSetsockopt)
{
    openStream();
    LONGS_EQUAL(sizeof(TickType_t), FreeRtosSocketsFake_LastSetsockoptOptionLength());
}

TEST(SolidSyslogFreeRtosTcpStream, OpenReturnsFalseOnConnectFailure)
{
    FreeRtosSocketsFake_SetConnectFails(true);
    CHECK_FALSE(SolidSyslogStream_Open(stream, addr));
}

TEST(SolidSyslogFreeRtosTcpStream, OpenClosesSocketOnConnectFailure)
{
    FreeRtosSocketsFake_SetConnectFails(true);
    openStream();
    CHECK_SOCKET_CLOSED_ONCE();
}

TEST(SolidSyslogFreeRtosTcpStream, OpenIsIdempotent)
{
    openStream();
    CHECK_TRUE(SolidSyslogStream_Open(stream, addr));
    CALLED_FAKE(FreeRtosSocketsFake_Socket, ONCE);
    CALLED_FAKE(FreeRtosSocketsFake_Connect, ONCE);
}

TEST(SolidSyslogFreeRtosTcpStream, SendFailsBeforeOpen)
{
    CHECK_FALSE(SolidSyslogStream_Send(stream, "x", 1));
    CALLED_FAKE(FreeRtosSocketsFake_Send, NEVER);
}

TEST(SolidSyslogFreeRtosTcpStream, SendCallsFreeRtosSendWithSocketBufferAndLength)
{
    openStream();
    SolidSyslogStream_Send(stream, TEST_MESSAGE, TEST_MESSAGE_LEN);
    CALLED_FAKE(FreeRtosSocketsFake_Send, ONCE);
    POINTERS_EQUAL(FreeRtosSocketsFake_LastSocketReturned(), FreeRtosSocketsFake_LastSendSocket());
    POINTERS_EQUAL(TEST_MESSAGE, FreeRtosSocketsFake_LastSendBuffer());
    LONGS_EQUAL(TEST_MESSAGE_LEN, FreeRtosSocketsFake_LastSendLength());
    LONGS_EQUAL(0, FreeRtosSocketsFake_LastSendFlags());
}

TEST(SolidSyslogFreeRtosTcpStream, SendReturnsTrueOnFullWrite)
{
    openStream();
    CHECK_TRUE(SolidSyslogStream_Send(stream, TEST_MESSAGE, TEST_MESSAGE_LEN));
}

TEST(SolidSyslogFreeRtosTcpStream, SendReturnsFalseOnShortWrite)
{
    openStream();
    FreeRtosSocketsFake_SetSendReturn(TEST_SHORT_WRITE_BYTES);
    CHECK_FALSE(SolidSyslogStream_Send(stream, TEST_MESSAGE, TEST_MESSAGE_LEN));
}

TEST(SolidSyslogFreeRtosTcpStream, SendDoesNotRetryAfterShortWrite)
{
    openStream();
    FreeRtosSocketsFake_SetSendReturn(TEST_SHORT_WRITE_BYTES);
    SolidSyslogStream_Send(stream, TEST_MESSAGE, TEST_MESSAGE_LEN);
    CALLED_FAKE(FreeRtosSocketsFake_Send, ONCE);
}

TEST(SolidSyslogFreeRtosTcpStream, SendClosesSocketOnShortWrite)
{
    openStream();
    FreeRtosSocketsFake_SetSendReturn(TEST_SHORT_WRITE_BYTES);
    SolidSyslogStream_Send(stream, TEST_MESSAGE, TEST_MESSAGE_LEN);
    CHECK_SOCKET_CLOSED_ONCE();
}

TEST(SolidSyslogFreeRtosTcpStream, SendReturnsFalseOnError)
{
    openStream();
    FreeRtosSocketsFake_SetSendFails(true);
    CHECK_FALSE(SolidSyslogStream_Send(stream, TEST_MESSAGE, TEST_MESSAGE_LEN));
}

TEST(SolidSyslogFreeRtosTcpStream, SendClosesSocketOnError)
{
    openStream();
    FreeRtosSocketsFake_SetSendFails(true);
    SolidSyslogStream_Send(stream, TEST_MESSAGE, TEST_MESSAGE_LEN);
    CHECK_SOCKET_CLOSED_ONCE();
}

TEST(SolidSyslogFreeRtosTcpStream, ReadReturnsNegativeOneBeforeOpen)
{
    LONGS_EQUAL(-1, readIntoBuffer());
    CALLED_FAKE(FreeRtosSocketsFake_Recv, NEVER);
}

TEST(SolidSyslogFreeRtosTcpStream, ReadCallsFreeRtosRecvWithSocketBufferAndLength)
{
    openStream();
    readIntoBuffer();
    CALLED_FAKE(FreeRtosSocketsFake_Recv, ONCE);
    POINTERS_EQUAL(FreeRtosSocketsFake_LastSocketReturned(), FreeRtosSocketsFake_LastRecvSocket());
    POINTERS_EQUAL(readBuffer, FreeRtosSocketsFake_LastRecvBuffer());
    LONGS_EQUAL(sizeof(readBuffer), FreeRtosSocketsFake_LastRecvLength());
    LONGS_EQUAL(0, FreeRtosSocketsFake_LastRecvFlags());
}

TEST(SolidSyslogFreeRtosTcpStream, ReadReturnsBytesOnSuccess)
{
    openStream();
    FreeRtosSocketsFake_SetRecvReturn(TEST_READ_BYTES);
    LONGS_EQUAL(TEST_READ_BYTES, readIntoBuffer());
}

TEST(SolidSyslogFreeRtosTcpStream, ReadReturnsZeroWhenWouldBlock)
{
    openStream();
    FreeRtosSocketsFake_SetRecvReturn(0);
    LONGS_EQUAL(0, readIntoBuffer());
}

TEST(SolidSyslogFreeRtosTcpStream, ReadLeavesSocketOpenWhenWouldBlock)
{
    openStream();
    FreeRtosSocketsFake_SetRecvReturn(0);
    readIntoBuffer();
    CALLED_FAKE(FreeRtosSocketsFake_Closesocket, NEVER);
}

TEST(SolidSyslogFreeRtosTcpStream, ReadReturnsNegativeOneOnErrorAndClosesSocket)
{
    openStream();
    FreeRtosSocketsFake_SetRecvFails(true);
    LONGS_EQUAL(-1, readIntoBuffer());
    CHECK_SOCKET_CLOSED_ONCE();
}

TEST(SolidSyslogFreeRtosTcpStream, CloseWithoutOpenIsNoOp)
{
    SolidSyslogStream_Close(stream);
    CALLED_FAKE(FreeRtosSocketsFake_Closesocket, NEVER);
}

TEST(SolidSyslogFreeRtosTcpStream, CloseClosesOpenSocket)
{
    openStream();
    SolidSyslogStream_Close(stream);
    CHECK_SOCKET_CLOSED_ONCE();
}

TEST(SolidSyslogFreeRtosTcpStream, CloseIsIdempotent)
{
    openStream();
    SolidSyslogStream_Close(stream);
    SolidSyslogStream_Close(stream);
    CALLED_FAKE(FreeRtosSocketsFake_Closesocket, ONCE);
}

TEST(SolidSyslogFreeRtosTcpStream, DestroyClosesOpenSocket)
{
    openStream();
    SolidSyslogFreeRtosTcpStream_Destroy(stream);
    stream = nullptr;
    CHECK_SOCKET_CLOSED_ONCE();
}

TEST(SolidSyslogFreeRtosTcpStream, DestroyAfterCloseDoesNotCloseAgain)
{
    openStream();
    SolidSyslogStream_Close(stream);
    SolidSyslogFreeRtosTcpStream_Destroy(stream);
    stream = nullptr;
    CALLED_FAKE(FreeRtosSocketsFake_Closesocket, ONCE);
}

// clang-format off
TEST_GROUP(SolidSyslogFreeRtosTcpStreamPool)
{
    // cppcheck-suppress constVariable -- assigned in test bodies; cppcheck does not model CppUTest lifecycle
    struct SolidSyslogStream* pooled[SOLIDSYSLOG_FREE_RTOS_TCP_STREAM_POOL_SIZE] = {};
    struct SolidSyslogStream* overflow                                           = nullptr;

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
                SolidSyslogFreeRtosTcpStream_Destroy(handle);
            }
        }
        // cppcheck-suppress knownConditionTrueFalse -- assigned in test bodies; cppcheck does not model CppUTest lifecycle
        if (overflow != nullptr)
        {
            SolidSyslogFreeRtosTcpStream_Destroy(overflow);
        }
        ConfigLockFake_Uninstall();
        ErrorHandlerFake_Uninstall();
    }

    void FillPool()
    {
        for (auto*& slot : pooled)
        {
            slot = SolidSyslogFreeRtosTcpStream_Create();
        }
    }
};

// clang-format on

TEST(SolidSyslogFreeRtosTcpStreamPool, FillingPoolThenOverflowReturnsDistinctFallback)
{
    FillPool();

    overflow = SolidSyslogFreeRtosTcpStream_Create();

    CHECK_IS_FALLBACK(overflow, pooled);
}

TEST(SolidSyslogFreeRtosTcpStreamPool, ExhaustedCreateReportsError)
{
    ErrorHandlerFake_Install(nullptr);
    FillPool();

    overflow = SolidSyslogFreeRtosTcpStream_Create();

    CALLED_FAKE(ErrorHandlerFake_Handle, ONCE);
    LONGS_EQUAL(SOLIDSYSLOG_SEVERITY_ERROR, ErrorHandlerFake_LastSeverity());
    STRCMP_EQUAL(SOLIDSYSLOG_ERROR_MSG_FREERTOSTCPSTREAM_POOL_EXHAUSTED, ErrorHandlerFake_LastMessage());
}

TEST(SolidSyslogFreeRtosTcpStreamPool, FallbackVtableMethodsAreNoOps)
{
    FillPool();
    overflow = SolidSyslogFreeRtosTcpStream_Create();
    struct SolidSyslogAddress* localAddr = SolidSyslogFreeRtosAddress_Create();
    char buf[8] = {0};
    FreeRtosSocketsFake_Reset();

    /* NullStream's Open/Send/Read return safe values so the Service algorithm
     * does not tear the (non-existent) connection down on the fallback. */
    CHECK_TRUE(SolidSyslogStream_Open(overflow, localAddr));
    CHECK_TRUE(SolidSyslogStream_Send(overflow, buf, sizeof(buf)));
    LONGS_EQUAL(0, SolidSyslogStream_Read(overflow, buf, sizeof(buf)));
    SolidSyslogStream_Close(overflow);
    SolidSyslogFreeRtosAddress_Destroy(localAddr);

    CALLED_FAKE(FreeRtosSocketsFake_Socket, NEVER);
    CALLED_FAKE(FreeRtosSocketsFake_Connect, NEVER);
    CALLED_FAKE(FreeRtosSocketsFake_Send, NEVER);
    CALLED_FAKE(FreeRtosSocketsFake_Recv, NEVER);
    CALLED_FAKE(FreeRtosSocketsFake_Closesocket, NEVER);
}

TEST(SolidSyslogFreeRtosTcpStreamPool, CreateAcquiresAndReleasesConfigLockOnFirstFreeSlot)
{
    ConfigLockFake_Install();

    pooled[0] = SolidSyslogFreeRtosTcpStream_Create();

    CALLED_FAKE(ConfigLockFake_Lock, ONCE);
    CALLED_FAKE(ConfigLockFake_Unlock, ONCE);
}

TEST(SolidSyslogFreeRtosTcpStreamPool, CreateLocksOncePerSlotProbedWhenPoolIsFull)
{
    FillPool();
    ConfigLockFake_Install();

    overflow = SolidSyslogFreeRtosTcpStream_Create();

    LONGS_EQUAL(SOLIDSYSLOG_FREE_RTOS_TCP_STREAM_POOL_SIZE, ConfigLockFake_LockCallCount());
    LONGS_EQUAL(SOLIDSYSLOG_FREE_RTOS_TCP_STREAM_POOL_SIZE, ConfigLockFake_UnlockCallCount());
}

TEST(SolidSyslogFreeRtosTcpStreamPool, DestroyOfPooledHandleLocksOnce)
{
    pooled[0] = SolidSyslogFreeRtosTcpStream_Create();
    ConfigLockFake_Install();

    SolidSyslogFreeRtosTcpStream_Destroy(pooled[0]);
    pooled[0] = nullptr;

    CALLED_FAKE(ConfigLockFake_Lock, ONCE);
    CALLED_FAKE(ConfigLockFake_Unlock, ONCE);
}

TEST(SolidSyslogFreeRtosTcpStreamPool, DestroyOfUnknownHandleDoesNotLock)
{
    ConfigLockFake_Install();
    struct SolidSyslogStream stranger = {};

    SolidSyslogFreeRtosTcpStream_Destroy(&stranger);

    CALLED_FAKE(ConfigLockFake_Lock, NEVER);
    CALLED_FAKE(ConfigLockFake_Unlock, NEVER);
}

TEST(SolidSyslogFreeRtosTcpStreamPool, DestroyOfUnknownHandleReportsWarning)
{
    ErrorHandlerFake_Install(nullptr);
    struct SolidSyslogStream stranger = {};

    SolidSyslogFreeRtosTcpStream_Destroy(&stranger);

    CALLED_FAKE(ErrorHandlerFake_Handle, ONCE);
    LONGS_EQUAL(SOLIDSYSLOG_SEVERITY_WARNING, ErrorHandlerFake_LastSeverity());
    STRCMP_EQUAL(SOLIDSYSLOG_ERROR_MSG_FREERTOSTCPSTREAM_UNKNOWN_DESTROY, ErrorHandlerFake_LastMessage());
}

TEST(SolidSyslogFreeRtosTcpStreamPool, DestroyOfStaleHandleReportsWarning)
{
    pooled[0] = SolidSyslogFreeRtosTcpStream_Create();
    SolidSyslogFreeRtosTcpStream_Destroy(pooled[0]);
    ErrorHandlerFake_Install(nullptr);

    SolidSyslogFreeRtosTcpStream_Destroy(pooled[0]);
    pooled[0] = nullptr;

    CALLED_FAKE(ErrorHandlerFake_Handle, ONCE);
    LONGS_EQUAL(SOLIDSYSLOG_SEVERITY_WARNING, ErrorHandlerFake_LastSeverity());
    STRCMP_EQUAL(SOLIDSYSLOG_ERROR_MSG_FREERTOSTCPSTREAM_UNKNOWN_DESTROY, ErrorHandlerFake_LastMessage());
}
