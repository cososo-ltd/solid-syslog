#include "TestUtils.h"
#include "CppUTest/TestHarness.h"

using namespace CososoTesting;

#include "vxWorks.h"

#include <netinet/in.h>
#include <sys/socket.h>

#include "ConfigLockFake.h"
#include "ErrorHandlerFake.h"
#include "SolidSyslogAddress.h"
#include "SolidSyslogErrorCategory.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogTunables.h"
#include "SolidSyslogResolver.h"
#include "SolidSyslogResolverDefinition.h"
#include "SolidSyslogTransport.h"
#include "SolidSyslogVxWorks64Address.h"
#include "SolidSyslogVxWorks64AddressPrivate.h"
#include "SolidSyslogVxWorks64Resolver.h"
#include "SolidSyslogVxWorks64ResolverErrors.h"
#include "VxWorks64NetFake.h"

// Asserts handle is non-null and not one of the slots in pool.
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
TEST_GROUP(SolidSyslogVxWorks64Resolver)
{
    struct SolidSyslogResolver* resolver = nullptr;
    struct SolidSyslogAddress* address   = nullptr;

    void setup() override
    {
        VxWorks64NetFake_Reset();
        resolver = SolidSyslogVxWorks64Resolver_Create();
        address  = SolidSyslogVxWorks64Address_Create();
    }

    bool Resolve(const char* host)
    {
        return SolidSyslogResolver_Resolve(resolver, SOLIDSYSLOG_TRANSPORT_UDP, host, 514U, address);
    }

    const struct sockaddr_in* Resolved() const
    {
        return SolidSyslogVxWorks64Address_AsConstSockaddrIn(address);
    }

    void teardown() override
    {
        SolidSyslogVxWorks64Address_Destroy(address);
        SolidSyslogVxWorks64Resolver_Destroy(resolver);
    }
};

// clang-format on

TEST(SolidSyslogVxWorks64Resolver, ResolvesADottedLiteral)
{
    VxWorks64NetFake_SetInetAddrReturn(0x0100007FUL);

    CHECK_TRUE(SolidSyslogResolver_Resolve(resolver, SOLIDSYSLOG_TRANSPORT_UDP, "127.0.0.1", 514U, address));
}

TEST(SolidSyslogVxWorks64Resolver, PopulatesAddressFamily)
{
    VxWorks64NetFake_SetInetAddrReturn(0x0100007FUL);

    Resolve("127.0.0.1");

    LONGS_EQUAL(AF_INET, Resolved()->sin_family);
}

TEST(SolidSyslogVxWorks64Resolver, PopulatesTheAddressInetAddrParsed)
{
    VxWorks64NetFake_SetInetAddrReturn(0x0100007FUL);

    Resolve("127.0.0.1");

    LONGS_EQUAL(0x0100007FUL, Resolved()->sin_addr.s_addr);
}

TEST(SolidSyslogVxWorks64Resolver, PopulatesThePortInNetworkOrder)
{
    VxWorks64NetFake_SetInetAddrReturn(0x0100007FUL);

    // 6514 rather than 514, whose bytes are equal and so read the same in
    // either order.
    SolidSyslogResolver_Resolve(resolver, SOLIDSYSLOG_TRANSPORT_UDP, "127.0.0.1", 6514U, address);

    LONGS_EQUAL(htons(6514U), Resolved()->sin_port);
}

TEST(SolidSyslogVxWorks64Resolver, ANameIsLookedUpInTheHostLibrary)
{
    VxWorks64NetFake_SetInetAddrReturn((unsigned long) ERROR);
    VxWorks64NetFake_SetHostGetByNameReturn(0x0A00000A);

    Resolve("collector");

    CALLED_FAKE(VxWorks64NetFake_HostGetByName, ONCE);
}

TEST(SolidSyslogVxWorks64Resolver, ANameResolvesToTheAddressTheHostLibraryFound)
{
    VxWorks64NetFake_SetInetAddrReturn((unsigned long) ERROR);
    VxWorks64NetFake_SetHostGetByNameReturn(0x0A00000A);

    Resolve("collector");

    LONGS_EQUAL(0x0A00000AUL, Resolved()->sin_addr.s_addr);
}

TEST(SolidSyslogVxWorks64Resolver, ReturnsFalseWhenTheHostLibraryCannotResolveTheName)
{
    VxWorks64NetFake_SetInetAddrReturn((unsigned long) ERROR);
    VxWorks64NetFake_SetHostGetByNameReturn(ERROR);

    CHECK_FALSE(Resolve("nowhere"));
}

// The Resolver contract: the result is written only on a true return, so a
// failed lookup leaves whatever the caller last resolved in place.
TEST(SolidSyslogVxWorks64Resolver, AFailedLookupLeavesTheAddressUntouched)
{
    VxWorks64NetFake_SetInetAddrReturn(0x0100007FUL);
    Resolve("127.0.0.1");
    VxWorks64NetFake_SetInetAddrReturn((unsigned long) ERROR);
    VxWorks64NetFake_SetHostGetByNameReturn(ERROR);

    Resolve("nowhere");

    LONGS_EQUAL(0x0100007FUL, Resolved()->sin_addr.s_addr);
}

TEST(SolidSyslogVxWorks64Resolver, ResolveAfterDestroyFails)
{
    VxWorks64NetFake_SetInetAddrReturn(0x0100007FUL);
    struct SolidSyslogResolver* stale = resolver;
    SolidSyslogVxWorks64Resolver_Destroy(resolver);
    resolver = nullptr;

    CHECK_FALSE(SolidSyslogResolver_Resolve(stale, SOLIDSYSLOG_TRANSPORT_UDP, "127.0.0.1", 514U, address));
}

TEST(SolidSyslogVxWorks64Resolver, TheLiteralIsTheHostArgument)
{
    VxWorks64NetFake_SetInetAddrReturn(0x0100007FUL);

    Resolve("127.0.0.1");

    STRCMP_EQUAL("127.0.0.1", VxWorks64NetFake_LastInetAddrString());
}

TEST(SolidSyslogVxWorks64Resolver, TheNameLookedUpIsTheHostArgument)
{
    VxWorks64NetFake_SetInetAddrReturn((unsigned long) ERROR);
    VxWorks64NetFake_SetHostGetByNameReturn(0x0A00000A);

    Resolve("collector");

    STRCMP_EQUAL("collector", VxWorks64NetFake_LastHostGetByNameName());
}

TEST(SolidSyslogVxWorks64Resolver, ALiteralNeverReachesTheHostLibrary)
{
    VxWorks64NetFake_SetInetAddrReturn(0x0100007FUL);

    Resolve("127.0.0.1");

    CALLED_FAKE(VxWorks64NetFake_HostGetByName, NEVER);
}

// clang-format off
TEST_GROUP(SolidSyslogVxWorks64ResolverPool)
{
    struct SolidSyslogResolver* pooled[SOLIDSYSLOG_RESOLVER_POOL_SIZE] = {};
    struct SolidSyslogResolver* overflow                                            = nullptr;

    void teardown() override
    {
        for (auto* handle : pooled)
        {
            if (handle != nullptr)
            {
                SolidSyslogVxWorks64Resolver_Destroy(handle);
            }
        }
        if (overflow != nullptr)
        {
            SolidSyslogVxWorks64Resolver_Destroy(overflow);
        }
        ConfigLockFake_Uninstall();
    }

    void FillPool()
    {
        for (auto*& slot : pooled)
        {
            slot = SolidSyslogVxWorks64Resolver_Create();
        }
    }
};

// clang-format on

TEST(SolidSyslogVxWorks64ResolverPool, FillingPoolThenOverflowReturnsDistinctFallback)
{
    FillPool();

    overflow = SolidSyslogVxWorks64Resolver_Create();

    CHECK_IS_FALLBACK(overflow, pooled);
}

TEST(SolidSyslogVxWorks64ResolverPool, ExhaustedCreateReportsError)
{
    ErrorHandlerFake_Install(nullptr);
    FillPool();

    overflow = SolidSyslogVxWorks64Resolver_Create();

    CHECK_ERROR_REPORTED_ONCE(
        SOLIDSYSLOG_SEVERITY_CRITICAL,
        &SolidSyslogVxWorks64ResolverErrorSource,
        SOLIDSYSLOG_CAT_POOL_EXHAUSTED,
        SOLIDSYSLOG_RESOLVER_ERROR_POOL_EXHAUSTED
    );
}

TEST(SolidSyslogVxWorks64ResolverPool, FallbackResolveReturnsFalse)
{
    FillPool();
    overflow = SolidSyslogVxWorks64Resolver_Create();

    struct SolidSyslogAddress* fallbackResult = SolidSyslogVxWorks64Address_Create();
    CHECK_FALSE(SolidSyslogResolver_Resolve(overflow, SOLIDSYSLOG_TRANSPORT_UDP, "127.0.0.1", 514U, fallbackResult));
    SolidSyslogVxWorks64Address_Destroy(fallbackResult);
}

TEST(SolidSyslogVxWorks64ResolverPool, CreateAcquiresAndReleasesConfigLockOnFirstFreeSlot)
{
    ConfigLockFake_Install();

    pooled[0] = SolidSyslogVxWorks64Resolver_Create();

    CALLED_FAKE(ConfigLockFake_Lock, ONCE);
    CALLED_FAKE(ConfigLockFake_Unlock, ONCE);
}

TEST(SolidSyslogVxWorks64ResolverPool, CreateLocksOncePerSlotProbedWhenPoolIsFull)
{
    FillPool();
    ConfigLockFake_Install();

    overflow = SolidSyslogVxWorks64Resolver_Create();

    LONGS_EQUAL(SOLIDSYSLOG_RESOLVER_POOL_SIZE, ConfigLockFake_LockCallCount());
    LONGS_EQUAL(SOLIDSYSLOG_RESOLVER_POOL_SIZE, ConfigLockFake_UnlockCallCount());
}

TEST(SolidSyslogVxWorks64ResolverPool, DestroyOfPooledHandleLocksOnce)
{
    pooled[0] = SolidSyslogVxWorks64Resolver_Create();
    ConfigLockFake_Install();

    SolidSyslogVxWorks64Resolver_Destroy(pooled[0]);
    pooled[0] = nullptr;

    CALLED_FAKE(ConfigLockFake_Lock, ONCE);
    CALLED_FAKE(ConfigLockFake_Unlock, ONCE);
}

TEST(SolidSyslogVxWorks64ResolverPool, DestroyOfUnknownHandleDoesNotLock)
{
    ConfigLockFake_Install();
    struct SolidSyslogResolver stranger = {};

    SolidSyslogVxWorks64Resolver_Destroy(&stranger);

    CALLED_FAKE(ConfigLockFake_Lock, NEVER);
    CALLED_FAKE(ConfigLockFake_Unlock, NEVER);
}

TEST(SolidSyslogVxWorks64ResolverPool, DestroyOfUnknownHandleReportsWarning)
{
    ErrorHandlerFake_Install(nullptr);
    struct SolidSyslogResolver stranger = {};

    SolidSyslogVxWorks64Resolver_Destroy(&stranger);

    CHECK_ERROR_REPORTED_ONCE(
        SOLIDSYSLOG_SEVERITY_WARNING,
        &SolidSyslogVxWorks64ResolverErrorSource,
        SOLIDSYSLOG_CAT_UNKNOWN_DESTROY,
        SOLIDSYSLOG_RESOLVER_ERROR_UNKNOWN_DESTROY
    );
}

TEST(SolidSyslogVxWorks64ResolverPool, DestroyOfStaleHandleReportsWarning)
{
    pooled[0] = SolidSyslogVxWorks64Resolver_Create();
    SolidSyslogVxWorks64Resolver_Destroy(pooled[0]);
    ErrorHandlerFake_Install(nullptr);

    SolidSyslogVxWorks64Resolver_Destroy(pooled[0]);
    pooled[0] = nullptr;

    CHECK_ERROR_REPORTED_ONCE(
        SOLIDSYSLOG_SEVERITY_WARNING,
        &SolidSyslogVxWorks64ResolverErrorSource,
        SOLIDSYSLOG_CAT_UNKNOWN_DESTROY,
        SOLIDSYSLOG_RESOLVER_ERROR_UNKNOWN_DESTROY
    );
}
