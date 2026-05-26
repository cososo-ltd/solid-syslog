#include "TestUtils.h"
#include "CppUTest/TestHarness.h"

using namespace CososoTesting; // NOLINT(google-build-using-namespace) -- test-file scope only; brings NEVER/ONCE/TWICE/THRICE into scope for the CALLED_*
    // macros

#include <cstdint>

#include "ConfigLockFake.h"
#include "ErrorHandlerFake.h"
#include "SolidSyslogLwipRawAddress.h"
#include "SolidSyslogLwipRawAddressPrivate.h"
#include "SolidSyslogLwipRawResolver.h"
#include "SolidSyslogLwipRawResolverErrors.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogResolver.h"
#include "SolidSyslogResolverDefinition.h"
#include "SolidSyslogTransport.h"
#include "SolidSyslogTunables.h"

#include "lwip/ip_addr.h"

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

// Asserts the most recent ErrorHandlerFake call matched (severity, source, code).
// Use after the act-phase of a test that expects exactly one SolidSyslog_Error call.
#define CHECK_REPORTED(severity, source, code)                     \
    do                                                             \
    {                                                              \
        CALLED_FAKE(ErrorHandlerFake_Handle, ONCE);                \
        LONGS_EQUAL((severity), ErrorHandlerFake_LastSeverity());  \
        POINTERS_EQUAL(&(source), ErrorHandlerFake_LastSource());  \
        UNSIGNED_LONGS_EQUAL((code), ErrorHandlerFake_LastCode()); \
    } while (0)

// NOLINTEND(cppcoreguidelines-macro-usage,cppcoreguidelines-avoid-do-while)

static const char* const TEST_HOST = "127.0.0.1";
static const uint16_t TEST_PORT = 514;

// clang-format off
TEST_GROUP(SolidSyslogLwipRawResolver)
{
    struct SolidSyslogResolver* resolver = nullptr;
    struct SolidSyslogAddress*  addr     = nullptr;

    void setup() override
    {
        resolver = SolidSyslogLwipRawResolver_Create();
        addr     = SolidSyslogLwipRawAddress_Create();
    }

    void teardown() override
    {
        SolidSyslogLwipRawAddress_Destroy(addr);
        SolidSyslogLwipRawResolver_Destroy(resolver);
    }

    bool Resolve(const char* host = TEST_HOST, uint16_t port = TEST_PORT, enum SolidSyslogTransport transport = SOLIDSYSLOG_TRANSPORT_UDP) const
    {
        return SolidSyslogResolver_Resolve(resolver, transport, host, port, addr);
    }
};

// clang-format on

TEST(SolidSyslogLwipRawResolver, CreateSucceeds)
{
    CHECK(resolver != nullptr);
}

TEST(SolidSyslogLwipRawResolver, ResolveReturnsTrueOnLiteralIpv4)
{
    CHECK_TRUE(Resolve());
}

TEST(SolidSyslogLwipRawResolver, ResolvePopulatesIpFromLiteralIpv4)
{
    Resolve("127.0.0.1");

    ip_addr_t expected;
    IP4_ADDR(&expected, 127, 0, 0, 1);
    LONGS_EQUAL(
        ip4_addr_get_u32(ip_2_ip4(&expected)),
        ip4_addr_get_u32(ip_2_ip4(&SolidSyslogLwipRawAddress_AsConst(addr)->Ip))
    );
}

TEST(SolidSyslogLwipRawResolver, ResolvePopulatesPortFromArg)
{
    Resolve(TEST_HOST, 9999);

    LONGS_EQUAL(9999, SolidSyslogLwipRawAddress_AsConst(addr)->Port);
}

TEST(SolidSyslogLwipRawResolver, ResolveReturnsFalseOnNonLiteralHost)
{
    // Locks in the no-DNS contract: a hostname that isn't a literal dotted-quad
    // is rejected. Real DNS would land as the future SolidSyslogLwipRawDnsResolver
    // sibling (S28.07), not as a flag on this class.
    CHECK_FALSE(Resolve("example.com"));
}

TEST(SolidSyslogLwipRawResolver, ResolveReturnsFalseWhenIpaddrAtonRejectsHost)
{
    // We defer to lwIP's ipaddr_aton — whatever the parser accepts, we accept;
    // whatever it rejects, we reject. We do not enforce any specific shape on
    // top of the parser (no dotted-quad-only check) so the tests assert only
    // the rejection contract for clearly non-numeric inputs (DNS names,
    // alphabetic, empty), not a specific accept-list.
    CHECK_FALSE(Resolve("a.b.c.d"));
    CHECK_FALSE(Resolve(""));
}

// clang-format off
TEST_GROUP(SolidSyslogLwipRawResolverPool)
{
    // cppcheck-suppress constVariable -- assigned in test bodies; cppcheck does not model CppUTest lifecycle
    struct SolidSyslogResolver* pooled[SOLIDSYSLOG_LWIP_RAW_RESOLVER_POOL_SIZE] = {};
    struct SolidSyslogResolver* overflow                                         = nullptr;

    void teardown() override
    {
        for (auto* handle : pooled)
        {
            if (handle != nullptr)
            {
                SolidSyslogLwipRawResolver_Destroy(handle);
            }
        }
        // cppcheck-suppress knownConditionTrueFalse -- assigned in test bodies; cppcheck does not model CppUTest lifecycle
        if (overflow != nullptr)
        {
            SolidSyslogLwipRawResolver_Destroy(overflow);
        }
        ConfigLockFake_Uninstall();
    }

    void FillPool()
    {
        for (auto*& slot : pooled)
        {
            slot = SolidSyslogLwipRawResolver_Create();
        }
    }
};

// clang-format on

TEST(SolidSyslogLwipRawResolverPool, FillingPoolThenOverflowReturnsDistinctFallback)
{
    FillPool();

    overflow = SolidSyslogLwipRawResolver_Create();

    CHECK_IS_FALLBACK(overflow, pooled);
}

TEST(SolidSyslogLwipRawResolverPool, ExhaustedCreateReportsError)
{
    ErrorHandlerFake_Install(nullptr);
    FillPool();

    overflow = SolidSyslogLwipRawResolver_Create();

    CHECK_REPORTED(SOLIDSYSLOG_SEVERITY_ERROR, LwipRawResolverErrorSource, LWIPRAWRESOLVER_ERROR_POOL_EXHAUSTED);
}

TEST(SolidSyslogLwipRawResolverPool, FallbackResolveReturnsFalse)
{
    FillPool();
    overflow = SolidSyslogLwipRawResolver_Create();
    struct SolidSyslogAddress* fallbackResult = SolidSyslogLwipRawAddress_Create();

    CHECK_FALSE(SolidSyslogResolver_Resolve(overflow, SOLIDSYSLOG_TRANSPORT_UDP, TEST_HOST, TEST_PORT, fallbackResult));

    SolidSyslogLwipRawAddress_Destroy(fallbackResult);
}

TEST(SolidSyslogLwipRawResolverPool, CreateAcquiresAndReleasesConfigLockOnFirstFreeSlot)
{
    ConfigLockFake_Install();

    pooled[0] = SolidSyslogLwipRawResolver_Create();

    CALLED_FAKE(ConfigLockFake_Lock, ONCE);
    CALLED_FAKE(ConfigLockFake_Unlock, ONCE);
}

TEST(SolidSyslogLwipRawResolverPool, CreateLocksOncePerSlotProbedWhenPoolIsFull)
{
    FillPool();
    ConfigLockFake_Install();

    overflow = SolidSyslogLwipRawResolver_Create();

    LONGS_EQUAL(SOLIDSYSLOG_LWIP_RAW_RESOLVER_POOL_SIZE, ConfigLockFake_LockCallCount());
    LONGS_EQUAL(SOLIDSYSLOG_LWIP_RAW_RESOLVER_POOL_SIZE, ConfigLockFake_UnlockCallCount());
}

TEST(SolidSyslogLwipRawResolverPool, DestroyOfPooledHandleLocksOnce)
{
    pooled[0] = SolidSyslogLwipRawResolver_Create();
    ConfigLockFake_Install();

    SolidSyslogLwipRawResolver_Destroy(pooled[0]);
    pooled[0] = nullptr;

    CALLED_FAKE(ConfigLockFake_Lock, ONCE);
    CALLED_FAKE(ConfigLockFake_Unlock, ONCE);
}

TEST(SolidSyslogLwipRawResolverPool, DestroyOfUnknownHandleDoesNotLock)
{
    ConfigLockFake_Install();
    struct SolidSyslogResolver stranger = {};

    SolidSyslogLwipRawResolver_Destroy(&stranger);

    CALLED_FAKE(ConfigLockFake_Lock, NEVER);
    CALLED_FAKE(ConfigLockFake_Unlock, NEVER);
}

TEST(SolidSyslogLwipRawResolverPool, DestroyOfUnknownHandleReportsWarning)
{
    ErrorHandlerFake_Install(nullptr);
    struct SolidSyslogResolver stranger = {};

    SolidSyslogLwipRawResolver_Destroy(&stranger);

    CHECK_REPORTED(SOLIDSYSLOG_SEVERITY_WARNING, LwipRawResolverErrorSource, LWIPRAWRESOLVER_ERROR_UNKNOWN_DESTROY);
}

TEST(SolidSyslogLwipRawResolverPool, DestroyOfStaleHandleReportsWarning)
{
    pooled[0] = SolidSyslogLwipRawResolver_Create();
    SolidSyslogLwipRawResolver_Destroy(pooled[0]);
    ErrorHandlerFake_Install(nullptr);

    SolidSyslogLwipRawResolver_Destroy(pooled[0]);
    pooled[0] = nullptr;

    CHECK_REPORTED(SOLIDSYSLOG_SEVERITY_WARNING, LwipRawResolverErrorSource, LWIPRAWRESOLVER_ERROR_UNKNOWN_DESTROY);
}

TEST(SolidSyslogLwipRawResolver, UdpTransportResolvesIdenticallyToTcp)
{
    // Locks in that the literal-IPv4 resolver does not dispatch on transport —
    // a future reader must not add transport-typed pcb selection here.
    Resolve(TEST_HOST, TEST_PORT, SOLIDSYSLOG_TRANSPORT_UDP);
    uint32_t udpIp = ip4_addr_get_u32(ip_2_ip4(&SolidSyslogLwipRawAddress_AsConst(addr)->Ip));
    uint16_t udpPort = SolidSyslogLwipRawAddress_AsConst(addr)->Port;

    Resolve(TEST_HOST, TEST_PORT, SOLIDSYSLOG_TRANSPORT_TCP);

    LONGS_EQUAL(udpIp, ip4_addr_get_u32(ip_2_ip4(&SolidSyslogLwipRawAddress_AsConst(addr)->Ip)));
    LONGS_EQUAL(udpPort, SolidSyslogLwipRawAddress_AsConst(addr)->Port);
}
