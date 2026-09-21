#include "TestUtils.h"
#include "CppUTest/TestHarness.h"

using namespace CososoTesting;

#include "lwip/sockets.h"

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

#include "ConfigLockFake.h"
#include "ErrorHandlerFake.h"
#include "SolidSyslogErrorCategory.h"
#include "SolidSyslogLwipSocketAddress.h"
#include "SolidSyslogLwipSocketAddressErrors.h"
#include "SolidSyslogLwipSocketAddressPrivate.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogTunables.h"

// clang-format off
TEST_GROUP(SolidSyslogLwipSocketAddress)
{
    struct SolidSyslogAddress* address = nullptr;

    void setup() override
    {
        address = SolidSyslogLwipSocketAddress_Create();
    }

    void teardown() override
    {
        SolidSyslogLwipSocketAddress_Destroy(address);
    }
};

// clang-format on

TEST(SolidSyslogLwipSocketAddress, CreateReturnsNonNull)
{
    CHECK(address != nullptr);
}

TEST(SolidSyslogLwipSocketAddress, AsSockaddrInRoundTripsBytes)
{
    struct sockaddr_in expected = {};
    expected.sin_family = AF_INET;
    expected.sin_port = lwip_htons(514U);
    expected.sin_addr.s_addr = lwip_htonl(0x7F000001U);

    *SolidSyslogLwipSocketAddress_AsSockaddrIn(address) = expected;

    const struct sockaddr_in* actual = SolidSyslogLwipSocketAddress_AsConstSockaddrIn(address);
    MEMCMP_EQUAL(&expected, actual, sizeof(expected));
}

TEST(SolidSyslogLwipSocketAddress, CreateZeroesTheSockaddrFromAnyPriorSlotContents)
{
    struct sockaddr_in dirty = {};
    dirty.sin_family = AF_INET;
    dirty.sin_port = lwip_htons(9999U);
    dirty.sin_addr.s_addr = lwip_htonl(0xDEADBEEFU);
    *SolidSyslogLwipSocketAddress_AsSockaddrIn(address) = dirty;
    SolidSyslogLwipSocketAddress_Destroy(address);

    address = SolidSyslogLwipSocketAddress_Create();

    struct sockaddr_in zeroes = {};
    MEMCMP_EQUAL(&zeroes, SolidSyslogLwipSocketAddress_AsConstSockaddrIn(address), sizeof(zeroes));
}

// clang-format off
TEST_GROUP(SolidSyslogLwipSocketAddressPool)
{
    struct SolidSyslogAddress* pooled[SOLIDSYSLOG_ADDRESS_POOL_SIZE] = {};
    struct SolidSyslogAddress* overflow                              = nullptr;

    void teardown() override
    {
        for (auto* handle : pooled)
        {
            if (handle != nullptr)
            {
                SolidSyslogLwipSocketAddress_Destroy(handle);
            }
        }
        if (overflow != nullptr)
        {
            SolidSyslogLwipSocketAddress_Destroy(overflow);
        }
        ConfigLockFake_Uninstall();
    }

    void FillPool()
    {
        for (auto*& slot : pooled)
        {
            slot = SolidSyslogLwipSocketAddress_Create();
        }
    }
};

// clang-format on

TEST(SolidSyslogLwipSocketAddressPool, FillingPoolThenOverflowReturnsDistinctFallback)
{
    FillPool();

    overflow = SolidSyslogLwipSocketAddress_Create();

    CHECK_IS_FALLBACK(overflow, pooled);
}

TEST(SolidSyslogLwipSocketAddressPool, ExhaustedCreateReportsError)
{
    ErrorHandlerFake_Install(nullptr);
    FillPool();

    overflow = SolidSyslogLwipSocketAddress_Create();

    CHECK_ERROR_REPORTED_ONCE(
        SOLIDSYSLOG_SEVERITY_CRITICAL,
        &SolidSyslogLwipSocketAddressErrorSource,
        SOLIDSYSLOG_CAT_POOL_EXHAUSTED,
        SOLIDSYSLOG_ADDRESS_ERROR_POOL_EXHAUSTED
    );
}

TEST(SolidSyslogLwipSocketAddressPool, CreateAcquiresAndReleasesConfigLockOnFirstFreeSlot)
{
    ConfigLockFake_Install();

    pooled[0] = SolidSyslogLwipSocketAddress_Create();

    CALLED_FAKE(ConfigLockFake_Lock, ONCE);
    CALLED_FAKE(ConfigLockFake_Unlock, ONCE);
}

TEST(SolidSyslogLwipSocketAddressPool, CreateLocksOncePerSlotProbedWhenPoolIsFull)
{
    FillPool();
    ConfigLockFake_Install();

    overflow = SolidSyslogLwipSocketAddress_Create();

    LONGS_EQUAL(SOLIDSYSLOG_ADDRESS_POOL_SIZE, ConfigLockFake_LockCallCount());
    LONGS_EQUAL(SOLIDSYSLOG_ADDRESS_POOL_SIZE, ConfigLockFake_UnlockCallCount());
}

TEST(SolidSyslogLwipSocketAddressPool, DestroyOfPooledHandleLocksOnce)
{
    pooled[0] = SolidSyslogLwipSocketAddress_Create();
    ConfigLockFake_Install();

    SolidSyslogLwipSocketAddress_Destroy(pooled[0]);
    pooled[0] = nullptr;

    CALLED_FAKE(ConfigLockFake_Lock, ONCE);
    CALLED_FAKE(ConfigLockFake_Unlock, ONCE);
}

TEST(SolidSyslogLwipSocketAddressPool, DestroyOfUnknownHandleDoesNotLock)
{
    ConfigLockFake_Install();
    char stranger = 0;

    SolidSyslogLwipSocketAddress_Destroy((struct SolidSyslogAddress*) &stranger);

    CALLED_FAKE(ConfigLockFake_Lock, NEVER);
    CALLED_FAKE(ConfigLockFake_Unlock, NEVER);
}

TEST(SolidSyslogLwipSocketAddressPool, DestroyOfUnknownHandleReportsWarning)
{
    ErrorHandlerFake_Install(nullptr);
    char stranger = 0;

    SolidSyslogLwipSocketAddress_Destroy((struct SolidSyslogAddress*) &stranger);

    CHECK_ERROR_REPORTED_ONCE(
        SOLIDSYSLOG_SEVERITY_WARNING,
        &SolidSyslogLwipSocketAddressErrorSource,
        SOLIDSYSLOG_CAT_UNKNOWN_DESTROY,
        SOLIDSYSLOG_ADDRESS_ERROR_UNKNOWN_DESTROY
    );
}

TEST(SolidSyslogLwipSocketAddressPool, DestroyOfStaleHandleReportsWarning)
{
    pooled[0] = SolidSyslogLwipSocketAddress_Create();
    SolidSyslogLwipSocketAddress_Destroy(pooled[0]);
    ErrorHandlerFake_Install(nullptr);

    SolidSyslogLwipSocketAddress_Destroy(pooled[0]);
    pooled[0] = nullptr;

    CHECK_ERROR_REPORTED_ONCE(
        SOLIDSYSLOG_SEVERITY_WARNING,
        &SolidSyslogLwipSocketAddressErrorSource,
        SOLIDSYSLOG_CAT_UNKNOWN_DESTROY,
        SOLIDSYSLOG_ADDRESS_ERROR_UNKNOWN_DESTROY
    );
}
