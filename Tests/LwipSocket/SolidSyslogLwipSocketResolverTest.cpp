#include "TestUtils.h"
#include "CppUTest/TestHarness.h"

using namespace CososoTesting;

#include "lwip/sockets.h"

#include <cstdint>

#include "ErrorHandlerFake.h"
#include "LwipNetdbFake.h"
#include "SolidSyslogLwipSocketAddress.h"
#include "SolidSyslogLwipSocketAddressPrivate.h"
#include "SolidSyslogLwipSocketResolver.h"
#include "SolidSyslogLwipSocketResolverErrors.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogResolverCategories.h"
#include "SolidSyslogResolver.h"
#include "SolidSyslogTransport.h"

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
    LwipNetdbFake_SetIpv4Result("192.0.2.10");

    CHECK_TRUE(Resolve("collector.example.test", 514U));

    LONGS_EQUAL(AF_INET, Result()->sin_family);
    LONGS_EQUAL(lwip_htons(514U), Result()->sin_port);
    LONGS_EQUAL(lwip_htonl(0xC000020AU), Result()->sin_addr.s_addr);
}

TEST(SolidSyslogLwipSocketResolver, ResolveAsksForIpv4ByNameWithNoServiceName)
{
    CHECK_TRUE(Resolve("collector.example.test", 514U));

    LONGS_EQUAL(1U, LwipNetdbFake_GetAddrInfoCallCount());
    STRCMP_EQUAL("collector.example.test", LwipNetdbFake_LastNodename());
    POINTERS_EQUAL(nullptr, LwipNetdbFake_LastServname());
    LONGS_EQUAL(AF_INET, LwipNetdbFake_LastHintsFamily());
}

TEST(SolidSyslogLwipSocketResolver, ResolveFreesTheAnswerItWasGiven)
{
    CHECK_TRUE(Resolve("collector.example.test", 514U));

    LONGS_EQUAL(1U, LwipNetdbFake_FreeAddrInfoCallCount());
}

TEST(SolidSyslogLwipSocketResolver, AFailedLookupIsRefusedWithoutAnEvent)
{
    LwipNetdbFake_SetReturn(EAI_FAIL);

    CHECK_FALSE(Resolve("collector.example.test", 514U));

    CALLED_FAKE(ErrorHandlerFake_Handle, NEVER);
    LONGS_EQUAL(0U, LwipNetdbFake_FreeAddrInfoCallCount());
}

TEST(SolidSyslogLwipSocketResolver, AFailedLookupLeavesTheCallersAddressAlone)
{
    LwipNetdbFake_SetIpv4Result("192.0.2.10");
    CHECK_TRUE(Resolve("collector.example.test", 514U));
    LwipNetdbFake_SetReturn(EAI_FAIL);

    CHECK_FALSE(Resolve("elsewhere.example.test", 9999U));

    LONGS_EQUAL(lwip_htons(514U), Result()->sin_port);
    LONGS_EQUAL(lwip_htonl(0xC000020AU), Result()->sin_addr.s_addr);
}

TEST(SolidSyslogLwipSocketResolver, AnAnswerInAnotherFamilyIsRefusedAndReported)
{
    LwipNetdbFake_SetResultFamily(AF_INET6);

    CHECK_FALSE(Resolve("collector.example.test", 514U));

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

    CHECK_FALSE(Resolve("collector.example.test", 514U));

    LONGS_EQUAL(1U, LwipNetdbFake_FreeAddrInfoCallCount());
}

TEST(SolidSyslogLwipSocketResolver, AnAnswerInAnotherFamilyLeavesTheCallersAddressAlone)
{
    LwipNetdbFake_SetResultFamily(AF_INET6);

    CHECK_FALSE(Resolve("collector.example.test", 514U));

    LONGS_EQUAL(0U, Result()->sin_port);
    LONGS_EQUAL(0U, Result()->sin_addr.s_addr);
}

TEST(SolidSyslogLwipSocketResolver, AStackThatWillNotServeTheIpv4HintIsReported)
{
    LwipNetdbFake_SetReturn(EAI_FAMILY);

    CHECK_FALSE(Resolve("collector.example.test", 514U));

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

    CHECK_FALSE(Resolve("collector.example.test", 514U));

    LONGS_EQUAL(0U, LwipNetdbFake_FreeAddrInfoCallCount());
}
