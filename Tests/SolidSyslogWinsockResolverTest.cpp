#include "TestUtils.h"
#include "CppUTest/TestHarness.h"

using namespace CososoTesting; // NOLINT(google-build-using-namespace) -- test-file scope only; brings NEVER/ONCE/TWICE/THRICE into scope for the CALLED_*
    // macros
#include "ConfigLockFake.h"
#include "ErrorHandlerFake.h"
#include "SolidSyslogWinsockAddress.h"
#include "SolidSyslogWinsockAddressPrivate.h"
#include "SolidSyslogErrorMessages.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogResolver.h"
#include "SolidSyslogResolverDefinition.h"
#include "SolidSyslogTunables.h"
#include "SolidSyslogWinsockResolver.h"
#include "SolidSyslogWinsockResolverInternal.h"
#include "WinsockFake.h"
#include <cstdint>
#include <winsock2.h>
#include <ws2tcpip.h>

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

// clang-format off
static const char* const TEST_HOST           = "127.0.0.1";
static const uint16_t    TEST_PORT           = 514;
static const char* const TEST_ALTERNATE_HOST = "192.168.1.1";
static const uint16_t    TEST_ALTERNATE_PORT = 9999;
// clang-format on

// clang-format off
TEST_GROUP(SolidSyslogWinsockResolver)
{
    struct SolidSyslogResolver* resolver = nullptr;
    struct SolidSyslogAddress*  result   = nullptr;

    void setup() override
    {
        WinsockFake_Reset();
        UT_PTR_SET(Winsock_getaddrinfo, WinsockFake_getaddrinfo);
        UT_PTR_SET(Winsock_freeaddrinfo, WinsockFake_freeaddrinfo);
        resolver = SolidSyslogWinsockResolver_Create();
        result   = SolidSyslogWinsockAddress_Create();
    }

    void teardown() override
    {
        SolidSyslogWinsockAddress_Destroy(result);
        SolidSyslogWinsockResolver_Destroy(resolver);
    }

    bool Resolve(const char* host, uint16_t port, enum SolidSyslogTransport transport = SOLIDSYSLOG_TRANSPORT_UDP)
    {
        return SolidSyslogResolver_Resolve(resolver, transport, host, port, result);
    }

    // NOLINTNEXTLINE(modernize-use-nodiscard) -- used through accessor syntax in tests
    const struct sockaddr_in* Result() const
    {
        return SolidSyslogWinsockAddress_AsConstSockaddrIn(result);
    }
};

// clang-format on

TEST(SolidSyslogWinsockResolver, CreateDestroyWorksWithoutCrashing)
{
}

TEST(SolidSyslogWinsockResolver, ReturnsTrueOnSuccess)
{
    CHECK_TRUE(Resolve(TEST_HOST, TEST_PORT));
}

TEST(SolidSyslogWinsockResolver, PopulatesAddressFamily)
{
    Resolve(TEST_HOST, TEST_PORT);
    LONGS_EQUAL(AF_INET, Result()->sin_family);
}

TEST(SolidSyslogWinsockResolver, PopulatesResolvedAddressFromHostArgument)
{
    Resolve(TEST_HOST, TEST_PORT);
    char addrString[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &Result()->sin_addr, addrString, sizeof(addrString));
    STRCMP_EQUAL(TEST_HOST, addrString);
}

TEST(SolidSyslogWinsockResolver, PopulatesPortFromPortArgument)
{
    Resolve(TEST_HOST, TEST_ALTERNATE_PORT);
    LONGS_EQUAL(TEST_ALTERNATE_PORT, ntohs(Result()->sin_port));
}

TEST(SolidSyslogWinsockResolver, GetAddrInfoCalledWithHostArgument)
{
    Resolve(TEST_ALTERNATE_HOST, TEST_PORT);
    CALLED_FAKE(WinsockFake_GetAddrInfo, ONCE);
    STRCMP_EQUAL(TEST_ALTERNATE_HOST, WinsockFake_LastGetAddrInfoHostname());
}

TEST(SolidSyslogWinsockResolver, UdpTransportPassesDatagramSocktype)
{
    Resolve(TEST_HOST, TEST_PORT, SOLIDSYSLOG_TRANSPORT_UDP);
    LONGS_EQUAL(SOCK_DGRAM, WinsockFake_LastGetAddrInfoSocktype());
}

TEST(SolidSyslogWinsockResolver, TcpTransportPassesStreamSocktype)
{
    Resolve(TEST_HOST, TEST_PORT, SOLIDSYSLOG_TRANSPORT_TCP);
    LONGS_EQUAL(SOCK_STREAM, WinsockFake_LastGetAddrInfoSocktype());
}

TEST(SolidSyslogWinsockResolver, ReturnsFalseWhenGetAddrInfoFails)
{
    WinsockFake_SetGetAddrInfoFails(true);
    CHECK_FALSE(Resolve(TEST_HOST, TEST_PORT));
}

TEST(SolidSyslogWinsockResolver, DoesNotFreeAddrInfoWhenGetAddrInfoFails)
{
    WinsockFake_SetGetAddrInfoFails(true);
    Resolve(TEST_HOST, TEST_PORT);
    CALLED_FAKE(WinsockFake_FreeAddrInfo, NEVER);
}

TEST(SolidSyslogWinsockResolver, FreesAddrInfoOnSuccess)
{
    Resolve(TEST_HOST, TEST_PORT);
    CALLED_FAKE(WinsockFake_FreeAddrInfo, ONCE);
}

// clang-format off
TEST_GROUP(SolidSyslogWinsockResolverPool)
{
    // cppcheck-suppress constVariable -- assigned in test bodies; cppcheck does not model CppUTest lifecycle
    struct SolidSyslogResolver* pooled[SOLIDSYSLOG_WINSOCK_RESOLVER_POOL_SIZE] = {};
    struct SolidSyslogResolver* overflow                                       = nullptr;

    void teardown() override
    {
        for (auto* handle : pooled)
        {
            if (handle != nullptr)
            {
                SolidSyslogWinsockResolver_Destroy(handle);
            }
        }
        // cppcheck-suppress knownConditionTrueFalse -- assigned in test bodies; cppcheck does not model CppUTest lifecycle
        if (overflow != nullptr)
        {
            SolidSyslogWinsockResolver_Destroy(overflow);
        }
        ConfigLockFake_Uninstall();
        ErrorHandlerFake_Uninstall();
    }

    void FillPool()
    {
        for (auto*& slot : pooled)
        {
            slot = SolidSyslogWinsockResolver_Create();
        }
    }
};

// clang-format on

TEST(SolidSyslogWinsockResolverPool, FillingPoolThenOverflowReturnsDistinctFallback)
{
    FillPool();

    overflow = SolidSyslogWinsockResolver_Create();

    CHECK_IS_FALLBACK(overflow, pooled);
}

TEST(SolidSyslogWinsockResolverPool, ExhaustedCreateReportsError)
{
    ErrorHandlerFake_Install(nullptr);
    FillPool();

    overflow = SolidSyslogWinsockResolver_Create();

    CALLED_FAKE(ErrorHandlerFake_Handle, ONCE);
    LONGS_EQUAL(SOLIDSYSLOG_SEVERITY_ERROR, ErrorHandlerFake_LastSeverity());
    STRCMP_EQUAL(SOLIDSYSLOG_ERROR_MSG_WINSOCKRESOLVER_POOL_EXHAUSTED, ErrorHandlerFake_LastMessage());
}

TEST(SolidSyslogWinsockResolverPool, FallbackResolveIsNoOp)
{
    FillPool();
    overflow = SolidSyslogWinsockResolver_Create();
    struct SolidSyslogAddress* address = SolidSyslogWinsockAddress_Create();

    CHECK_FALSE(SolidSyslogResolver_Resolve(overflow, SOLIDSYSLOG_TRANSPORT_UDP, "h", 1, address));

    SolidSyslogWinsockAddress_Destroy(address);
}

TEST(SolidSyslogWinsockResolverPool, CreateAcquiresAndReleasesConfigLockOnFirstFreeSlot)
{
    ConfigLockFake_Install();

    pooled[0] = SolidSyslogWinsockResolver_Create();

    CALLED_FAKE(ConfigLockFake_Lock, ONCE);
    CALLED_FAKE(ConfigLockFake_Unlock, ONCE);
}

TEST(SolidSyslogWinsockResolverPool, CreateLocksOncePerSlotProbedWhenPoolIsFull)
{
    FillPool();
    ConfigLockFake_Install();

    overflow = SolidSyslogWinsockResolver_Create();

    LONGS_EQUAL(SOLIDSYSLOG_WINSOCK_RESOLVER_POOL_SIZE, ConfigLockFake_LockCallCount());
    LONGS_EQUAL(SOLIDSYSLOG_WINSOCK_RESOLVER_POOL_SIZE, ConfigLockFake_UnlockCallCount());
}

TEST(SolidSyslogWinsockResolverPool, DestroyOfPooledHandleLocksOnce)
{
    pooled[0] = SolidSyslogWinsockResolver_Create();
    ConfigLockFake_Install();

    SolidSyslogWinsockResolver_Destroy(pooled[0]);
    pooled[0] = nullptr;

    CALLED_FAKE(ConfigLockFake_Lock, ONCE);
    CALLED_FAKE(ConfigLockFake_Unlock, ONCE);
}

TEST(SolidSyslogWinsockResolverPool, DestroyOfUnknownHandleDoesNotLock)
{
    ConfigLockFake_Install();
    struct SolidSyslogResolver stranger = {};

    SolidSyslogWinsockResolver_Destroy(&stranger);

    CALLED_FAKE(ConfigLockFake_Lock, NEVER);
    CALLED_FAKE(ConfigLockFake_Unlock, NEVER);
}

TEST(SolidSyslogWinsockResolverPool, DestroyOfUnknownHandleReportsWarning)
{
    ErrorHandlerFake_Install(nullptr);
    struct SolidSyslogResolver stranger = {};

    SolidSyslogWinsockResolver_Destroy(&stranger);

    CALLED_FAKE(ErrorHandlerFake_Handle, ONCE);
    LONGS_EQUAL(SOLIDSYSLOG_SEVERITY_WARNING, ErrorHandlerFake_LastSeverity());
    STRCMP_EQUAL(SOLIDSYSLOG_ERROR_MSG_WINSOCKRESOLVER_UNKNOWN_DESTROY, ErrorHandlerFake_LastMessage());
}

TEST(SolidSyslogWinsockResolverPool, DestroyOfStaleHandleReportsWarning)
{
    pooled[0] = SolidSyslogWinsockResolver_Create();
    SolidSyslogWinsockResolver_Destroy(pooled[0]);
    ErrorHandlerFake_Install(nullptr);

    SolidSyslogWinsockResolver_Destroy(pooled[0]);
    pooled[0] = nullptr;

    CALLED_FAKE(ErrorHandlerFake_Handle, ONCE);
    LONGS_EQUAL(SOLIDSYSLOG_SEVERITY_WARNING, ErrorHandlerFake_LastSeverity());
    STRCMP_EQUAL(SOLIDSYSLOG_ERROR_MSG_WINSOCKRESOLVER_UNKNOWN_DESTROY, ErrorHandlerFake_LastMessage());
}
