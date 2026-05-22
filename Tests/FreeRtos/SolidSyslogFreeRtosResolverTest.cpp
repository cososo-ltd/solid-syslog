#include "TestUtils.h"
#include "CppUTest/TestHarness.h"

using namespace CososoTesting; // NOLINT(google-build-using-namespace) -- test-file scope only; brings NEVER/ONCE/TWICE/THRICE into scope for the CALLED_*
    // macros

#include <cstdint>

#include "ConfigLockFake.h"
#include "ErrorHandlerFake.h"
#include "FreeRtosDnsFake.h"
#include "SolidSyslogErrorMessages.h"
#include "SolidSyslogFreeRtosAddress.h"
#include "SolidSyslogFreeRtosAddressPrivate.h"
#include "SolidSyslogFreeRtosResolver.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogResolver.h"
#include "SolidSyslogResolverDefinition.h"
#include "SolidSyslogTransport.h"
#include "SolidSyslogTunables.h"

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

static const char* const TEST_HOST = "10.0.2.2";
static const char* const TEST_ALTERNATE_HOST = "192.168.1.1";
static const uint16_t TEST_PORT = 514;
static const uint16_t TEST_ALTERNATE_PORT = 9999;

// clang-format off
TEST_GROUP(SolidSyslogFreeRtosResolverTest)
{
    struct SolidSyslogResolver* resolver = nullptr;
    struct SolidSyslogAddress*  addr     = nullptr;

    void setup() override
    {
        FreeRtosDnsFake_Reset();
        resolver = SolidSyslogFreeRtosResolver_Create();
        addr     = SolidSyslogFreeRtosAddress_Create();
    }

    void teardown() override
    {
        SolidSyslogFreeRtosAddress_Destroy(addr);
        SolidSyslogFreeRtosResolver_Destroy(resolver);
    }

    bool Resolve(const char* host = TEST_HOST, uint16_t port = TEST_PORT, enum SolidSyslogTransport transport = SOLIDSYSLOG_TRANSPORT_UDP) const
    {
        return SolidSyslogResolver_Resolve(resolver, transport, host, port, addr);
    }

    // NOLINTNEXTLINE(modernize-use-nodiscard) -- used through accessor syntax in tests
    const struct freertos_sockaddr* Result() const
    {
        return SolidSyslogFreeRtosAddress_AsConstFreertosSockaddr(addr);
    }
};

// clang-format on

TEST(SolidSyslogFreeRtosResolverTest, CreateSucceeds)
{
    CHECK(resolver != nullptr);
}

TEST(SolidSyslogFreeRtosResolverTest, ResolveReturnsTrueOnSuccess)
{
    CHECK_TRUE(Resolve());
}

TEST(SolidSyslogFreeRtosResolverTest, ResolvePopulatesAddressFamily)
{
    Resolve();
    LONGS_EQUAL(FREERTOS_AF_INET, Result()->sin_family);
}

TEST(SolidSyslogFreeRtosResolverTest, ResolvePopulatesIpv4FromGetAddrInfoResult)
{
    Resolve(TEST_HOST);
    LONGS_EQUAL(FreeRTOS_inet_addr_quick(10, 0, 2, 2), Result()->sin_address.ulIP_IPv4);
}

TEST(SolidSyslogFreeRtosResolverTest, ResolvePopulatesPortFromArgInNetworkOrder)
{
    Resolve(TEST_HOST, TEST_ALTERNATE_PORT);
    LONGS_EQUAL(FreeRTOS_htons(TEST_ALTERNATE_PORT), Result()->sin_port);
}

TEST(SolidSyslogFreeRtosResolverTest, ResolvePassesHostStringToGetAddrInfo)
{
    Resolve(TEST_ALTERNATE_HOST);
    CALLED_FAKE(FreeRtosDnsFake_GetAddrInfo, ONCE);
    STRCMP_EQUAL(TEST_ALTERNATE_HOST, FreeRtosDnsFake_LastGetAddrInfoHostname());
}

TEST(SolidSyslogFreeRtosResolverTest, UdpTransportPassesDatagramSocktype)
{
    Resolve(TEST_HOST, TEST_PORT, SOLIDSYSLOG_TRANSPORT_UDP);
    LONGS_EQUAL(FREERTOS_SOCK_DGRAM, FreeRtosDnsFake_LastGetAddrInfoSocktype());
}

TEST(SolidSyslogFreeRtosResolverTest, TcpTransportPassesStreamSocktype)
{
    Resolve(TEST_HOST, TEST_PORT, SOLIDSYSLOG_TRANSPORT_TCP);
    LONGS_EQUAL(FREERTOS_SOCK_STREAM, FreeRtosDnsFake_LastGetAddrInfoSocktype());
}

TEST(SolidSyslogFreeRtosResolverTest, ResolveReturnsFalseWhenGetAddrInfoFails)
{
    FreeRtosDnsFake_SetGetAddrInfoFails(true);
    CHECK_FALSE(Resolve());
}

TEST(SolidSyslogFreeRtosResolverTest, DoesNotFreeAddrInfoWhenGetAddrInfoFails)
{
    FreeRtosDnsFake_SetGetAddrInfoFails(true);
    Resolve();
    CALLED_FAKE(FreeRtosDnsFake_FreeAddrInfo, NEVER);
}

TEST(SolidSyslogFreeRtosResolverTest, FreesAddrInfoOnSuccess)
{
    Resolve();
    CALLED_FAKE(FreeRtosDnsFake_FreeAddrInfo, ONCE);
}

// clang-format off
TEST_GROUP(SolidSyslogFreeRtosResolverPoolTest)
{
    // cppcheck-suppress constVariable -- assigned in test bodies; cppcheck does not model CppUTest lifecycle
    struct SolidSyslogResolver* pooled[SOLIDSYSLOG_FREE_RTOS_RESOLVER_POOL_SIZE] = {};
    struct SolidSyslogResolver* overflow                                         = nullptr;

    void teardown() override
    {
        for (auto* handle : pooled)
        {
            if (handle != nullptr)
            {
                SolidSyslogFreeRtosResolver_Destroy(handle);
            }
        }
        // cppcheck-suppress knownConditionTrueFalse -- assigned in test bodies; cppcheck does not model CppUTest lifecycle
        if (overflow != nullptr)
        {
            SolidSyslogFreeRtosResolver_Destroy(overflow);
        }
        ConfigLockFake_Uninstall();
        ErrorHandlerFake_Uninstall();
    }

    void FillPool()
    {
        for (auto*& slot : pooled)
        {
            slot = SolidSyslogFreeRtosResolver_Create();
        }
    }
};

// clang-format on

TEST(SolidSyslogFreeRtosResolverPoolTest, FillingPoolThenOverflowReturnsDistinctFallback)
{
    FillPool();

    overflow = SolidSyslogFreeRtosResolver_Create();

    CHECK_IS_FALLBACK(overflow, pooled);
}

TEST(SolidSyslogFreeRtosResolverPoolTest, ExhaustedCreateReportsError)
{
    ErrorHandlerFake_Install(nullptr);
    FillPool();

    overflow = SolidSyslogFreeRtosResolver_Create();

    CALLED_FAKE(ErrorHandlerFake_Handle, ONCE);
    LONGS_EQUAL(SOLIDSYSLOG_SEVERITY_ERROR, ErrorHandlerFake_LastSeverity());
    STRCMP_EQUAL(SOLIDSYSLOG_ERROR_MSG_FREERTOSRESOLVER_POOL_EXHAUSTED, ErrorHandlerFake_LastMessage());
}

TEST(SolidSyslogFreeRtosResolverPoolTest, FallbackResolveReturnsFalse)
{
    FillPool();
    overflow = SolidSyslogFreeRtosResolver_Create();
    struct SolidSyslogAddress* fallbackResult = SolidSyslogFreeRtosAddress_Create();

    CHECK_FALSE(SolidSyslogResolver_Resolve(overflow, SOLIDSYSLOG_TRANSPORT_UDP, TEST_HOST, TEST_PORT, fallbackResult));

    SolidSyslogFreeRtosAddress_Destroy(fallbackResult);
}

TEST(SolidSyslogFreeRtosResolverPoolTest, CreateAcquiresAndReleasesConfigLockOnFirstFreeSlot)
{
    ConfigLockFake_Install();

    pooled[0] = SolidSyslogFreeRtosResolver_Create();

    CALLED_FAKE(ConfigLockFake_Lock, ONCE);
    CALLED_FAKE(ConfigLockFake_Unlock, ONCE);
}

TEST(SolidSyslogFreeRtosResolverPoolTest, CreateLocksOncePerSlotProbedWhenPoolIsFull)
{
    FillPool();
    ConfigLockFake_Install();

    overflow = SolidSyslogFreeRtosResolver_Create();

    LONGS_EQUAL(SOLIDSYSLOG_FREE_RTOS_RESOLVER_POOL_SIZE, ConfigLockFake_LockCallCount());
    LONGS_EQUAL(SOLIDSYSLOG_FREE_RTOS_RESOLVER_POOL_SIZE, ConfigLockFake_UnlockCallCount());
}

TEST(SolidSyslogFreeRtosResolverPoolTest, DestroyOfPooledHandleLocksOnce)
{
    pooled[0] = SolidSyslogFreeRtosResolver_Create();
    ConfigLockFake_Install();

    SolidSyslogFreeRtosResolver_Destroy(pooled[0]);
    pooled[0] = nullptr;

    CALLED_FAKE(ConfigLockFake_Lock, ONCE);
    CALLED_FAKE(ConfigLockFake_Unlock, ONCE);
}

TEST(SolidSyslogFreeRtosResolverPoolTest, DestroyOfUnknownHandleDoesNotLock)
{
    ConfigLockFake_Install();
    struct SolidSyslogResolver stranger = {};

    SolidSyslogFreeRtosResolver_Destroy(&stranger);

    CALLED_FAKE(ConfigLockFake_Lock, NEVER);
    CALLED_FAKE(ConfigLockFake_Unlock, NEVER);
}

TEST(SolidSyslogFreeRtosResolverPoolTest, DestroyOfUnknownHandleReportsWarning)
{
    ErrorHandlerFake_Install(nullptr);
    struct SolidSyslogResolver stranger = {};

    SolidSyslogFreeRtosResolver_Destroy(&stranger);

    CALLED_FAKE(ErrorHandlerFake_Handle, ONCE);
    LONGS_EQUAL(SOLIDSYSLOG_SEVERITY_WARNING, ErrorHandlerFake_LastSeverity());
    STRCMP_EQUAL(SOLIDSYSLOG_ERROR_MSG_FREERTOSRESOLVER_UNKNOWN_DESTROY, ErrorHandlerFake_LastMessage());
}

TEST(SolidSyslogFreeRtosResolverPoolTest, DestroyOfStaleHandleReportsWarning)
{
    pooled[0] = SolidSyslogFreeRtosResolver_Create();
    SolidSyslogFreeRtosResolver_Destroy(pooled[0]);
    ErrorHandlerFake_Install(nullptr);

    SolidSyslogFreeRtosResolver_Destroy(pooled[0]);
    pooled[0] = nullptr;

    CALLED_FAKE(ErrorHandlerFake_Handle, ONCE);
    LONGS_EQUAL(SOLIDSYSLOG_SEVERITY_WARNING, ErrorHandlerFake_LastSeverity());
    STRCMP_EQUAL(SOLIDSYSLOG_ERROR_MSG_FREERTOSRESOLVER_UNKNOWN_DESTROY, ErrorHandlerFake_LastMessage());
}
