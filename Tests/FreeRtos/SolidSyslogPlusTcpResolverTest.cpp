#include "TestUtils.h"
#include "CppUTest/TestHarness.h"

using namespace CososoTesting; // NOLINT(google-build-using-namespace) -- test-file scope only; brings NEVER/ONCE/TWICE/THRICE into scope for the CALLED_*
    // macros

#include <cstdint>

#include "ConfigLockFake.h"
#include "ErrorHandlerFake.h"
#include "FreeRtosDnsFake.h"
#include "SolidSyslogPlusTcpAddress.h"
#include "SolidSyslogPlusTcpAddressPrivate.h"
#include "SolidSyslogPlusTcpResolver.h"
#include "SolidSyslogPlusTcpResolverErrors.h"
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
TEST_GROUP(SolidSyslogPlusTcpResolverTest)
{
    struct SolidSyslogResolver* resolver = nullptr;
    struct SolidSyslogAddress*  addr     = nullptr;

    void setup() override
    {
        FreeRtosDnsFake_Reset();
        resolver = SolidSyslogPlusTcpResolver_Create();
        addr     = SolidSyslogPlusTcpAddress_Create();
    }

    void teardown() override
    {
        SolidSyslogPlusTcpAddress_Destroy(addr);
        SolidSyslogPlusTcpResolver_Destroy(resolver);
    }

    bool Resolve(const char* host = TEST_HOST, uint16_t port = TEST_PORT, enum SolidSyslogTransport transport = SOLIDSYSLOG_TRANSPORT_UDP) const
    {
        return SolidSyslogResolver_Resolve(resolver, transport, host, port, addr);
    }

    // NOLINTNEXTLINE(modernize-use-nodiscard) -- used through accessor syntax in tests
    const struct freertos_sockaddr* Result() const
    {
        return SolidSyslogPlusTcpAddress_AsConstFreertosSockaddr(addr);
    }
};

// clang-format on

TEST(SolidSyslogPlusTcpResolverTest, CreateSucceeds)
{
    CHECK(resolver != nullptr);
}

TEST(SolidSyslogPlusTcpResolverTest, ResolveReturnsTrueOnSuccess)
{
    CHECK_TRUE(Resolve());
}

TEST(SolidSyslogPlusTcpResolverTest, ResolvePopulatesAddressFamily)
{
    Resolve();
    LONGS_EQUAL(FREERTOS_AF_INET, Result()->sin_family);
}

TEST(SolidSyslogPlusTcpResolverTest, ResolvePopulatesIpv4FromGetAddrInfoResult)
{
    Resolve(TEST_HOST);
    LONGS_EQUAL(FreeRTOS_inet_addr_quick(10, 0, 2, 2), Result()->sin_address.ulIP_IPv4);
}

TEST(SolidSyslogPlusTcpResolverTest, ResolvePopulatesPortFromArgInNetworkOrder)
{
    Resolve(TEST_HOST, TEST_ALTERNATE_PORT);
    LONGS_EQUAL(FreeRTOS_htons(TEST_ALTERNATE_PORT), Result()->sin_port);
}

TEST(SolidSyslogPlusTcpResolverTest, ResolvePassesHostStringToGetAddrInfo)
{
    Resolve(TEST_ALTERNATE_HOST);
    CALLED_FAKE(FreeRtosDnsFake_GetAddrInfo, ONCE);
    STRCMP_EQUAL(TEST_ALTERNATE_HOST, FreeRtosDnsFake_LastGetAddrInfoHostname());
}

TEST(SolidSyslogPlusTcpResolverTest, UdpTransportPassesDatagramSocktype)
{
    Resolve(TEST_HOST, TEST_PORT, SOLIDSYSLOG_TRANSPORT_UDP);
    LONGS_EQUAL(FREERTOS_SOCK_DGRAM, FreeRtosDnsFake_LastGetAddrInfoSocktype());
}

TEST(SolidSyslogPlusTcpResolverTest, TcpTransportPassesStreamSocktype)
{
    Resolve(TEST_HOST, TEST_PORT, SOLIDSYSLOG_TRANSPORT_TCP);
    LONGS_EQUAL(FREERTOS_SOCK_STREAM, FreeRtosDnsFake_LastGetAddrInfoSocktype());
}

TEST(SolidSyslogPlusTcpResolverTest, ResolveReturnsFalseWhenGetAddrInfoFails)
{
    FreeRtosDnsFake_SetGetAddrInfoFails(true);
    CHECK_FALSE(Resolve());
}

TEST(SolidSyslogPlusTcpResolverTest, DoesNotFreeAddrInfoWhenGetAddrInfoFails)
{
    FreeRtosDnsFake_SetGetAddrInfoFails(true);
    Resolve();
    CALLED_FAKE(FreeRtosDnsFake_FreeAddrInfo, NEVER);
}

TEST(SolidSyslogPlusTcpResolverTest, FreesAddrInfoOnSuccess)
{
    Resolve();
    CALLED_FAKE(FreeRtosDnsFake_FreeAddrInfo, ONCE);
}

// clang-format off
TEST_GROUP(SolidSyslogPlusTcpResolverPoolTest)
{
    // cppcheck-suppress constVariable -- assigned in test bodies; cppcheck does not model CppUTest lifecycle
    struct SolidSyslogResolver* pooled[SOLIDSYSLOG_PLUS_TCP_RESOLVER_POOL_SIZE] = {};
    struct SolidSyslogResolver* overflow                                         = nullptr;

    void teardown() override
    {
        for (auto* handle : pooled)
        {
            if (handle != nullptr)
            {
                SolidSyslogPlusTcpResolver_Destroy(handle);
            }
        }
        // cppcheck-suppress knownConditionTrueFalse -- assigned in test bodies; cppcheck does not model CppUTest lifecycle
        if (overflow != nullptr)
        {
            SolidSyslogPlusTcpResolver_Destroy(overflow);
        }
        ConfigLockFake_Uninstall();
    }

    void FillPool()
    {
        for (auto*& slot : pooled)
        {
            slot = SolidSyslogPlusTcpResolver_Create();
        }
    }
};

// clang-format on

TEST(SolidSyslogPlusTcpResolverPoolTest, FillingPoolThenOverflowReturnsDistinctFallback)
{
    FillPool();

    overflow = SolidSyslogPlusTcpResolver_Create();

    CHECK_IS_FALLBACK(overflow, pooled);
}

TEST(SolidSyslogPlusTcpResolverPoolTest, ExhaustedCreateReportsError)
{
    ErrorHandlerFake_Install(nullptr);
    FillPool();

    overflow = SolidSyslogPlusTcpResolver_Create();

    CALLED_FAKE(ErrorHandlerFake_Handle, ONCE);
    LONGS_EQUAL(SOLIDSYSLOG_SEVERITY_ERROR, ErrorHandlerFake_LastSeverity());
    POINTERS_EQUAL(&PlusTcpResolverErrorSource, ErrorHandlerFake_LastSource());
    UNSIGNED_LONGS_EQUAL(PLUSTCPRESOLVER_ERROR_POOL_EXHAUSTED, ErrorHandlerFake_LastCode());
}

TEST(SolidSyslogPlusTcpResolverPoolTest, FallbackResolveReturnsFalse)
{
    FillPool();
    overflow = SolidSyslogPlusTcpResolver_Create();
    struct SolidSyslogAddress* fallbackResult = SolidSyslogPlusTcpAddress_Create();

    CHECK_FALSE(SolidSyslogResolver_Resolve(overflow, SOLIDSYSLOG_TRANSPORT_UDP, TEST_HOST, TEST_PORT, fallbackResult));

    SolidSyslogPlusTcpAddress_Destroy(fallbackResult);
}

TEST(SolidSyslogPlusTcpResolverPoolTest, CreateAcquiresAndReleasesConfigLockOnFirstFreeSlot)
{
    ConfigLockFake_Install();

    pooled[0] = SolidSyslogPlusTcpResolver_Create();

    CALLED_FAKE(ConfigLockFake_Lock, ONCE);
    CALLED_FAKE(ConfigLockFake_Unlock, ONCE);
}

TEST(SolidSyslogPlusTcpResolverPoolTest, CreateLocksOncePerSlotProbedWhenPoolIsFull)
{
    FillPool();
    ConfigLockFake_Install();

    overflow = SolidSyslogPlusTcpResolver_Create();

    LONGS_EQUAL(SOLIDSYSLOG_PLUS_TCP_RESOLVER_POOL_SIZE, ConfigLockFake_LockCallCount());
    LONGS_EQUAL(SOLIDSYSLOG_PLUS_TCP_RESOLVER_POOL_SIZE, ConfigLockFake_UnlockCallCount());
}

TEST(SolidSyslogPlusTcpResolverPoolTest, DestroyOfPooledHandleLocksOnce)
{
    pooled[0] = SolidSyslogPlusTcpResolver_Create();
    ConfigLockFake_Install();

    SolidSyslogPlusTcpResolver_Destroy(pooled[0]);
    pooled[0] = nullptr;

    CALLED_FAKE(ConfigLockFake_Lock, ONCE);
    CALLED_FAKE(ConfigLockFake_Unlock, ONCE);
}

TEST(SolidSyslogPlusTcpResolverPoolTest, DestroyOfUnknownHandleDoesNotLock)
{
    ConfigLockFake_Install();
    struct SolidSyslogResolver stranger = {};

    SolidSyslogPlusTcpResolver_Destroy(&stranger);

    CALLED_FAKE(ConfigLockFake_Lock, NEVER);
    CALLED_FAKE(ConfigLockFake_Unlock, NEVER);
}

TEST(SolidSyslogPlusTcpResolverPoolTest, DestroyOfUnknownHandleReportsWarning)
{
    ErrorHandlerFake_Install(nullptr);
    struct SolidSyslogResolver stranger = {};

    SolidSyslogPlusTcpResolver_Destroy(&stranger);

    CALLED_FAKE(ErrorHandlerFake_Handle, ONCE);
    LONGS_EQUAL(SOLIDSYSLOG_SEVERITY_WARNING, ErrorHandlerFake_LastSeverity());
    POINTERS_EQUAL(&PlusTcpResolverErrorSource, ErrorHandlerFake_LastSource());
    UNSIGNED_LONGS_EQUAL(PLUSTCPRESOLVER_ERROR_UNKNOWN_DESTROY, ErrorHandlerFake_LastCode());
}

TEST(SolidSyslogPlusTcpResolverPoolTest, DestroyOfStaleHandleReportsWarning)
{
    pooled[0] = SolidSyslogPlusTcpResolver_Create();
    SolidSyslogPlusTcpResolver_Destroy(pooled[0]);
    ErrorHandlerFake_Install(nullptr);

    SolidSyslogPlusTcpResolver_Destroy(pooled[0]);
    pooled[0] = nullptr;

    CALLED_FAKE(ErrorHandlerFake_Handle, ONCE);
    LONGS_EQUAL(SOLIDSYSLOG_SEVERITY_WARNING, ErrorHandlerFake_LastSeverity());
    POINTERS_EQUAL(&PlusTcpResolverErrorSource, ErrorHandlerFake_LastSource());
    UNSIGNED_LONGS_EQUAL(PLUSTCPRESOLVER_ERROR_UNKNOWN_DESTROY, ErrorHandlerFake_LastCode());
}
