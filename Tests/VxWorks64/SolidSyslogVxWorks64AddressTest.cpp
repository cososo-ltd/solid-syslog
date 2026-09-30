#include "TestUtils.h"
#include "CppUTest/TestHarness.h"

using namespace CososoTesting;

#include "vxWorks.h"

#include <netinet/in.h>
#include <sys/socket.h>

#include "ConfigLockFake.h"
#include "ErrorHandlerFake.h"
#include "SolidSyslogErrorCategory.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogTunables.h"
#include "SolidSyslogVxWorks64Address.h"
#include "SolidSyslogVxWorks64AddressErrors.h"
#include "SolidSyslogVxWorks64AddressPrivate.h"

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
TEST_GROUP(SolidSyslogVxWorks64Address)
{
    struct SolidSyslogAddress* address = nullptr;

    void setup() override
    {
        address = SolidSyslogVxWorks64Address_Create();
    }

    void teardown() override
    {
        SolidSyslogVxWorks64Address_Destroy(address);
    }
};

// clang-format on

TEST(SolidSyslogVxWorks64Address, CreateReturnsNonNull)
{
    CHECK(address != nullptr);
}

TEST(SolidSyslogVxWorks64Address, AsSockaddrInRoundTripsBytes)
{
    struct sockaddr_in expected = {};
    expected.sin_family = AF_INET;
    expected.sin_port = htons(514U);
    expected.sin_addr.s_addr = htonl(0x7F000001U);

    *SolidSyslogVxWorks64Address_AsSockaddrIn(address) = expected;

    const struct sockaddr_in* actual = SolidSyslogVxWorks64Address_AsConstSockaddrIn(address);
    MEMCMP_EQUAL(&expected, actual, sizeof(expected));
}

TEST(SolidSyslogVxWorks64Address, CreateZeroesTheSockaddrFromAnyPriorSlotContents)
{
    struct sockaddr_in dirty = {};
    dirty.sin_family = AF_INET;
    dirty.sin_port = htons(9999U);
    dirty.sin_addr.s_addr = htonl(0xDEADBEEFU);
    *SolidSyslogVxWorks64Address_AsSockaddrIn(address) = dirty;
    SolidSyslogVxWorks64Address_Destroy(address);

    address = SolidSyslogVxWorks64Address_Create();

    struct sockaddr_in zeroes = {};
    MEMCMP_EQUAL(&zeroes, SolidSyslogVxWorks64Address_AsConstSockaddrIn(address), sizeof(zeroes));
}

// clang-format off
TEST_GROUP(SolidSyslogVxWorks64AddressPool)
{
    struct SolidSyslogAddress* pooled[SOLIDSYSLOG_ADDRESS_POOL_SIZE] = {};
    struct SolidSyslogAddress* overflow                              = nullptr;

    void teardown() override
    {
        for (auto* handle : pooled)
        {
            if (handle != nullptr)
            {
                SolidSyslogVxWorks64Address_Destroy(handle);
            }
        }
        if (overflow != nullptr)
        {
            SolidSyslogVxWorks64Address_Destroy(overflow);
        }
        ConfigLockFake_Uninstall();
    }

    void FillPool()
    {
        for (auto*& slot : pooled)
        {
            slot = SolidSyslogVxWorks64Address_Create();
        }
    }
};

// clang-format on

TEST(SolidSyslogVxWorks64AddressPool, FillingPoolThenOverflowReturnsDistinctFallback)
{
    FillPool();

    overflow = SolidSyslogVxWorks64Address_Create();

    CHECK_IS_FALLBACK(overflow, pooled);
}

TEST(SolidSyslogVxWorks64AddressPool, ExhaustedCreateReportsError)
{
    ErrorHandlerFake_Install(nullptr);
    FillPool();

    overflow = SolidSyslogVxWorks64Address_Create();

    CHECK_ERROR_REPORTED_ONCE(
        SOLIDSYSLOG_SEVERITY_CRITICAL,
        &SolidSyslogVxWorks64AddressErrorSource,
        SOLIDSYSLOG_CAT_POOL_EXHAUSTED,
        SOLIDSYSLOG_ADDRESS_ERROR_POOL_EXHAUSTED
    );
}

TEST(SolidSyslogVxWorks64AddressPool, CreateAcquiresAndReleasesConfigLockOnFirstFreeSlot)
{
    ConfigLockFake_Install();

    pooled[0] = SolidSyslogVxWorks64Address_Create();

    CALLED_FAKE(ConfigLockFake_Lock, ONCE);
    CALLED_FAKE(ConfigLockFake_Unlock, ONCE);
}

TEST(SolidSyslogVxWorks64AddressPool, CreateLocksOncePerSlotProbedWhenPoolIsFull)
{
    FillPool();
    ConfigLockFake_Install();

    overflow = SolidSyslogVxWorks64Address_Create();

    LONGS_EQUAL(SOLIDSYSLOG_ADDRESS_POOL_SIZE, ConfigLockFake_LockCallCount());
    LONGS_EQUAL(SOLIDSYSLOG_ADDRESS_POOL_SIZE, ConfigLockFake_UnlockCallCount());
}

TEST(SolidSyslogVxWorks64AddressPool, DestroyOfPooledHandleLocksOnce)
{
    pooled[0] = SolidSyslogVxWorks64Address_Create();
    ConfigLockFake_Install();

    SolidSyslogVxWorks64Address_Destroy(pooled[0]);
    pooled[0] = nullptr;

    CALLED_FAKE(ConfigLockFake_Lock, ONCE);
    CALLED_FAKE(ConfigLockFake_Unlock, ONCE);
}

TEST(SolidSyslogVxWorks64AddressPool, DestroyOfUnknownHandleDoesNotLock)
{
    ConfigLockFake_Install();
    char stranger = 0;

    SolidSyslogVxWorks64Address_Destroy((struct SolidSyslogAddress*) &stranger);

    CALLED_FAKE(ConfigLockFake_Lock, NEVER);
    CALLED_FAKE(ConfigLockFake_Unlock, NEVER);
}

TEST(SolidSyslogVxWorks64AddressPool, DestroyOfUnknownHandleReportsWarning)
{
    ErrorHandlerFake_Install(nullptr);
    char stranger = 0;

    SolidSyslogVxWorks64Address_Destroy((struct SolidSyslogAddress*) &stranger);

    CHECK_ERROR_REPORTED_ONCE(
        SOLIDSYSLOG_SEVERITY_WARNING,
        &SolidSyslogVxWorks64AddressErrorSource,
        SOLIDSYSLOG_CAT_UNKNOWN_DESTROY,
        SOLIDSYSLOG_ADDRESS_ERROR_UNKNOWN_DESTROY
    );
}

TEST(SolidSyslogVxWorks64AddressPool, DestroyOfStaleHandleReportsWarning)
{
    pooled[0] = SolidSyslogVxWorks64Address_Create();
    SolidSyslogVxWorks64Address_Destroy(pooled[0]);
    ErrorHandlerFake_Install(nullptr);

    SolidSyslogVxWorks64Address_Destroy(pooled[0]);
    pooled[0] = nullptr;

    CHECK_ERROR_REPORTED_ONCE(
        SOLIDSYSLOG_SEVERITY_WARNING,
        &SolidSyslogVxWorks64AddressErrorSource,
        SOLIDSYSLOG_CAT_UNKNOWN_DESTROY,
        SOLIDSYSLOG_ADDRESS_ERROR_UNKNOWN_DESTROY
    );
}
