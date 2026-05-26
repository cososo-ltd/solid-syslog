#include "TestUtils.h"
#include "CppUTest/TestHarness.h"

using namespace CososoTesting;

#include "ConfigLockFake.h"
#include "ErrorHandlerFake.h"
#include "SolidSyslogPlusTcpAddress.h"
#include "SolidSyslogPlusTcpAddressErrors.h"
#include "SolidSyslogPlusTcpAddressPrivate.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogTunables.h"
#include "FreeRTOS.h"
#include "FreeRTOS_IP.h"
#include "FreeRTOS_Sockets.h"

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

// clang-format off
TEST_GROUP(SolidSyslogPlusTcpAddress)
{
    struct SolidSyslogAddress* address = nullptr;

    void setup() override
    {
        address = SolidSyslogPlusTcpAddress_Create();
    }

    void teardown() override
    {
        SolidSyslogPlusTcpAddress_Destroy(address);
    }
};

// clang-format on

TEST(SolidSyslogPlusTcpAddress, CreateReturnsNonNull)

{
    CHECK(address != nullptr);
}

TEST(SolidSyslogPlusTcpAddress, AsFreertosSockaddrRoundTripsBytes)

{
    struct freertos_sockaddr expected = {};
    expected.sin_family = FREERTOS_AF_INET;
    expected.sin_port = FreeRTOS_htons(514U);
    expected.sin_address.ulIP_IPv4 = FreeRTOS_inet_addr_quick(127, 0, 0, 1);

    *SolidSyslogPlusTcpAddress_AsFreertosSockaddr(address) = expected;

    const struct freertos_sockaddr* actual = SolidSyslogPlusTcpAddress_AsConstFreertosSockaddr(address);
    MEMCMP_EQUAL(&expected, actual, sizeof(expected));
}

TEST(SolidSyslogPlusTcpAddress, CreateZeroesTheSockaddrFromAnyPriorSlotContents)

{
    struct freertos_sockaddr dirty = {};
    dirty.sin_family = FREERTOS_AF_INET;
    dirty.sin_port = FreeRTOS_htons(9999U);
    dirty.sin_address.ulIP_IPv4 = FreeRTOS_inet_addr_quick(1, 2, 3, 4);
    *SolidSyslogPlusTcpAddress_AsFreertosSockaddr(address) = dirty;
    SolidSyslogPlusTcpAddress_Destroy(address);

    address = SolidSyslogPlusTcpAddress_Create();

    struct freertos_sockaddr zeroes = {};
    MEMCMP_EQUAL(&zeroes, SolidSyslogPlusTcpAddress_AsConstFreertosSockaddr(address), sizeof(zeroes));
}

// clang-format off
TEST_GROUP(SolidSyslogPlusTcpAddressPool)
{
    struct SolidSyslogAddress* pooled[SOLIDSYSLOG_ADDRESS_POOL_SIZE] = {};
    struct SolidSyslogAddress* overflow                              = nullptr;

    void teardown() override
    {
        for (auto* handle : pooled)
        {
            if (handle != nullptr)
            {
                SolidSyslogPlusTcpAddress_Destroy(handle);
            }
        }
        if (overflow != nullptr)
        {
            SolidSyslogPlusTcpAddress_Destroy(overflow);
        }
        ConfigLockFake_Uninstall();
    }

    void FillPool()
    {
        for (auto*& slot : pooled)
        {
            slot = SolidSyslogPlusTcpAddress_Create();
        }
    }
};

// clang-format on

TEST(SolidSyslogPlusTcpAddressPool, FillingPoolThenOverflowReturnsDistinctFallback)

{
    FillPool();

    overflow = SolidSyslogPlusTcpAddress_Create();

    CHECK_IS_FALLBACK(overflow, pooled);
}

TEST(SolidSyslogPlusTcpAddressPool, ExhaustedCreateReportsError)

{
    ErrorHandlerFake_Install(nullptr);
    FillPool();

    overflow = SolidSyslogPlusTcpAddress_Create();

    CALLED_FAKE(ErrorHandlerFake_Handle, ONCE);
    LONGS_EQUAL(SOLIDSYSLOG_SEVERITY_ERROR, ErrorHandlerFake_LastSeverity());
    POINTERS_EQUAL(&PlusTcpAddressErrorSource, ErrorHandlerFake_LastSource());
    UNSIGNED_LONGS_EQUAL(PLUSTCPADDRESS_ERROR_POOL_EXHAUSTED, ErrorHandlerFake_LastCode());
}

TEST(SolidSyslogPlusTcpAddressPool, CreateAcquiresAndReleasesConfigLockOnFirstFreeSlot)

{
    ConfigLockFake_Install();

    pooled[0] = SolidSyslogPlusTcpAddress_Create();

    CALLED_FAKE(ConfigLockFake_Lock, ONCE);
    CALLED_FAKE(ConfigLockFake_Unlock, ONCE);
}

TEST(SolidSyslogPlusTcpAddressPool, CreateLocksOncePerSlotProbedWhenPoolIsFull)

{
    FillPool();
    ConfigLockFake_Install();

    overflow = SolidSyslogPlusTcpAddress_Create();

    LONGS_EQUAL(SOLIDSYSLOG_ADDRESS_POOL_SIZE, ConfigLockFake_LockCallCount());
    LONGS_EQUAL(SOLIDSYSLOG_ADDRESS_POOL_SIZE, ConfigLockFake_UnlockCallCount());
}

TEST(SolidSyslogPlusTcpAddressPool, DestroyOfPooledHandleLocksOnce)

{
    pooled[0] = SolidSyslogPlusTcpAddress_Create();
    ConfigLockFake_Install();

    SolidSyslogPlusTcpAddress_Destroy(pooled[0]);
    pooled[0] = nullptr;

    CALLED_FAKE(ConfigLockFake_Lock, ONCE);
    CALLED_FAKE(ConfigLockFake_Unlock, ONCE);
}

TEST(SolidSyslogPlusTcpAddressPool, DestroyOfUnknownHandleDoesNotLock)

{
    ConfigLockFake_Install();
    char stranger = 0;

    SolidSyslogPlusTcpAddress_Destroy((struct SolidSyslogAddress*) &stranger);

    CALLED_FAKE(ConfigLockFake_Lock, NEVER);
    CALLED_FAKE(ConfigLockFake_Unlock, NEVER);
}

TEST(SolidSyslogPlusTcpAddressPool, DestroyOfUnknownHandleReportsWarning)

{
    ErrorHandlerFake_Install(nullptr);
    char stranger = 0;

    SolidSyslogPlusTcpAddress_Destroy((struct SolidSyslogAddress*) &stranger);

    CALLED_FAKE(ErrorHandlerFake_Handle, ONCE);
    LONGS_EQUAL(SOLIDSYSLOG_SEVERITY_WARNING, ErrorHandlerFake_LastSeverity());
    POINTERS_EQUAL(&PlusTcpAddressErrorSource, ErrorHandlerFake_LastSource());
    UNSIGNED_LONGS_EQUAL(PLUSTCPADDRESS_ERROR_UNKNOWN_DESTROY, ErrorHandlerFake_LastCode());
}

TEST(SolidSyslogPlusTcpAddressPool, DestroyOfStaleHandleReportsWarning)

{
    pooled[0] = SolidSyslogPlusTcpAddress_Create();
    SolidSyslogPlusTcpAddress_Destroy(pooled[0]);
    ErrorHandlerFake_Install(nullptr);

    SolidSyslogPlusTcpAddress_Destroy(pooled[0]);
    pooled[0] = nullptr;

    CALLED_FAKE(ErrorHandlerFake_Handle, ONCE);
    LONGS_EQUAL(SOLIDSYSLOG_SEVERITY_WARNING, ErrorHandlerFake_LastSeverity());
    POINTERS_EQUAL(&PlusTcpAddressErrorSource, ErrorHandlerFake_LastSource());
    UNSIGNED_LONGS_EQUAL(PLUSTCPADDRESS_ERROR_UNKNOWN_DESTROY, ErrorHandlerFake_LastCode());
}
