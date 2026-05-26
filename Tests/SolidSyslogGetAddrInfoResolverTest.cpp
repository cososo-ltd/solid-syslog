#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <cstdint>

#include "ConfigLockFake.h"
#include "ErrorHandlerFake.h"
#include "SolidSyslogGetAddrInfoResolver.h"
#include "SolidSyslogGetAddrInfoResolverErrors.h"
#include "SolidSyslogPosixAddress.h"
#include "SolidSyslogPosixAddressPrivate.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogResolver.h"
#include "SolidSyslogResolverDefinition.h"
#include "SocketFake.h"
#include "SolidSyslogTransport.h"
#include "SolidSyslogTunables.h"
#include "TestUtils.h"
#include "CppUTest/TestHarness.h"

using namespace CososoTesting;

// clang-format off
static const char* const TEST_HOST           = "127.0.0.1";
static const uint16_t    TEST_PORT           = 514;
static const char* const TEST_ALTERNATE_HOST = "192.168.1.1";
static const uint16_t    TEST_ALTERNATE_PORT = 9999;
// clang-format on

// clang-format off
TEST_GROUP(SolidSyslogGetAddrInfoResolver)
{
    struct SolidSyslogResolver* resolver = nullptr;
    struct SolidSyslogAddress*  result   = nullptr;

    void setup() override
    {
        SocketFake_Reset();
        resolver = SolidSyslogGetAddrInfoResolver_Create();
        result   = SolidSyslogPosixAddress_Create();
    }

    void teardown() override
    {
        SolidSyslogPosixAddress_Destroy(result);
        SolidSyslogGetAddrInfoResolver_Destroy(resolver);
    }

    bool Resolve(const char* host, uint16_t port, enum SolidSyslogTransport transport = SOLIDSYSLOG_TRANSPORT_UDP) const
    {
        return SolidSyslogResolver_Resolve(resolver, transport, host, port, result);
    }

    // NOLINTNEXTLINE(modernize-use-nodiscard) -- used through accessor syntax in tests
    const struct sockaddr_in* Result() const
    {
        return SolidSyslogPosixAddress_AsConstSockaddrIn(result);
    }
};

// clang-format on

TEST(SolidSyslogGetAddrInfoResolver, CreateDestroyWorksWithoutCrashing)
{
}

TEST(SolidSyslogGetAddrInfoResolver, ReturnsTrueOnSuccess)
{
    CHECK_TRUE(Resolve(TEST_HOST, TEST_PORT));
}

TEST(SolidSyslogGetAddrInfoResolver, PopulatesAddressFamily)
{
    Resolve(TEST_HOST, TEST_PORT);
    LONGS_EQUAL(AF_INET, Result()->sin_family);
}

TEST(SolidSyslogGetAddrInfoResolver, PopulatesResolvedAddressFromHostArgument)
{
    Resolve(TEST_HOST, TEST_PORT);
    char addrString[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &Result()->sin_addr, addrString, sizeof(addrString));
    STRCMP_EQUAL(TEST_HOST, addrString);
}

TEST(SolidSyslogGetAddrInfoResolver, PopulatesPortFromPortArgument)
{
    Resolve(TEST_HOST, TEST_ALTERNATE_PORT);
    LONGS_EQUAL(TEST_ALTERNATE_PORT, ntohs(Result()->sin_port));
}

TEST(SolidSyslogGetAddrInfoResolver, GetAddrInfoCalledWithHostArgument)
{
    Resolve(TEST_ALTERNATE_HOST, TEST_PORT);
    CALLED_FAKE(SocketFake_GetAddrInfo, ONCE);
    STRCMP_EQUAL(TEST_ALTERNATE_HOST, SocketFake_LastGetAddrInfoHostname());
}

TEST(SolidSyslogGetAddrInfoResolver, UdpTransportPassesDatagramSocktype)
{
    Resolve(TEST_HOST, TEST_PORT, SOLIDSYSLOG_TRANSPORT_UDP);
    LONGS_EQUAL(SOCK_DGRAM, SocketFake_LastGetAddrInfoSocktype());
}

TEST(SolidSyslogGetAddrInfoResolver, TcpTransportPassesStreamSocktype)
{
    Resolve(TEST_HOST, TEST_PORT, SOLIDSYSLOG_TRANSPORT_TCP);
    LONGS_EQUAL(SOCK_STREAM, SocketFake_LastGetAddrInfoSocktype());
}

TEST(SolidSyslogGetAddrInfoResolver, ReturnsFalseWhenGetAddrInfoFails)
{
    SocketFake_SetGetAddrInfoFails(true);
    CHECK_FALSE(Resolve(TEST_HOST, TEST_PORT));
}

TEST(SolidSyslogGetAddrInfoResolver, DoesNotFreeAddrInfoWhenGetAddrInfoFails)
{
    SocketFake_SetGetAddrInfoFails(true);
    Resolve(TEST_HOST, TEST_PORT);
    CALLED_FAKE(SocketFake_FreeAddrInfo, NEVER);
}

TEST(SolidSyslogGetAddrInfoResolver, FreesAddrInfoOnSuccess)
{
    Resolve(TEST_HOST, TEST_PORT);
    CALLED_FAKE(SocketFake_FreeAddrInfo, ONCE);
}

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
TEST_GROUP(SolidSyslogGetAddrInfoResolverPool)
{
    struct SolidSyslogResolver* pooled[SOLIDSYSLOG_GETADDRINFO_RESOLVER_POOL_SIZE] = {};
    struct SolidSyslogResolver* overflow                                            = nullptr;

    void teardown() override
    {
        for (auto* handle : pooled)
        {
            if (handle != nullptr)
            {
                SolidSyslogGetAddrInfoResolver_Destroy(handle);
            }
        }
        if (overflow != nullptr)
        {
            SolidSyslogGetAddrInfoResolver_Destroy(overflow);
        }
        ConfigLockFake_Uninstall();
    }

    void FillPool()
    {
        for (auto*& slot : pooled)
        {
            slot = SolidSyslogGetAddrInfoResolver_Create();
        }
    }
};

// clang-format on

TEST(SolidSyslogGetAddrInfoResolverPool, FillingPoolThenOverflowReturnsDistinctFallback)
{
    FillPool();

    overflow = SolidSyslogGetAddrInfoResolver_Create();

    CHECK_IS_FALLBACK(overflow, pooled);
}

TEST(SolidSyslogGetAddrInfoResolverPool, ExhaustedCreateReportsError)
{
    ErrorHandlerFake_Install(nullptr);
    FillPool();

    overflow = SolidSyslogGetAddrInfoResolver_Create();

    CALLED_FAKE(ErrorHandlerFake_Handle, ONCE);
    LONGS_EQUAL(SOLIDSYSLOG_SEVERITY_ERROR, ErrorHandlerFake_LastSeverity());
    POINTERS_EQUAL(&GetAddrInfoResolverErrorSource, ErrorHandlerFake_LastSource());
    UNSIGNED_LONGS_EQUAL(GETADDRINFORESOLVER_ERROR_POOL_EXHAUSTED, ErrorHandlerFake_LastCode());
}

TEST(SolidSyslogGetAddrInfoResolverPool, FallbackResolveReturnsFalse)
{
    FillPool();
    overflow = SolidSyslogGetAddrInfoResolver_Create();

    struct SolidSyslogAddress* fallbackResult = SolidSyslogPosixAddress_Create();
    CHECK_FALSE(SolidSyslogResolver_Resolve(overflow, SOLIDSYSLOG_TRANSPORT_UDP, TEST_HOST, TEST_PORT, fallbackResult));
    SolidSyslogPosixAddress_Destroy(fallbackResult);
}

TEST(SolidSyslogGetAddrInfoResolverPool, CreateAcquiresAndReleasesConfigLockOnFirstFreeSlot)
{
    ConfigLockFake_Install();

    pooled[0] = SolidSyslogGetAddrInfoResolver_Create();

    CALLED_FAKE(ConfigLockFake_Lock, ONCE);
    CALLED_FAKE(ConfigLockFake_Unlock, ONCE);
}

TEST(SolidSyslogGetAddrInfoResolverPool, CreateLocksOncePerSlotProbedWhenPoolIsFull)
{
    FillPool();
    ConfigLockFake_Install();

    overflow = SolidSyslogGetAddrInfoResolver_Create();

    LONGS_EQUAL(SOLIDSYSLOG_GETADDRINFO_RESOLVER_POOL_SIZE, ConfigLockFake_LockCallCount());
    LONGS_EQUAL(SOLIDSYSLOG_GETADDRINFO_RESOLVER_POOL_SIZE, ConfigLockFake_UnlockCallCount());
}

TEST(SolidSyslogGetAddrInfoResolverPool, DestroyOfPooledHandleLocksOnce)
{
    pooled[0] = SolidSyslogGetAddrInfoResolver_Create();
    ConfigLockFake_Install();

    SolidSyslogGetAddrInfoResolver_Destroy(pooled[0]);
    pooled[0] = nullptr;

    CALLED_FAKE(ConfigLockFake_Lock, ONCE);
    CALLED_FAKE(ConfigLockFake_Unlock, ONCE);
}

TEST(SolidSyslogGetAddrInfoResolverPool, DestroyOfUnknownHandleDoesNotLock)
{
    ConfigLockFake_Install();
    struct SolidSyslogResolver stranger = {};

    SolidSyslogGetAddrInfoResolver_Destroy(&stranger);

    CALLED_FAKE(ConfigLockFake_Lock, NEVER);
    CALLED_FAKE(ConfigLockFake_Unlock, NEVER);
}

TEST(SolidSyslogGetAddrInfoResolverPool, DestroyOfUnknownHandleReportsWarning)
{
    ErrorHandlerFake_Install(nullptr);
    struct SolidSyslogResolver stranger = {};

    SolidSyslogGetAddrInfoResolver_Destroy(&stranger);

    CALLED_FAKE(ErrorHandlerFake_Handle, ONCE);
    LONGS_EQUAL(SOLIDSYSLOG_SEVERITY_WARNING, ErrorHandlerFake_LastSeverity());
    POINTERS_EQUAL(&GetAddrInfoResolverErrorSource, ErrorHandlerFake_LastSource());
    UNSIGNED_LONGS_EQUAL(GETADDRINFORESOLVER_ERROR_UNKNOWN_DESTROY, ErrorHandlerFake_LastCode());
}

TEST(SolidSyslogGetAddrInfoResolverPool, DestroyOfStaleHandleReportsWarning)
{
    pooled[0] = SolidSyslogGetAddrInfoResolver_Create();
    SolidSyslogGetAddrInfoResolver_Destroy(pooled[0]);
    ErrorHandlerFake_Install(nullptr);

    SolidSyslogGetAddrInfoResolver_Destroy(pooled[0]);
    pooled[0] = nullptr;

    CALLED_FAKE(ErrorHandlerFake_Handle, ONCE);
    LONGS_EQUAL(SOLIDSYSLOG_SEVERITY_WARNING, ErrorHandlerFake_LastSeverity());
    POINTERS_EQUAL(&GetAddrInfoResolverErrorSource, ErrorHandlerFake_LastSource());
    UNSIGNED_LONGS_EQUAL(GETADDRINFORESOLVER_ERROR_UNKNOWN_DESTROY, ErrorHandlerFake_LastCode());
}
