#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <cstdint>

#include "ConfigLockFake.h"
#include "CppUTest/TestHarness.h"
#include "ErrorHandlerFake.h"
#include "SocketFake.h"
#include "SolidSyslogErrorCategory.h"
#include "SolidSyslogPosixResolver.h"
#include "SolidSyslogPosixResolverErrors.h"
#include "SolidSyslogPosixAddress.h"
#include "SolidSyslogPosixAddressPrivate.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogResolver.h"
#include "SolidSyslogResolverDefinition.h"
#include "SolidSyslogTransport.h"
#include "SolidSyslogTunables.h"
#include "TestUtils.h"

using namespace CososoTesting;

// clang-format off
static const char* const TEST_HOST           = "127.0.0.1";
static const uint16_t    TEST_PORT           = 514;
static const char* const TEST_ALTERNATE_HOST = "192.168.1.1";
static const uint16_t    TEST_ALTERNATE_PORT = 9999;
// clang-format on

// clang-format off
TEST_GROUP(SolidSyslogPosixResolver)
{
    struct SolidSyslogResolver* resolver = nullptr;
    struct SolidSyslogAddress*  result   = nullptr;

    void setup() override
    {
        SocketFake_Reset();
        resolver = SolidSyslogPosixResolver_Create();
        result   = SolidSyslogPosixAddress_Create();
    }

    void teardown() override
    {
        SolidSyslogPosixAddress_Destroy(result);
        SolidSyslogPosixResolver_Destroy(resolver);
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

TEST(SolidSyslogPosixResolver, CreateDestroyWorksWithoutCrashing)
{
}

TEST(SolidSyslogPosixResolver, ReturnsTrueOnSuccess)
{
    CHECK_TRUE(Resolve(TEST_HOST, TEST_PORT));
}

TEST(SolidSyslogPosixResolver, PopulatesAddressFamily)
{
    Resolve(TEST_HOST, TEST_PORT);
    LONGS_EQUAL(AF_INET, Result()->sin_family);
}

TEST(SolidSyslogPosixResolver, PopulatesResolvedAddressFromHostArgument)
{
    Resolve(TEST_HOST, TEST_PORT);
    char addrString[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &Result()->sin_addr, addrString, sizeof(addrString));
    STRCMP_EQUAL(TEST_HOST, addrString);
}

TEST(SolidSyslogPosixResolver, PopulatesPortFromPortArgument)
{
    Resolve(TEST_HOST, TEST_ALTERNATE_PORT);
    LONGS_EQUAL(TEST_ALTERNATE_PORT, ntohs(Result()->sin_port));
}

TEST(SolidSyslogPosixResolver, GetAddrInfoCalledWithHostArgument)
{
    Resolve(TEST_ALTERNATE_HOST, TEST_PORT);
    CALLED_FAKE(SocketFake_GetAddrInfo, ONCE);
    STRCMP_EQUAL(TEST_ALTERNATE_HOST, SocketFake_LastGetAddrInfoHostname());
}

TEST(SolidSyslogPosixResolver, UdpTransportPassesDatagramSocktype)
{
    Resolve(TEST_HOST, TEST_PORT, SOLIDSYSLOG_TRANSPORT_UDP);
    LONGS_EQUAL(SOCK_DGRAM, SocketFake_LastGetAddrInfoSocktype());
}

TEST(SolidSyslogPosixResolver, TcpTransportPassesStreamSocktype)
{
    Resolve(TEST_HOST, TEST_PORT, SOLIDSYSLOG_TRANSPORT_TCP);
    LONGS_EQUAL(SOCK_STREAM, SocketFake_LastGetAddrInfoSocktype());
}

TEST(SolidSyslogPosixResolver, ReturnsFalseWhenGetAddrInfoFails)
{
    SocketFake_SetGetAddrInfoFails(true);
    CHECK_FALSE(Resolve(TEST_HOST, TEST_PORT));
}

TEST(SolidSyslogPosixResolver, DoesNotFreeAddrInfoWhenGetAddrInfoFails)
{
    SocketFake_SetGetAddrInfoFails(true);
    Resolve(TEST_HOST, TEST_PORT);
    CALLED_FAKE(SocketFake_FreeAddrInfo, NEVER);
}

TEST(SolidSyslogPosixResolver, FreesAddrInfoOnSuccess)
{
    Resolve(TEST_HOST, TEST_PORT);
    CALLED_FAKE(SocketFake_FreeAddrInfo, ONCE);
}

#define CHECK_IS_FALLBACK(handle, pool)                                                \
    {                                                                                  \
        CHECK_TEXT((handle) != nullptr, "Fallback handle was nullptr");                \
        for (auto* slot : (pool))                                                      \
        {                                                                              \
            CHECK_TEXT(slot != nullptr, "pool slot was nullptr (FillPool failed?)");   \
            CHECK_TEXT((handle) != slot, "Fallback handle collided with a pool slot"); \
        }                                                                              \
    }

// clang-format off
TEST_GROUP(SolidSyslogPosixResolverPool)
{
    struct SolidSyslogResolver* pooled[SOLIDSYSLOG_RESOLVER_POOL_SIZE] = {};
    struct SolidSyslogResolver* overflow                                            = nullptr;

    void teardown() override
    {
        for (auto* handle : pooled)
        {
            if (handle != nullptr)
            {
                SolidSyslogPosixResolver_Destroy(handle);
            }
        }
        if (overflow != nullptr)
        {
            SolidSyslogPosixResolver_Destroy(overflow);
        }
        ConfigLockFake_Uninstall();
    }

    void FillPool()
    {
        for (auto*& slot : pooled)
        {
            slot = SolidSyslogPosixResolver_Create();
        }
    }
};

// clang-format on

TEST(SolidSyslogPosixResolverPool, FillingPoolThenOverflowReturnsDistinctFallback)
{
    FillPool();

    overflow = SolidSyslogPosixResolver_Create();

    CHECK_IS_FALLBACK(overflow, pooled);
}

TEST(SolidSyslogPosixResolverPool, ExhaustedCreateReportsError)
{
    ErrorHandlerFake_Install(nullptr);
    FillPool();

    overflow = SolidSyslogPosixResolver_Create();

    CALLED_FAKE(ErrorHandlerFake_Handle, ONCE);
    LONGS_EQUAL(SOLIDSYSLOG_SEVERITY_CRITICAL, ErrorHandlerFake_LastSeverity());
    POINTERS_EQUAL(&PosixResolverErrorSource, ErrorHandlerFake_LastSource());
    UNSIGNED_LONGS_EQUAL(SOLIDSYSLOG_CAT_POOL_EXHAUSTED, ErrorHandlerFake_LastCategory());
    UNSIGNED_LONGS_EQUAL(SOLIDSYSLOG_POSIX_RESOLVER_ERROR_POOL_EXHAUSTED, ErrorHandlerFake_LastDetail());
}

TEST(SolidSyslogPosixResolverPool, FallbackResolveReturnsFalse)
{
    FillPool();
    overflow = SolidSyslogPosixResolver_Create();

    struct SolidSyslogAddress* fallbackResult = SolidSyslogPosixAddress_Create();
    CHECK_FALSE(SolidSyslogResolver_Resolve(overflow, SOLIDSYSLOG_TRANSPORT_UDP, TEST_HOST, TEST_PORT, fallbackResult));
    SolidSyslogPosixAddress_Destroy(fallbackResult);
}

TEST(SolidSyslogPosixResolverPool, CreateAcquiresAndReleasesConfigLockOnFirstFreeSlot)
{
    ConfigLockFake_Install();

    pooled[0] = SolidSyslogPosixResolver_Create();

    CALLED_FAKE(ConfigLockFake_Lock, ONCE);
    CALLED_FAKE(ConfigLockFake_Unlock, ONCE);
}

TEST(SolidSyslogPosixResolverPool, CreateLocksOncePerSlotProbedWhenPoolIsFull)
{
    FillPool();
    ConfigLockFake_Install();

    overflow = SolidSyslogPosixResolver_Create();

    LONGS_EQUAL(SOLIDSYSLOG_RESOLVER_POOL_SIZE, ConfigLockFake_LockCallCount());
    LONGS_EQUAL(SOLIDSYSLOG_RESOLVER_POOL_SIZE, ConfigLockFake_UnlockCallCount());
}

TEST(SolidSyslogPosixResolverPool, DestroyOfPooledHandleLocksOnce)
{
    pooled[0] = SolidSyslogPosixResolver_Create();
    ConfigLockFake_Install();

    SolidSyslogPosixResolver_Destroy(pooled[0]);
    pooled[0] = nullptr;

    CALLED_FAKE(ConfigLockFake_Lock, ONCE);
    CALLED_FAKE(ConfigLockFake_Unlock, ONCE);
}

TEST(SolidSyslogPosixResolverPool, DestroyOfUnknownHandleDoesNotLock)
{
    ConfigLockFake_Install();
    struct SolidSyslogResolver stranger = {};

    SolidSyslogPosixResolver_Destroy(&stranger);

    CALLED_FAKE(ConfigLockFake_Lock, NEVER);
    CALLED_FAKE(ConfigLockFake_Unlock, NEVER);
}

TEST(SolidSyslogPosixResolverPool, DestroyOfUnknownHandleReportsWarning)
{
    ErrorHandlerFake_Install(nullptr);
    struct SolidSyslogResolver stranger = {};

    SolidSyslogPosixResolver_Destroy(&stranger);

    CALLED_FAKE(ErrorHandlerFake_Handle, ONCE);
    LONGS_EQUAL(SOLIDSYSLOG_SEVERITY_WARNING, ErrorHandlerFake_LastSeverity());
    POINTERS_EQUAL(&PosixResolverErrorSource, ErrorHandlerFake_LastSource());
    UNSIGNED_LONGS_EQUAL(SOLIDSYSLOG_CAT_UNKNOWN_DESTROY, ErrorHandlerFake_LastCategory());
    UNSIGNED_LONGS_EQUAL(SOLIDSYSLOG_POSIX_RESOLVER_ERROR_UNKNOWN_DESTROY, ErrorHandlerFake_LastDetail());
}

TEST(SolidSyslogPosixResolverPool, DestroyOfStaleHandleReportsWarning)
{
    pooled[0] = SolidSyslogPosixResolver_Create();
    SolidSyslogPosixResolver_Destroy(pooled[0]);
    ErrorHandlerFake_Install(nullptr);

    SolidSyslogPosixResolver_Destroy(pooled[0]);
    pooled[0] = nullptr;

    CALLED_FAKE(ErrorHandlerFake_Handle, ONCE);
    LONGS_EQUAL(SOLIDSYSLOG_SEVERITY_WARNING, ErrorHandlerFake_LastSeverity());
    POINTERS_EQUAL(&PosixResolverErrorSource, ErrorHandlerFake_LastSource());
    UNSIGNED_LONGS_EQUAL(SOLIDSYSLOG_CAT_UNKNOWN_DESTROY, ErrorHandlerFake_LastCategory());
    UNSIGNED_LONGS_EQUAL(SOLIDSYSLOG_POSIX_RESOLVER_ERROR_UNKNOWN_DESTROY, ErrorHandlerFake_LastDetail());
}
