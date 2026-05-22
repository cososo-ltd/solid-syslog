#include "TestUtils.h"
#include "CppUTest/TestHarness.h"

using namespace CososoTesting; // NOLINT(google-build-using-namespace) -- test-file scope only; brings NEVER/ONCE/TWICE/THRICE into scope for the CALLED_*
    // macros

#include "ConfigLockFake.h"
#include "ErrorHandlerFake.h"
#include "SolidSyslogFreeRtosAddress.h"
#include "SolidSyslogFreeRtosAddressPrivate.h"
#include "SolidSyslogErrorMessages.h"
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

static const uint8_t TEST_OCTETS[4] = {10, 0, 2, 2};
static const uint16_t TEST_PORT = 514;
static const uint16_t TEST_ALTERNATE_PORT = 9999;
static const char* IGNORED_HOST = "ignored.example.com";

// clang-format off
TEST_GROUP(SolidSyslogFreeRtosResolverTest)
{
    struct SolidSyslogResolver* resolver  = nullptr;
    struct SolidSyslogAddress*  addr      = nullptr;

    void setup() override
    {
        resolver = SolidSyslogFreeRtosResolver_Create(TEST_OCTETS);
        addr     = SolidSyslogFreeRtosAddress_Create();
    }

    void teardown() override
    {
        SolidSyslogFreeRtosAddress_Destroy(addr);
        SolidSyslogFreeRtosResolver_Destroy(resolver);
    }

    bool Resolve(const char* host = IGNORED_HOST, uint16_t port = TEST_PORT, enum SolidSyslogTransport transport = SOLIDSYSLOG_TRANSPORT_UDP) const
    {
        return SolidSyslogResolver_Resolve(resolver, transport, host, port, addr);
    }

    void RecreateResolverWith(const uint8_t octets[4])
    {
        SolidSyslogFreeRtosResolver_Destroy(resolver);
        resolver = SolidSyslogFreeRtosResolver_Create(octets);
    }

    // NOLINTNEXTLINE(modernize-use-nodiscard) -- used through accessor syntax in tests
    const struct freertos_sockaddr* Result() const
    {
        return SolidSyslogFreeRtosAddress_AsConstFreertosSockaddr(addr);
    }
};

// clang-format on

TEST(SolidSyslogFreeRtosResolverTest, CreateReturnsNonNullResolver)
{
    CHECK(resolver != nullptr);
}

TEST(SolidSyslogFreeRtosResolverTest, ResolveReturnsTrue)
{
    CHECK_TRUE(Resolve());
}

TEST(SolidSyslogFreeRtosResolverTest, ResolveSetsSinFamilyToFreeRtosAfInet)
{
    Resolve();
    LONGS_EQUAL(FREERTOS_AF_INET, Result()->sin_family);
}

TEST(SolidSyslogFreeRtosResolverTest, ResolveWritesIpv4FromCreateOctets)
{
    Resolve();
    LONGS_EQUAL(FreeRTOS_inet_addr_quick(10, 0, 2, 2), Result()->sin_address.ulIP_IPv4);
}

TEST(SolidSyslogFreeRtosResolverTest, ResolveWritesPortFromArgInNetworkOrder)
{
    Resolve(IGNORED_HOST, TEST_ALTERNATE_PORT);
    LONGS_EQUAL(FreeRTOS_htons(TEST_ALTERNATE_PORT), Result()->sin_port);
}

TEST(SolidSyslogFreeRtosResolverTest, ResolveProducesSameIpv4ForAnyHostString)
{
    Resolve("first.Host");
    uint32_t firstIpv4 = Result()->sin_address.ulIP_IPv4;

    *SolidSyslogFreeRtosAddress_AsFreertosSockaddr(addr) = {};
    Resolve("totally.different.second.host");

    LONGS_EQUAL(firstIpv4, Result()->sin_address.ulIP_IPv4);
}

TEST(SolidSyslogFreeRtosResolverTest, ResolveProducesSameIpv4ForUdpAndTcpTransport)
{
    Resolve(IGNORED_HOST, TEST_PORT, SOLIDSYSLOG_TRANSPORT_UDP);
    uint32_t udpIpv4 = Result()->sin_address.ulIP_IPv4;

    *SolidSyslogFreeRtosAddress_AsFreertosSockaddr(addr) = {};
    Resolve(IGNORED_HOST, TEST_PORT, SOLIDSYSLOG_TRANSPORT_TCP);

    LONGS_EQUAL(udpIpv4, Result()->sin_address.ulIP_IPv4);
}

TEST(SolidSyslogFreeRtosResolverTest, ResolveWritesAllZeroOctets)
{
    static const uint8_t ZERO_OCTETS[4] = {0, 0, 0, 0};
    RecreateResolverWith(ZERO_OCTETS);
    Resolve();
    LONGS_EQUAL(FreeRTOS_inet_addr_quick(0, 0, 0, 0), Result()->sin_address.ulIP_IPv4);
}

TEST(SolidSyslogFreeRtosResolverTest, ResolveWritesAllOnesOctets)
{
    static const uint8_t MAX_OCTETS[4] = {255, 255, 255, 255};
    RecreateResolverWith(MAX_OCTETS);
    Resolve();
    LONGS_EQUAL(FreeRTOS_inet_addr_quick(255, 255, 255, 255), Result()->sin_address.ulIP_IPv4);
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
            slot = SolidSyslogFreeRtosResolver_Create(TEST_OCTETS);
        }
    }
};

// clang-format on

TEST(SolidSyslogFreeRtosResolverPoolTest, FillingPoolThenOverflowReturnsDistinctFallback)
{
    FillPool();

    overflow = SolidSyslogFreeRtosResolver_Create(TEST_OCTETS);

    CHECK_IS_FALLBACK(overflow, pooled);
}

TEST(SolidSyslogFreeRtosResolverPoolTest, ExhaustedCreateReportsError)
{
    ErrorHandlerFake_Install(nullptr);
    FillPool();

    overflow = SolidSyslogFreeRtosResolver_Create(TEST_OCTETS);

    CALLED_FAKE(ErrorHandlerFake_Handle, ONCE);
    LONGS_EQUAL(SOLIDSYSLOG_SEVERITY_ERROR, ErrorHandlerFake_LastSeverity());
    STRCMP_EQUAL(SOLIDSYSLOG_ERROR_MSG_FREERTOSRESOLVER_POOL_EXHAUSTED, ErrorHandlerFake_LastMessage());
}

TEST(SolidSyslogFreeRtosResolverPoolTest, FallbackResolveReturnsFalse)
{
    FillPool();
    overflow = SolidSyslogFreeRtosResolver_Create(TEST_OCTETS);
    struct SolidSyslogAddress* addr = SolidSyslogFreeRtosAddress_Create();

    CHECK_FALSE(SolidSyslogResolver_Resolve(overflow, SOLIDSYSLOG_TRANSPORT_UDP, "host", 514, addr));

    SolidSyslogFreeRtosAddress_Destroy(addr);
}

TEST(SolidSyslogFreeRtosResolverPoolTest, CreateAcquiresAndReleasesConfigLockOnFirstFreeSlot)
{
    ConfigLockFake_Install();

    pooled[0] = SolidSyslogFreeRtosResolver_Create(TEST_OCTETS);

    CALLED_FAKE(ConfigLockFake_Lock, ONCE);
    CALLED_FAKE(ConfigLockFake_Unlock, ONCE);
}

TEST(SolidSyslogFreeRtosResolverPoolTest, CreateLocksOncePerSlotProbedWhenPoolIsFull)
{
    FillPool();
    ConfigLockFake_Install();

    overflow = SolidSyslogFreeRtosResolver_Create(TEST_OCTETS);

    LONGS_EQUAL(SOLIDSYSLOG_FREE_RTOS_RESOLVER_POOL_SIZE, ConfigLockFake_LockCallCount());
    LONGS_EQUAL(SOLIDSYSLOG_FREE_RTOS_RESOLVER_POOL_SIZE, ConfigLockFake_UnlockCallCount());
}

TEST(SolidSyslogFreeRtosResolverPoolTest, DestroyOfPooledHandleLocksOnce)
{
    pooled[0] = SolidSyslogFreeRtosResolver_Create(TEST_OCTETS);
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
    pooled[0] = SolidSyslogFreeRtosResolver_Create(TEST_OCTETS);
    SolidSyslogFreeRtosResolver_Destroy(pooled[0]);
    ErrorHandlerFake_Install(nullptr);

    SolidSyslogFreeRtosResolver_Destroy(pooled[0]);
    pooled[0] = nullptr;

    CALLED_FAKE(ErrorHandlerFake_Handle, ONCE);
    LONGS_EQUAL(SOLIDSYSLOG_SEVERITY_WARNING, ErrorHandlerFake_LastSeverity());
    STRCMP_EQUAL(SOLIDSYSLOG_ERROR_MSG_FREERTOSRESOLVER_UNKNOWN_DESTROY, ErrorHandlerFake_LastMessage());
}
