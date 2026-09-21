#include "TestUtils.h"
#include "CppUTest/TestHarness.h"

using namespace CososoTesting;

#include "lwip/sockets.h"

#include <cstdint>

#include "ConfigLockFake.h"
#include "ErrorHandlerFake.h"
#include "LwipNetdbFake.h"
#include "SolidSyslogErrorCategory.h"
#include "SolidSyslogNullResolver.h"
#include "SolidSyslogTunables.h"
#include "SolidSyslogLwipSocketAddress.h"
#include "SolidSyslogLwipSocketAddressPrivate.h"
#include "SolidSyslogLwipSocketResolver.h"
#include "SolidSyslogLwipSocketResolverErrors.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogResolverCategories.h"
#include "SolidSyslogResolver.h"
#include "SolidSyslogTransport.h"

// clang-format off
// 6514 rather than 514: 514 is 0x0202, so host and network order are the same
// bytes and an assertion on the port would hold whichever the adapter wrote.
static const char* const TEST_HOST            = "collector.example.test";
static const char* const TEST_ALTERNATE_HOST  = "elsewhere.example.test";
static const char* const TEST_IPV4            = "192.0.2.10";
static const uint32_t    TEST_IPV4_HOST_ORDER = 0xC000020AU;
static const uint16_t    TEST_PORT            = 6514U;
static const uint16_t    TEST_ALTERNATE_PORT  = 9999U;
// clang-format on

// clang-format off
TEST_GROUP(SolidSyslogLwipSocketResolver)
{
    struct SolidSyslogResolver* resolver = nullptr;
    struct SolidSyslogAddress*  result   = nullptr;

    void setup() override
    {
        LwipNetdbFake_Reset();
        resolver = SolidSyslogLwipSocketResolver_Create();
        result   = SolidSyslogLwipSocketAddress_Create();
        // Installed after the pool draws above, so an event count starts clean.
        ErrorHandlerFake_Install(nullptr);
    }

    void teardown() override
    {
        SolidSyslogLwipSocketAddress_Destroy(result);
        SolidSyslogLwipSocketResolver_Destroy(resolver);
    }

    bool Resolve(const char* host, uint16_t port, enum SolidSyslogTransport transport = SOLIDSYSLOG_TRANSPORT_UDP) const
    {
        return SolidSyslogResolver_Resolve(resolver, transport, host, port, result);
    }

    // NOLINTNEXTLINE(modernize-use-nodiscard) -- used through accessor syntax in tests
    const struct sockaddr_in* Result() const
    {
        return SolidSyslogLwipSocketAddress_AsConstSockaddrIn(result);
    }
};

// clang-format on

TEST(SolidSyslogLwipSocketResolver, CreateReturnsNonNull)
{
    CHECK(resolver != nullptr);
}

TEST(SolidSyslogLwipSocketResolver, ResolveWritesTheResolvedIpv4Endpoint)
{
    LwipNetdbFake_SetIpv4Result(TEST_IPV4);

    CHECK_TRUE(Resolve(TEST_HOST, TEST_PORT));

    LONGS_EQUAL(AF_INET, Result()->sin_family);
    LONGS_EQUAL(lwip_htons(TEST_PORT), Result()->sin_port);
    LONGS_EQUAL(lwip_htonl(TEST_IPV4_HOST_ORDER), Result()->sin_addr.s_addr);
}

TEST(SolidSyslogLwipSocketResolver, ResolveAsksForIpv4ByNameWithNoServiceName)
{
    CHECK_TRUE(Resolve(TEST_HOST, TEST_PORT));

    LONGS_EQUAL(1U, LwipNetdbFake_GetAddrInfoCallCount());
    STRCMP_EQUAL(TEST_HOST, LwipNetdbFake_LastNodename());
    POINTERS_EQUAL(nullptr, LwipNetdbFake_LastServname());
    LONGS_EQUAL(AF_INET, LwipNetdbFake_LastHintsFamily());
}

TEST(SolidSyslogLwipSocketResolver, ResolveFreesTheAnswerItWasGiven)
{
    CHECK_TRUE(Resolve(TEST_HOST, TEST_PORT));

    LONGS_EQUAL(1U, LwipNetdbFake_FreeAddrInfoCallCount());
}

TEST(SolidSyslogLwipSocketResolver, AFailedLookupIsRefusedWithoutAnEvent)
{
    LwipNetdbFake_SetReturn(EAI_FAIL);

    CHECK_FALSE(Resolve(TEST_HOST, TEST_PORT));

    CALLED_FAKE(ErrorHandlerFake_Handle, NEVER);
    LONGS_EQUAL(0U, LwipNetdbFake_FreeAddrInfoCallCount());
}

TEST(SolidSyslogLwipSocketResolver, AFailedLookupLeavesTheCallersAddressAlone)
{
    LwipNetdbFake_SetIpv4Result(TEST_IPV4);
    CHECK_TRUE(Resolve(TEST_HOST, TEST_PORT));
    LwipNetdbFake_SetReturn(EAI_FAIL);

    CHECK_FALSE(Resolve(TEST_ALTERNATE_HOST, TEST_ALTERNATE_PORT));

    LONGS_EQUAL(lwip_htons(TEST_PORT), Result()->sin_port);
    LONGS_EQUAL(lwip_htonl(TEST_IPV4_HOST_ORDER), Result()->sin_addr.s_addr);
}

TEST(SolidSyslogLwipSocketResolver, AnAnswerInAnotherFamilyIsRefusedAndReported)
{
    LwipNetdbFake_SetResultFamily(AF_INET6);

    CHECK_FALSE(Resolve(TEST_HOST, TEST_PORT));

    CHECK_ERROR_REPORTED_ONCE(
        SOLIDSYSLOG_SEVERITY_ERROR,
        &SolidSyslogLwipSocketResolverErrorSource,
        SOLIDSYSLOG_CAT_RESOLVER_RESOLVE_FAILED,
        SOLIDSYSLOG_RESOLVER_ERROR_ADDRESS_FAMILY_UNSUPPORTED
    );
}

TEST(SolidSyslogLwipSocketResolver, AnAnswerInAnotherFamilyIsStillFreed)
{
    LwipNetdbFake_SetResultFamily(AF_INET6);

    CHECK_FALSE(Resolve(TEST_HOST, TEST_PORT));

    LONGS_EQUAL(1U, LwipNetdbFake_FreeAddrInfoCallCount());
}

TEST(SolidSyslogLwipSocketResolver, AnAnswerInAnotherFamilyLeavesTheCallersAddressAlone)
{
    LwipNetdbFake_SetResultFamily(AF_INET6);

    CHECK_FALSE(Resolve(TEST_HOST, TEST_PORT));

    LONGS_EQUAL(0U, Result()->sin_port);
    LONGS_EQUAL(0U, Result()->sin_addr.s_addr);
}

TEST(SolidSyslogLwipSocketResolver, AStackThatWillNotServeTheIpv4HintIsReported)
{
    LwipNetdbFake_SetReturn(EAI_FAMILY);

    CHECK_FALSE(Resolve(TEST_HOST, TEST_PORT));

    CHECK_ERROR_REPORTED_ONCE(
        SOLIDSYSLOG_SEVERITY_ERROR,
        &SolidSyslogLwipSocketResolverErrorSource,
        SOLIDSYSLOG_CAT_RESOLVER_RESOLVE_FAILED,
        SOLIDSYSLOG_RESOLVER_ERROR_ADDRESS_FAMILY_UNSUPPORTED
    );
}

TEST(SolidSyslogLwipSocketResolver, AStackThatWillNotServeTheIpv4HintFreesNothing)
{
    LwipNetdbFake_SetReturn(EAI_FAMILY);

    CHECK_FALSE(Resolve(TEST_HOST, TEST_PORT));

    LONGS_EQUAL(0U, LwipNetdbFake_FreeAddrInfoCallCount());
}

// clang-format off
TEST_GROUP(SolidSyslogLwipSocketResolverPool)
{
    struct SolidSyslogResolver* pooled[SOLIDSYSLOG_RESOLVER_POOL_SIZE] = {};
    struct SolidSyslogResolver* overflow                              = nullptr;

    void teardown() override
    {
        for (auto* handle : pooled)
        {
            if (handle != nullptr)
            {
                SolidSyslogLwipSocketResolver_Destroy(handle);
            }
        }
        ConfigLockFake_Uninstall();
    }

    void FillPool()
    {
        for (auto*& slot : pooled)
        {
            slot = SolidSyslogLwipSocketResolver_Create();
        }
    }
};

// clang-format on

TEST(SolidSyslogLwipSocketResolverPool, FillingPoolThenOverflowReturnsTheNullResolver)
{
    FillPool();

    overflow = SolidSyslogLwipSocketResolver_Create();

    POINTERS_EQUAL(SolidSyslogNullResolver_Get(), overflow);
}

TEST(SolidSyslogLwipSocketResolverPool, ExhaustedCreateReportsError)
{
    ErrorHandlerFake_Install(nullptr);
    FillPool();

    overflow = SolidSyslogLwipSocketResolver_Create();

    CHECK_ERROR_REPORTED_ONCE(
        SOLIDSYSLOG_SEVERITY_CRITICAL,
        &SolidSyslogLwipSocketResolverErrorSource,
        SOLIDSYSLOG_CAT_POOL_EXHAUSTED,
        SOLIDSYSLOG_RESOLVER_ERROR_POOL_EXHAUSTED
    );
}

TEST(SolidSyslogLwipSocketResolverPool, CreateAcquiresAndReleasesConfigLockOnFirstFreeSlot)
{
    ConfigLockFake_Install();

    pooled[0] = SolidSyslogLwipSocketResolver_Create();

    CALLED_FAKE(ConfigLockFake_Lock, ONCE);
    CALLED_FAKE(ConfigLockFake_Unlock, ONCE);
}

TEST(SolidSyslogLwipSocketResolverPool, CreateLocksOncePerSlotProbedWhenPoolIsFull)
{
    FillPool();
    ConfigLockFake_Install();

    overflow = SolidSyslogLwipSocketResolver_Create();

    LONGS_EQUAL(SOLIDSYSLOG_RESOLVER_POOL_SIZE, ConfigLockFake_LockCallCount());
    LONGS_EQUAL(SOLIDSYSLOG_RESOLVER_POOL_SIZE, ConfigLockFake_UnlockCallCount());
}

TEST(SolidSyslogLwipSocketResolverPool, DestroyOfPooledHandleLocksOnce)
{
    pooled[0] = SolidSyslogLwipSocketResolver_Create();
    ConfigLockFake_Install();

    SolidSyslogLwipSocketResolver_Destroy(pooled[0]);
    pooled[0] = nullptr;

    CALLED_FAKE(ConfigLockFake_Lock, ONCE);
    CALLED_FAKE(ConfigLockFake_Unlock, ONCE);
}

TEST(SolidSyslogLwipSocketResolverPool, DestroyOfUnknownHandleDoesNotLock)
{
    ConfigLockFake_Install();
    char stranger = 0;

    SolidSyslogLwipSocketResolver_Destroy((struct SolidSyslogResolver*) &stranger);

    CALLED_FAKE(ConfigLockFake_Lock, NEVER);
    CALLED_FAKE(ConfigLockFake_Unlock, NEVER);
}

TEST(SolidSyslogLwipSocketResolverPool, DestroyOfUnknownHandleReportsWarning)
{
    ErrorHandlerFake_Install(nullptr);
    char stranger = 0;

    SolidSyslogLwipSocketResolver_Destroy((struct SolidSyslogResolver*) &stranger);

    CHECK_ERROR_REPORTED_ONCE(
        SOLIDSYSLOG_SEVERITY_WARNING,
        &SolidSyslogLwipSocketResolverErrorSource,
        SOLIDSYSLOG_CAT_UNKNOWN_DESTROY,
        SOLIDSYSLOG_RESOLVER_ERROR_UNKNOWN_DESTROY
    );
}

TEST(SolidSyslogLwipSocketResolverPool, DestroyOfStaleHandleReportsWarning)
{
    pooled[0] = SolidSyslogLwipSocketResolver_Create();
    SolidSyslogLwipSocketResolver_Destroy(pooled[0]);
    ErrorHandlerFake_Install(nullptr);

    SolidSyslogLwipSocketResolver_Destroy(pooled[0]);
    pooled[0] = nullptr;

    CHECK_ERROR_REPORTED_ONCE(
        SOLIDSYSLOG_SEVERITY_WARNING,
        &SolidSyslogLwipSocketResolverErrorSource,
        SOLIDSYSLOG_CAT_UNKNOWN_DESTROY,
        SOLIDSYSLOG_RESOLVER_ERROR_UNKNOWN_DESTROY
    );
}

TEST(SolidSyslogLwipSocketResolverPool, ResolvingAfterDestroyIsASafeNoOp)
{
    struct SolidSyslogResolver* stale = SolidSyslogLwipSocketResolver_Create();
    struct SolidSyslogAddress* address = SolidSyslogLwipSocketAddress_Create();
    SolidSyslogLwipSocketResolver_Destroy(stale);
    LwipNetdbFake_Reset();

    bool resolved = SolidSyslogResolver_Resolve(stale, SOLIDSYSLOG_TRANSPORT_UDP, TEST_HOST, TEST_PORT, address);

    CHECK_FALSE(resolved);
    LONGS_EQUAL(0U, LwipNetdbFake_GetAddrInfoCallCount());
    SolidSyslogLwipSocketAddress_Destroy(address);
}
