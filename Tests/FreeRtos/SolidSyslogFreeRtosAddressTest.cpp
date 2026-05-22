#include "TestUtils.h"
#include "CppUTest/TestHarness.h"

using namespace CososoTesting; // NOLINT(google-build-using-namespace) -- test-file scope only; brings NEVER/ONCE/TWICE/THRICE into scope for the CALLED_*
    // macros

#include <cstring>

#include "ConfigLockFake.h"
#include "ErrorHandlerFake.h"
#include "SolidSyslogErrorMessages.h"
#include "SolidSyslogFreeRtosAddress.h"
#include "SolidSyslogFreeRtosAddressPrivate.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogTunables.h"

#include "FreeRTOS.h"
#include "FreeRTOS_IP.h"
#include "FreeRTOS_Sockets.h"

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

// NOLINTEND(cppcoreguidelines-macro-usage,cppcoreguidelines-avoid-do-while)

// clang-format off
TEST_GROUP(SolidSyslogFreeRtosAddress)
{
    struct SolidSyslogAddress* address = nullptr;

    void setup() override
    {
        address = SolidSyslogFreeRtosAddress_Create();
    }

    void teardown() override
    {
        SolidSyslogFreeRtosAddress_Destroy(address);
    }
};

// clang-format on

TEST(SolidSyslogFreeRtosAddress, CreateReturnsNonNull)
{
    CHECK(address != nullptr);
}

TEST(SolidSyslogFreeRtosAddress, AsFreertosSockaddrRoundTripsBytes)
{
    struct freertos_sockaddr expected = {};
    expected.sin_family = FREERTOS_AF_INET;
    expected.sin_port = FreeRTOS_htons(514U);
    expected.sin_address.ulIP_IPv4 = FreeRTOS_inet_addr_quick(127, 0, 0, 1);

    *SolidSyslogFreeRtosAddress_AsFreertosSockaddr(address) = expected;

    const struct freertos_sockaddr* actual = SolidSyslogFreeRtosAddress_AsConstFreertosSockaddr(address);
    MEMCMP_EQUAL(&expected, actual, sizeof(expected));
}

TEST(SolidSyslogFreeRtosAddress, CreateZeroesTheSockaddrFromAnyPriorSlotContents)
{
    struct freertos_sockaddr dirty = {};
    dirty.sin_family = FREERTOS_AF_INET;
    dirty.sin_port = FreeRTOS_htons(9999U);
    dirty.sin_address.ulIP_IPv4 = FreeRTOS_inet_addr_quick(1, 2, 3, 4);
    *SolidSyslogFreeRtosAddress_AsFreertosSockaddr(address) = dirty;
    SolidSyslogFreeRtosAddress_Destroy(address);

    address = SolidSyslogFreeRtosAddress_Create();

    struct freertos_sockaddr zeroes = {};
    MEMCMP_EQUAL(&zeroes, SolidSyslogFreeRtosAddress_AsConstFreertosSockaddr(address), sizeof(zeroes));
}

// clang-format off
TEST_GROUP(SolidSyslogFreeRtosAddressPool)
{
    // cppcheck-suppress constVariable -- assigned in test bodies; cppcheck does not model CppUTest lifecycle
    struct SolidSyslogAddress* pooled[SOLIDSYSLOG_ADDRESS_POOL_SIZE] = {};
    struct SolidSyslogAddress* overflow                              = nullptr;

    void teardown() override
    {
        for (auto* handle : pooled)
        {
            if (handle != nullptr)
            {
                SolidSyslogFreeRtosAddress_Destroy(handle);
            }
        }
        // cppcheck-suppress knownConditionTrueFalse -- assigned in test bodies; cppcheck does not model CppUTest lifecycle
        if (overflow != nullptr)
        {
            SolidSyslogFreeRtosAddress_Destroy(overflow);
        }
        ConfigLockFake_Uninstall();
        ErrorHandlerFake_Uninstall();
    }

    void FillPool()
    {
        for (auto*& slot : pooled)
        {
            slot = SolidSyslogFreeRtosAddress_Create();
        }
    }
};

// clang-format on

TEST(SolidSyslogFreeRtosAddressPool, FillingPoolThenOverflowReturnsDistinctFallback)
{
    FillPool();

    overflow = SolidSyslogFreeRtosAddress_Create();

    CHECK_IS_FALLBACK(overflow, pooled);
}

TEST(SolidSyslogFreeRtosAddressPool, ExhaustedCreateReportsError)
{
    ErrorHandlerFake_Install(nullptr);
    FillPool();

    overflow = SolidSyslogFreeRtosAddress_Create();

    CALLED_FAKE(ErrorHandlerFake_Handle, ONCE);
    LONGS_EQUAL(SOLIDSYSLOG_SEVERITY_ERROR, ErrorHandlerFake_LastSeverity());
    STRCMP_EQUAL(SOLIDSYSLOG_ERROR_MSG_FREERTOSADDRESS_POOL_EXHAUSTED, ErrorHandlerFake_LastMessage());
}

TEST(SolidSyslogFreeRtosAddressPool, CreateAcquiresAndReleasesConfigLockOnFirstFreeSlot)
{
    ConfigLockFake_Install();

    pooled[0] = SolidSyslogFreeRtosAddress_Create();

    CALLED_FAKE(ConfigLockFake_Lock, ONCE);
    CALLED_FAKE(ConfigLockFake_Unlock, ONCE);
}

TEST(SolidSyslogFreeRtosAddressPool, CreateLocksOncePerSlotProbedWhenPoolIsFull)
{
    FillPool();
    ConfigLockFake_Install();

    overflow = SolidSyslogFreeRtosAddress_Create();

    LONGS_EQUAL(SOLIDSYSLOG_ADDRESS_POOL_SIZE, ConfigLockFake_LockCallCount());
    LONGS_EQUAL(SOLIDSYSLOG_ADDRESS_POOL_SIZE, ConfigLockFake_UnlockCallCount());
}

TEST(SolidSyslogFreeRtosAddressPool, DestroyOfPooledHandleLocksOnce)
{
    pooled[0] = SolidSyslogFreeRtosAddress_Create();
    ConfigLockFake_Install();

    SolidSyslogFreeRtosAddress_Destroy(pooled[0]);
    pooled[0] = nullptr;

    CALLED_FAKE(ConfigLockFake_Lock, ONCE);
    CALLED_FAKE(ConfigLockFake_Unlock, ONCE);
}

TEST(SolidSyslogFreeRtosAddressPool, DestroyOfUnknownHandleDoesNotLock)
{
    ConfigLockFake_Install();
    char stranger = 0;

    SolidSyslogFreeRtosAddress_Destroy((struct SolidSyslogAddress*) &stranger);

    CALLED_FAKE(ConfigLockFake_Lock, NEVER);
    CALLED_FAKE(ConfigLockFake_Unlock, NEVER);
}

TEST(SolidSyslogFreeRtosAddressPool, DestroyOfUnknownHandleReportsWarning)
{
    ErrorHandlerFake_Install(nullptr);
    char stranger = 0;

    SolidSyslogFreeRtosAddress_Destroy((struct SolidSyslogAddress*) &stranger);

    CALLED_FAKE(ErrorHandlerFake_Handle, ONCE);
    LONGS_EQUAL(SOLIDSYSLOG_SEVERITY_WARNING, ErrorHandlerFake_LastSeverity());
    STRCMP_EQUAL(SOLIDSYSLOG_ERROR_MSG_FREERTOSADDRESS_UNKNOWN_DESTROY, ErrorHandlerFake_LastMessage());
}

TEST(SolidSyslogFreeRtosAddressPool, DestroyOfStaleHandleReportsWarning)
{
    pooled[0] = SolidSyslogFreeRtosAddress_Create();
    SolidSyslogFreeRtosAddress_Destroy(pooled[0]);
    ErrorHandlerFake_Install(nullptr);

    SolidSyslogFreeRtosAddress_Destroy(pooled[0]);
    pooled[0] = nullptr;

    CALLED_FAKE(ErrorHandlerFake_Handle, ONCE);
    LONGS_EQUAL(SOLIDSYSLOG_SEVERITY_WARNING, ErrorHandlerFake_LastSeverity());
    STRCMP_EQUAL(SOLIDSYSLOG_ERROR_MSG_FREERTOSADDRESS_UNKNOWN_DESTROY, ErrorHandlerFake_LastMessage());
}
