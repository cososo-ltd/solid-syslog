#include "TestUtils.h"
#include "CppUTest/TestHarness.h"

using namespace CososoTesting;

#include <cstdint>
#include <string>

#include "ConfigLockFake.h"
#include "ErrorHandlerFake.h"
#include "LwipDnsFake.h"
#include "LwipFakeMarshalGuard.h"
#include "SolidSyslogErrorCategory.h"
#include "SolidSyslogLwipRawAddress.h"
#include "SolidSyslogLwipRawAddressPrivate.h"
#include "SolidSyslogLwipRawDnsResolver.h"
#include "SolidSyslogLwipRawDnsResolverErrors.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogResolver.h"
#include "SolidSyslogResolverCategories.h"
#include "SolidSyslogResolverDefinition.h"
#include "SolidSyslogTransport.h"
#include "SolidSyslogTunables.h"
#include "SolidSyslogResolverErrors.h"
#include "SolidSyslogError.h"
#include "lwip/err.h"
#include "lwip/ip_addr.h"

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

// Asserts the most recent ErrorHandlerFake call matched (severity, source, code).
#define CHECK_REPORTED(severity, source, expectedCategory, code) \
    CHECK_ERROR_REPORTED_ONCE((severity), &(source), (expectedCategory), (code))

static const char* const TEST_HOST = "syslog-ng";
static const uint16_t TEST_PORT = 514;

unsigned FakeSleep_CallCount = 0U;
uint32_t FakeSleep_LastMs = 0;

// When armed, FakeSleep fires the pending dns_found_callback the first time it
// is called with a callback outstanding - the host stand-in for the tcpip
// thread completing the lookup while the caller spins. Delivers &FakeSleep_FireIp
// when FakeSleep_FireWithAddr, else NULL (lookup failure).
bool FakeSleep_FireArmed = false;
bool FakeSleep_FireWithAddr = false;
ip_addr_t FakeSleep_FireIp;

void FakeSleep_Reset()
{
    FakeSleep_CallCount = 0;
    FakeSleep_LastMs = 0;
    FakeSleep_FireArmed = false;
    FakeSleep_FireWithAddr = false;
    FakeSleep_FireIp = ip_addr_t{};
}

extern "C" void FakeSleep(uint32_t milliseconds)
{
    FakeSleep_CallCount++;
    FakeSleep_LastMs = milliseconds;
    if (FakeSleep_FireArmed && LwipDnsFake_HasPendingCallback())
    {
        LwipDnsFake_FireCallback(FakeSleep_FireWithAddr ? &FakeSleep_FireIp : nullptr);
    }
}

// Counts marshal hops while keeping the MarshalGuard's active rail set around
// each callback (same contract as LwipFakeMarshalGuard_TrackingMarshal, so the
// teardown breach check still holds). Used to pin that the async-completion
// result is read under a SECOND marshal hop (DoPublishResult) rather than off
// the volatile Done flag on the caller's thread - the cross-thread data-race fix.
unsigned Marshal_CallCount = 0U;

extern "C" void CountingTrackingMarshal(SolidSyslogLwipRawCallback callback, void* context)
{
    Marshal_CallCount++;
    LwipFakeMarshalGuard_Active = true;
    callback(context);
    LwipFakeMarshalGuard_Active = false;
}

static ip_addr_t Address(const char* literal)
{
    ip_addr_t address{};
    ipaddr_aton(literal, &address);
    return address;
}

// Records whether the marshal rail was active when the error handler ran.
unsigned MarshalSpyingHandler_CallCount = 0U;
bool MarshalSpyingHandler_SawMarshalActive = false;

extern "C" void MarshalSpyingHandler(void* context, const struct SolidSyslogErrorEvent* event)
{
    (void) context;
    (void) event;
    MarshalSpyingHandler_CallCount++;
    MarshalSpyingHandler_SawMarshalActive = MarshalSpyingHandler_SawMarshalActive || LwipFakeMarshalGuard_Active;
}

static ip_addr_t Ipv6Loopback()
{
    return Address("::1");
}

// clang-format off
TEST_GROUP(SolidSyslogLwipRawDnsResolver)
{
    struct SolidSyslogLwipRawDnsResolverConfig config = {};
    struct SolidSyslogResolver* resolver = nullptr;
    struct SolidSyslogAddress*  addr     = nullptr;

    void setup() override
    {
        LwipDnsFake_Reset();
        FakeSleep_Reset();
        Marshal_CallCount = 0;
        MarshalSpyingHandler_CallCount = 0;
        MarshalSpyingHandler_SawMarshalActive = false;
        LwipFakeMarshalGuard_Reset();
        SolidSyslogLwipRaw_SetMarshal(CountingTrackingMarshal);
        config.Sleep = FakeSleep;
        resolver     = SolidSyslogLwipRawDnsResolver_Create(&config);
        addr         = SolidSyslogLwipRawAddress_Create();
    }

    void teardown() override
    {
        SolidSyslogLwipRawAddress_Destroy(addr);
        SolidSyslogLwipRawDnsResolver_Destroy(resolver);
        LwipFakeMarshalGuard_CheckNoBreach();
        SolidSyslogLwipRaw_SetMarshal(nullptr);
        SolidSyslog_SetErrorHandler(nullptr, nullptr);
    }

    bool Resolve(const char* host = TEST_HOST, uint16_t port = TEST_PORT, enum SolidSyslogTransport transport = SOLIDSYSLOG_TRANSPORT_UDP) const
    {
        return SolidSyslogResolver_Resolve(resolver, transport, host, port, addr);
    }

    // dns_gethostbyname answers at once with this address.
    static void ArrangeSynchronousAnswer(const ip_addr_t& answer)
    {
        LwipDnsFake_SetResult(ERR_OK);
        LwipDnsFake_SetResolvedIp(&answer);
    }

    // dns_gethostbyname queues the query; the first Sleep delivers this address.
    static void ArmAsyncAnswer(const ip_addr_t& answer)
    {
        LwipDnsFake_SetResult(ERR_INPROGRESS);
        FakeSleep_FireArmed = true;
        FakeSleep_FireWithAddr = true;
        FakeSleep_FireIp = answer;
    }
};

// clang-format on

TEST(SolidSyslogLwipRawDnsResolver, CreateSucceeds)
{
    CHECK(resolver != nullptr);
}

TEST(SolidSyslogLwipRawDnsResolver, ResolveReturnsTrueOnSynchronousHit)
{
    ArrangeSynchronousAnswer(Address("10.0.2.2"));

    CHECK_TRUE(Resolve());
}

TEST(SolidSyslogLwipRawDnsResolver, ResolvePopulatesIpFromSynchronousHit)
{
    ArrangeSynchronousAnswer(Address("10.0.2.2"));

    Resolve();

    STRCMP_EQUAL("10.0.2.2", ipaddr_ntoa(&SolidSyslogLwipRawAddress_AsConst(addr)->Ip));
}

TEST(SolidSyslogLwipRawDnsResolver, ResolvePopulatesPortFromArgOnSynchronousHit)
{
    ArrangeSynchronousAnswer(Address("10.0.2.2"));

    Resolve(TEST_HOST, 9999);

    LONGS_EQUAL(9999, SolidSyslogLwipRawAddress_AsConst(addr)->Port);
}

TEST(SolidSyslogLwipRawDnsResolver, ASynchronousIpv6AnswerIsRefused)
{
    ArrangeSynchronousAnswer(Ipv6Loopback());

    CHECK_FALSE(Resolve());
}

TEST(SolidSyslogLwipRawDnsResolver, ASynchronousIpv6AnswerIsReportedAsUnsupportedFamily)
{
    ErrorHandlerFake_Install(nullptr);
    ArrangeSynchronousAnswer(Ipv6Loopback());

    Resolve();

    CHECK_REPORTED(
        SOLIDSYSLOG_SEVERITY_ERROR,
        SolidSyslogLwipRawDnsResolverErrorSource,
        SOLIDSYSLOG_CAT_RESOLVER_RESOLVE_FAILED,
        SOLIDSYSLOG_RESOLVER_ERROR_ADDRESS_FAMILY_UNSUPPORTED
    );
}

TEST(SolidSyslogLwipRawDnsResolver, ASynchronousIpv6AnswerLeavesTheCallersAddressAlone)
{
    ArrangeSynchronousAnswer(Address("10.0.2.2"));
    Resolve(TEST_HOST, TEST_PORT);
    ArrangeSynchronousAnswer(Ipv6Loopback());

    Resolve(TEST_HOST, 9999);

    STRCMP_EQUAL("10.0.2.2", ipaddr_ntoa(&SolidSyslogLwipRawAddress_AsConst(addr)->Ip));
    LONGS_EQUAL(TEST_PORT, SolidSyslogLwipRawAddress_AsConst(addr)->Port);
}

TEST(SolidSyslogLwipRawDnsResolver, AnAsyncIpv6AnswerIsRefused)
{
    ArmAsyncAnswer(Ipv6Loopback());

    CHECK_FALSE(Resolve());
}

TEST(SolidSyslogLwipRawDnsResolver, AnAsyncIpv6AnswerIsReportedAsUnsupportedFamily)
{
    ErrorHandlerFake_Install(nullptr);
    ArmAsyncAnswer(Ipv6Loopback());

    Resolve();

    CHECK_REPORTED(
        SOLIDSYSLOG_SEVERITY_ERROR,
        SolidSyslogLwipRawDnsResolverErrorSource,
        SOLIDSYSLOG_CAT_RESOLVER_RESOLVE_FAILED,
        SOLIDSYSLOG_RESOLVER_ERROR_ADDRESS_FAMILY_UNSUPPORTED
    );
}

TEST(SolidSyslogLwipRawDnsResolver, AnAsyncIpv6AnswerIsReportedOutsideTheMarshalHop)
{
    // The error handler is the integrator's code: it is told on the caller's
    // thread once the publish hop has returned, never from inside it.
    SolidSyslog_SetErrorHandler(MarshalSpyingHandler, nullptr);
    ArmAsyncAnswer(Ipv6Loopback());

    Resolve();

    LONGS_EQUAL(1, MarshalSpyingHandler_CallCount);
    CHECK_FALSE(MarshalSpyingHandler_SawMarshalActive);
}

TEST(SolidSyslogLwipRawDnsResolver, ResolveDoesNotSpinOnSynchronousHit)
{
    LwipDnsFake_SetResult(ERR_OK);

    Resolve();

    LONGS_EQUAL(0, FakeSleep_CallCount);
}

TEST(SolidSyslogLwipRawDnsResolver, ResolvePassesHostToDnsGetHostByName)
{
    LwipDnsFake_SetResult(ERR_OK);

    Resolve("syslog-ng");

    STRCMP_EQUAL("syslog-ng", LwipDnsFake_LastHostname());
}

TEST(SolidSyslogLwipRawDnsResolver, ResolveReturnsTrueWhenAsyncCallbackDeliversAddress)
{
    ArmAsyncAnswer(Address("10.0.2.2"));

    CHECK_TRUE(Resolve());
}

TEST(SolidSyslogLwipRawDnsResolver, ResolvePopulatesIpFromAsyncCallback)
{
    ArmAsyncAnswer(Address("192.0.2.7"));

    Resolve();

    STRCMP_EQUAL("192.0.2.7", ipaddr_ntoa(&SolidSyslogLwipRawAddress_AsConst(addr)->Ip));
}

TEST(SolidSyslogLwipRawDnsResolver, ResolvePopulatesPortFromArgOnAsyncCallback)
{
    ArmAsyncAnswer(Address("10.0.2.2"));

    Resolve(TEST_HOST, 6514);

    LONGS_EQUAL(6514, SolidSyslogLwipRawAddress_AsConst(addr)->Port);
}

TEST(SolidSyslogLwipRawDnsResolver, ResolveSpinsUntilAsyncCallbackFires)
{
    ArmAsyncAnswer(Address("10.0.2.2"));

    Resolve();

    // Fired on the first sleep, so exactly one poll happened before completion.
    LONGS_EQUAL(1, FakeSleep_CallCount);
    UNSIGNED_LONGS_EQUAL(SOLIDSYSLOG_LWIP_RAW_DNS_RESOLVE_POLL_MS, FakeSleep_LastMs);
}

TEST(SolidSyslogLwipRawDnsResolver, SynchronousHitTakesExactlyOneMarshalHop)
{
    // ERR_OK publishes inline (read-after-marshal is ordered by the hop return),
    // so only the dns_gethostbyname hop runs - no separate publish hop.
    LwipDnsFake_SetResult(ERR_OK);

    Resolve();

    LONGS_EQUAL(1, Marshal_CallCount);
}

TEST(SolidSyslogLwipRawDnsResolver, AsyncSuccessReadsResultUnderASecondMarshalHop)
{
    // The cross-thread fix: after the spin observes the volatile Done flag, the
    // non-volatile ResolvedIp / ResolvedOk are read back on the lwIP thread via a
    // SECOND marshal hop (DoPublishResult) - never off the flag on the caller's
    // thread. Two hops total: dns_gethostbyname + publish.
    ArmAsyncAnswer(Address("10.0.2.2"));

    Resolve();

    LONGS_EQUAL(2, Marshal_CallCount);
}

TEST(SolidSyslogLwipRawDnsResolver, TimeoutDoesNotTakeThePublishHop)
{
    // No completion -> no publish hop; only the dns_gethostbyname hop ran.
    LwipDnsFake_SetResult(ERR_INPROGRESS); // callback never fires

    Resolve();

    LONGS_EQUAL(1, Marshal_CallCount);
}

TEST(SolidSyslogLwipRawDnsResolver, ResolveReturnsFalseWhenAsyncCallbackDeliversNull)
{
    LwipDnsFake_SetResult(ERR_INPROGRESS);
    FakeSleep_FireArmed = true;
    FakeSleep_FireWithAddr = false; // deliver NULL - lookup failed

    CHECK_FALSE(Resolve());
}

TEST(SolidSyslogLwipRawDnsResolver, ResolveReturnsFalseOnTimeout)
{
    LwipDnsFake_SetResult(ERR_INPROGRESS); // callback never fires

    CHECK_FALSE(Resolve());
}

TEST(SolidSyslogLwipRawDnsResolver, ResolveSleepsUntilDeadlineOnTimeout)
{
    LwipDnsFake_SetResult(ERR_INPROGRESS); // callback never fires

    Resolve();

    LONGS_EQUAL(SOLIDSYSLOG_DNS_RESOLVE_TIMEOUT_MS / SOLIDSYSLOG_LWIP_RAW_DNS_RESOLVE_POLL_MS, FakeSleep_CallCount);
    UNSIGNED_LONGS_EQUAL(SOLIDSYSLOG_LWIP_RAW_DNS_RESOLVE_POLL_MS, FakeSleep_LastMs);
}

TEST(SolidSyslogLwipRawDnsResolver, ResolveReportsWarningOnTimeout)
{
    ErrorHandlerFake_Install(nullptr);
    LwipDnsFake_SetResult(ERR_INPROGRESS); // callback never fires

    Resolve();

    CHECK_REPORTED(
        SOLIDSYSLOG_SEVERITY_WARNING,
        SolidSyslogLwipRawDnsResolverErrorSource,
        SOLIDSYSLOG_CAT_RESOLVER_RESOLVE_FAILED,
        SOLIDSYSLOG_RESOLVER_ERROR_RESOLVE_TIMEOUT
    );
}

TEST(SolidSyslogLwipRawDnsResolver, ResolveReturnsFalseOnErrArg)
{
    LwipDnsFake_SetResult(ERR_ARG);

    CHECK_FALSE(Resolve());
}

TEST(SolidSyslogLwipRawDnsResolver, ResolveDoesNotSpinOnErrArg)
{
    LwipDnsFake_SetResult(ERR_ARG);

    Resolve();

    LONGS_EQUAL(0, FakeSleep_CallCount);
}

TEST(SolidSyslogLwipRawDnsResolver, ResolveInvokesDnsGetHostByNameUnderMarshal)
{
    // The standing LwipFakeMarshalGuard (installed in setup, checked in
    // teardown) fails the test if dns_gethostbyname ran outside the marshal;
    // this asserts the call actually happened, so the guard has something to
    // vouch for. dns_gethostbyname touches lwIP core state and MUST be marshalled
    // (unlike the numeric resolver's pure ipaddr_aton parse).
    LwipDnsFake_SetResult(ERR_OK);

    Resolve();

    LONGS_EQUAL(1, LwipDnsFake_GetHostByNameCallCount());
}

TEST(SolidSyslogLwipRawDnsResolver, UdpTransportResolvesIdenticallyToTcp)
{
    // Locks in that the DNS resolver does not dispatch on transport - a future
    // reader must not add transport-typed lookup behaviour here.
    ArrangeSynchronousAnswer(Address("10.0.2.2"));

    Resolve(TEST_HOST, TEST_PORT, SOLIDSYSLOG_TRANSPORT_UDP);
    std::string udpIp = ipaddr_ntoa(&SolidSyslogLwipRawAddress_AsConst(addr)->Ip);
    uint16_t udpPort = SolidSyslogLwipRawAddress_AsConst(addr)->Port;

    Resolve(TEST_HOST, TEST_PORT, SOLIDSYSLOG_TRANSPORT_TCP);

    STRCMP_EQUAL(udpIp.c_str(), ipaddr_ntoa(&SolidSyslogLwipRawAddress_AsConst(addr)->Ip));
    LONGS_EQUAL(udpPort, SolidSyslogLwipRawAddress_AsConst(addr)->Port);
}

TEST(SolidSyslogLwipRawDnsResolver, ResolveAcceptsNumericLiteralAsSynchronousHit)
{
    // Superset of the numeric resolver: a dotted-quad is handed to
    // dns_gethostbyname, which resolves it synchronously (ERR_OK) - so numeric
    // hosts still resolve through this class. Here the fake stands in for that
    // ERR_OK return; the contract under test is that the literal host string
    // flows through unchanged and the resolve succeeds.
    ArrangeSynchronousAnswer(Address("10.0.2.2"));

    CHECK_TRUE(Resolve("10.0.2.2"));
    STRCMP_EQUAL("10.0.2.2", LwipDnsFake_LastHostname());
}

// clang-format off
TEST_GROUP(SolidSyslogLwipRawDnsResolverPool)
{
    struct SolidSyslogLwipRawDnsResolverConfig config = {};
    struct SolidSyslogResolver* pooled[SOLIDSYSLOG_RESOLVER_POOL_SIZE] = {};
    struct SolidSyslogResolver* overflow                                            = nullptr;

    void setup() override
    {
        config.Sleep = FakeSleep;
    }

    void teardown() override
    {
        for (auto* handle : pooled)
        {
            if (handle != nullptr)
            {
                SolidSyslogLwipRawDnsResolver_Destroy(handle);
            }
        }
        if (overflow != nullptr)
        {
            SolidSyslogLwipRawDnsResolver_Destroy(overflow);
        }
        ConfigLockFake_Uninstall();
    }

    void FillPool()
    {
        for (auto*& slot : pooled)
        {
            slot = SolidSyslogLwipRawDnsResolver_Create(&config);
        }
    }
};

// clang-format on

TEST(SolidSyslogLwipRawDnsResolverPool, CreateWithNullConfigReturnsFallback)
{
    FillPool();

    overflow = SolidSyslogLwipRawDnsResolver_Create(nullptr);

    CHECK_IS_FALLBACK(overflow, pooled);
}

TEST(SolidSyslogLwipRawDnsResolverPool, CreateWithNullSleepReturnsFallback)
{
    FillPool();
    struct SolidSyslogLwipRawDnsResolverConfig badConfig = {};
    badConfig.Sleep = nullptr;

    overflow = SolidSyslogLwipRawDnsResolver_Create(&badConfig);

    CHECK_IS_FALLBACK(overflow, pooled);
}

TEST(SolidSyslogLwipRawDnsResolverPool, FillingPoolThenOverflowReturnsDistinctFallback)
{
    FillPool();

    overflow = SolidSyslogLwipRawDnsResolver_Create(&config);

    CHECK_IS_FALLBACK(overflow, pooled);
}

TEST(SolidSyslogLwipRawDnsResolverPool, ExhaustedCreateReportsError)
{
    ErrorHandlerFake_Install(nullptr);
    FillPool();

    overflow = SolidSyslogLwipRawDnsResolver_Create(&config);

    CHECK_REPORTED(
        SOLIDSYSLOG_SEVERITY_CRITICAL,
        SolidSyslogLwipRawDnsResolverErrorSource,
        SOLIDSYSLOG_CAT_POOL_EXHAUSTED,
        SOLIDSYSLOG_RESOLVER_ERROR_POOL_EXHAUSTED
    );
}

TEST(SolidSyslogLwipRawDnsResolverPool, CreateWithNullConfigReportsError)
{
    ErrorHandlerFake_Install(nullptr);

    SolidSyslogLwipRawDnsResolver_Create(nullptr);

    CHECK_REPORTED(
        SOLIDSYSLOG_SEVERITY_CRITICAL,
        SolidSyslogLwipRawDnsResolverErrorSource,
        SOLIDSYSLOG_CAT_BAD_CONFIG,
        SOLIDSYSLOG_RESOLVER_ERROR_NULL_CONFIG
    );
}

TEST(SolidSyslogLwipRawDnsResolverPool, CreateWithNullSleepReportsError)
{
    ErrorHandlerFake_Install(nullptr);
    struct SolidSyslogLwipRawDnsResolverConfig badConfig = {};
    badConfig.Sleep = nullptr;

    SolidSyslogLwipRawDnsResolver_Create(&badConfig);

    CHECK_REPORTED(
        SOLIDSYSLOG_SEVERITY_CRITICAL,
        SolidSyslogLwipRawDnsResolverErrorSource,
        SOLIDSYSLOG_CAT_BAD_CONFIG,
        SOLIDSYSLOG_RESOLVER_ERROR_NULL_SLEEP
    );
}

TEST(SolidSyslogLwipRawDnsResolverPool, FallbackResolveReturnsFalse)
{
    FillPool();
    overflow = SolidSyslogLwipRawDnsResolver_Create(&config);
    struct SolidSyslogAddress* fallbackResult = SolidSyslogLwipRawAddress_Create();

    CHECK_FALSE(SolidSyslogResolver_Resolve(overflow, SOLIDSYSLOG_TRANSPORT_UDP, TEST_HOST, TEST_PORT, fallbackResult));

    SolidSyslogLwipRawAddress_Destroy(fallbackResult);
}

TEST(SolidSyslogLwipRawDnsResolverPool, CreateAcquiresAndReleasesConfigLockOnFirstFreeSlot)
{
    ConfigLockFake_Install();

    pooled[0] = SolidSyslogLwipRawDnsResolver_Create(&config);

    CALLED_FAKE(ConfigLockFake_Lock, ONCE);
    CALLED_FAKE(ConfigLockFake_Unlock, ONCE);
}

TEST(SolidSyslogLwipRawDnsResolverPool, CreateLocksOncePerSlotProbedWhenPoolIsFull)
{
    FillPool();
    ConfigLockFake_Install();

    overflow = SolidSyslogLwipRawDnsResolver_Create(&config);

    LONGS_EQUAL(SOLIDSYSLOG_RESOLVER_POOL_SIZE, ConfigLockFake_LockCallCount());
    LONGS_EQUAL(SOLIDSYSLOG_RESOLVER_POOL_SIZE, ConfigLockFake_UnlockCallCount());
}

TEST(SolidSyslogLwipRawDnsResolverPool, DestroyOfPooledHandleLocksOnce)
{
    pooled[0] = SolidSyslogLwipRawDnsResolver_Create(&config);
    ConfigLockFake_Install();

    SolidSyslogLwipRawDnsResolver_Destroy(pooled[0]);
    pooled[0] = nullptr;

    CALLED_FAKE(ConfigLockFake_Lock, ONCE);
    CALLED_FAKE(ConfigLockFake_Unlock, ONCE);
}

TEST(SolidSyslogLwipRawDnsResolverPool, DestroyOfUnknownHandleDoesNotLock)
{
    ConfigLockFake_Install();
    struct SolidSyslogResolver stranger = {};

    SolidSyslogLwipRawDnsResolver_Destroy(&stranger);

    CALLED_FAKE(ConfigLockFake_Lock, NEVER);
    CALLED_FAKE(ConfigLockFake_Unlock, NEVER);
}

TEST(SolidSyslogLwipRawDnsResolverPool, DestroyOfUnknownHandleReportsWarning)
{
    ErrorHandlerFake_Install(nullptr);
    struct SolidSyslogResolver stranger = {};

    SolidSyslogLwipRawDnsResolver_Destroy(&stranger);

    CHECK_REPORTED(
        SOLIDSYSLOG_SEVERITY_WARNING,
        SolidSyslogLwipRawDnsResolverErrorSource,
        SOLIDSYSLOG_CAT_UNKNOWN_DESTROY,
        SOLIDSYSLOG_RESOLVER_ERROR_UNKNOWN_DESTROY
    );
}

TEST(SolidSyslogLwipRawDnsResolverPool, DestroyOfStaleHandleReportsWarning)
{
    pooled[0] = SolidSyslogLwipRawDnsResolver_Create(&config);
    SolidSyslogLwipRawDnsResolver_Destroy(pooled[0]);
    ErrorHandlerFake_Install(nullptr);

    SolidSyslogLwipRawDnsResolver_Destroy(pooled[0]);
    pooled[0] = nullptr;

    CHECK_REPORTED(
        SOLIDSYSLOG_SEVERITY_WARNING,
        SolidSyslogLwipRawDnsResolverErrorSource,
        SOLIDSYSLOG_CAT_UNKNOWN_DESTROY,
        SOLIDSYSLOG_RESOLVER_ERROR_UNKNOWN_DESTROY
    );
}

TEST(SolidSyslogLwipRawDnsResolver, TheErrorSourceNamesItsDetailsThroughResolverErrors)
{
    CHECK(SolidSyslogResolverErrors_Name == SolidSyslogLwipRawDnsResolverErrorSource.DetailName);
}
