#include "TestUtils.h"
#include "CppUTest/TestHarness.h"

using namespace CososoTesting;

#include <cstring>
#include <winsock2.h>

#include "ConfigLockFake.h"
#include "ErrorHandlerFake.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogTunables.h"
#include "SolidSyslogWinsockAddress.h"
#include "SolidSyslogWinsockAddressErrors.h"
#include "SolidSyslogWinsockAddressPrivate.h"

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
TEST_GROUP(SolidSyslogWinsockAddress)
{
    struct SolidSyslogAddress* address = nullptr;

    void setup() override
    {
        address = SolidSyslogWinsockAddress_Create();
    }

    void teardown() override
    {
        SolidSyslogWinsockAddress_Destroy(address);
    }
};

// clang-format on

TEST(SolidSyslogWinsockAddress, CreateReturnsNonNull)
{
    CHECK(address != nullptr);
}

TEST(SolidSyslogWinsockAddress, AsSockaddrInRoundTripsBytes)
{
    struct sockaddr_in expected = {};
    expected.sin_family = AF_INET;
    expected.sin_port = htons(514U);
    expected.sin_addr.s_addr = htonl(0x7F000001U);

    *SolidSyslogWinsockAddress_AsSockaddrIn(address) = expected;

    const struct sockaddr_in* actual = SolidSyslogWinsockAddress_AsConstSockaddrIn(address);
    MEMCMP_EQUAL(&expected, actual, sizeof(expected));
}

TEST(SolidSyslogWinsockAddress, CreateZeroesTheSockaddrFromAnyPriorSlotContents)
{
    struct sockaddr_in dirty = {};
    dirty.sin_family = AF_INET;
    dirty.sin_port = htons(9999U);
    dirty.sin_addr.s_addr = htonl(0xDEADBEEFU);
    *SolidSyslogWinsockAddress_AsSockaddrIn(address) = dirty;
    SolidSyslogWinsockAddress_Destroy(address);

    address = SolidSyslogWinsockAddress_Create();

    struct sockaddr_in zeroes = {};
    MEMCMP_EQUAL(&zeroes, SolidSyslogWinsockAddress_AsConstSockaddrIn(address), sizeof(zeroes));
}

// clang-format off
TEST_GROUP(SolidSyslogWinsockAddressPool)
{
    struct SolidSyslogAddress* pooled[SOLIDSYSLOG_ADDRESS_POOL_SIZE] = {};
    struct SolidSyslogAddress* overflow                              = nullptr;

    void teardown() override
    {
        for (auto* handle : pooled)
        {
            if (handle != nullptr)
            {
                SolidSyslogWinsockAddress_Destroy(handle);
            }
        }
        if (overflow != nullptr)
        {
            SolidSyslogWinsockAddress_Destroy(overflow);
        }
        ConfigLockFake_Uninstall();
    }

    void FillPool()
    {
        for (auto*& slot : pooled)
        {
            slot = SolidSyslogWinsockAddress_Create();
        }
    }
};

// clang-format on

TEST(SolidSyslogWinsockAddressPool, FillingPoolThenOverflowReturnsDistinctFallback)
{
    FillPool();

    overflow = SolidSyslogWinsockAddress_Create();

    CHECK_IS_FALLBACK(overflow, pooled);
}

TEST(SolidSyslogWinsockAddressPool, ExhaustedCreateReportsError)
{
    ErrorHandlerFake_Install(nullptr);
    FillPool();

    overflow = SolidSyslogWinsockAddress_Create();

    CALLED_FAKE(ErrorHandlerFake_Handle, ONCE);
    LONGS_EQUAL(SOLIDSYSLOG_SEVERITY_ERROR, ErrorHandlerFake_LastSeverity());
    POINTERS_EQUAL(&WinsockAddressErrorSource, ErrorHandlerFake_LastSource());
    UNSIGNED_LONGS_EQUAL(WINSOCKADDRESS_ERROR_POOL_EXHAUSTED, ErrorHandlerFake_LastCode());
}

TEST(SolidSyslogWinsockAddressPool, CreateAcquiresAndReleasesConfigLockOnFirstFreeSlot)
{
    ConfigLockFake_Install();

    pooled[0] = SolidSyslogWinsockAddress_Create();

    CALLED_FAKE(ConfigLockFake_Lock, ONCE);
    CALLED_FAKE(ConfigLockFake_Unlock, ONCE);
}

TEST(SolidSyslogWinsockAddressPool, CreateLocksOncePerSlotProbedWhenPoolIsFull)
{
    FillPool();
    ConfigLockFake_Install();

    overflow = SolidSyslogWinsockAddress_Create();

    LONGS_EQUAL(SOLIDSYSLOG_ADDRESS_POOL_SIZE, ConfigLockFake_LockCallCount());
    LONGS_EQUAL(SOLIDSYSLOG_ADDRESS_POOL_SIZE, ConfigLockFake_UnlockCallCount());
}

TEST(SolidSyslogWinsockAddressPool, DestroyOfPooledHandleLocksOnce)
{
    pooled[0] = SolidSyslogWinsockAddress_Create();
    ConfigLockFake_Install();

    SolidSyslogWinsockAddress_Destroy(pooled[0]);
    pooled[0] = nullptr;

    CALLED_FAKE(ConfigLockFake_Lock, ONCE);
    CALLED_FAKE(ConfigLockFake_Unlock, ONCE);
}

TEST(SolidSyslogWinsockAddressPool, DestroyOfUnknownHandleDoesNotLock)
{
    ConfigLockFake_Install();
    char stranger = 0;

    SolidSyslogWinsockAddress_Destroy((struct SolidSyslogAddress*) &stranger);

    CALLED_FAKE(ConfigLockFake_Lock, NEVER);
    CALLED_FAKE(ConfigLockFake_Unlock, NEVER);
}

TEST(SolidSyslogWinsockAddressPool, DestroyOfUnknownHandleReportsWarning)
{
    ErrorHandlerFake_Install(nullptr);
    char stranger = 0;

    SolidSyslogWinsockAddress_Destroy((struct SolidSyslogAddress*) &stranger);

    CALLED_FAKE(ErrorHandlerFake_Handle, ONCE);
    LONGS_EQUAL(SOLIDSYSLOG_SEVERITY_WARNING, ErrorHandlerFake_LastSeverity());
    POINTERS_EQUAL(&WinsockAddressErrorSource, ErrorHandlerFake_LastSource());
    UNSIGNED_LONGS_EQUAL(WINSOCKADDRESS_ERROR_UNKNOWN_DESTROY, ErrorHandlerFake_LastCode());
}

TEST(SolidSyslogWinsockAddressPool, DestroyOfStaleHandleReportsWarning)
{
    pooled[0] = SolidSyslogWinsockAddress_Create();
    SolidSyslogWinsockAddress_Destroy(pooled[0]);
    ErrorHandlerFake_Install(nullptr);

    SolidSyslogWinsockAddress_Destroy(pooled[0]);
    pooled[0] = nullptr;

    CALLED_FAKE(ErrorHandlerFake_Handle, ONCE);
    LONGS_EQUAL(SOLIDSYSLOG_SEVERITY_WARNING, ErrorHandlerFake_LastSeverity());
    POINTERS_EQUAL(&WinsockAddressErrorSource, ErrorHandlerFake_LastSource());
    UNSIGNED_LONGS_EQUAL(WINSOCKADDRESS_ERROR_UNKNOWN_DESTROY, ErrorHandlerFake_LastCode());
}
