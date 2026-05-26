#include "TestUtils.h"
#include "CppUTest/TestHarness.h"

using namespace CososoTesting;

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include "ConfigLockFake.h"
#include "ErrorHandlerFake.h"
#include "SolidSyslogPosixAddress.h"
#include "SolidSyslogPosixAddressErrors.h"
#include "SolidSyslogPosixAddressPrivate.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogTunables.h"

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
TEST_GROUP(SolidSyslogPosixAddress)
{
    struct SolidSyslogAddress* address = nullptr;

    void setup() override
    {
        address = SolidSyslogPosixAddress_Create();
    }

    void teardown() override
    {
        SolidSyslogPosixAddress_Destroy(address);
    }
};

// clang-format on

TEST(SolidSyslogPosixAddress, CreateReturnsNonNull)
{
    CHECK(address != nullptr);
}

TEST(SolidSyslogPosixAddress, AsSockaddrInRoundTripsBytes)
{
    struct sockaddr_in expected = {};
    expected.sin_family = AF_INET;
    expected.sin_port = htons(514U);
    expected.sin_addr.s_addr = htonl(0x7F000001U);

    *SolidSyslogPosixAddress_AsSockaddrIn(address) = expected;

    const struct sockaddr_in* actual = SolidSyslogPosixAddress_AsConstSockaddrIn(address);
    MEMCMP_EQUAL(&expected, actual, sizeof(expected));
}

TEST(SolidSyslogPosixAddress, CreateZeroesTheSockaddrFromAnyPriorSlotContents)
{
    struct sockaddr_in dirty = {};
    dirty.sin_family = AF_INET;
    dirty.sin_port = htons(9999U);
    dirty.sin_addr.s_addr = htonl(0xDEADBEEFU);
    *SolidSyslogPosixAddress_AsSockaddrIn(address) = dirty;
    SolidSyslogPosixAddress_Destroy(address);

    address = SolidSyslogPosixAddress_Create();

    struct sockaddr_in zeroes = {};
    MEMCMP_EQUAL(&zeroes, SolidSyslogPosixAddress_AsConstSockaddrIn(address), sizeof(zeroes));
}

// clang-format off
TEST_GROUP(SolidSyslogPosixAddressPool)
{
    struct SolidSyslogAddress* pooled[SOLIDSYSLOG_ADDRESS_POOL_SIZE] = {};
    struct SolidSyslogAddress* overflow                              = nullptr;

    void teardown() override
    {
        for (auto* handle : pooled)
        {
            if (handle != nullptr)
            {
                SolidSyslogPosixAddress_Destroy(handle);
            }
        }
        if (overflow != nullptr)
        {
            SolidSyslogPosixAddress_Destroy(overflow);
        }
        ConfigLockFake_Uninstall();
    }

    void FillPool()
    {
        for (auto*& slot : pooled)
        {
            slot = SolidSyslogPosixAddress_Create();
        }
    }
};

// clang-format on

TEST(SolidSyslogPosixAddressPool, FillingPoolThenOverflowReturnsDistinctFallback)
{
    FillPool();

    overflow = SolidSyslogPosixAddress_Create();

    CHECK_IS_FALLBACK(overflow, pooled);
}

TEST(SolidSyslogPosixAddressPool, ExhaustedCreateReportsError)
{
    ErrorHandlerFake_Install(nullptr);
    FillPool();

    overflow = SolidSyslogPosixAddress_Create();

    CALLED_FAKE(ErrorHandlerFake_Handle, ONCE);
    LONGS_EQUAL(SOLIDSYSLOG_SEVERITY_ERROR, ErrorHandlerFake_LastSeverity());
    POINTERS_EQUAL(&PosixAddressErrorSource, ErrorHandlerFake_LastSource());
    UNSIGNED_LONGS_EQUAL(POSIXADDRESS_ERROR_POOL_EXHAUSTED, ErrorHandlerFake_LastCode());
}

TEST(SolidSyslogPosixAddressPool, CreateAcquiresAndReleasesConfigLockOnFirstFreeSlot)
{
    ConfigLockFake_Install();

    pooled[0] = SolidSyslogPosixAddress_Create();

    CALLED_FAKE(ConfigLockFake_Lock, ONCE);
    CALLED_FAKE(ConfigLockFake_Unlock, ONCE);
}

TEST(SolidSyslogPosixAddressPool, CreateLocksOncePerSlotProbedWhenPoolIsFull)
{
    FillPool();
    ConfigLockFake_Install();

    overflow = SolidSyslogPosixAddress_Create();

    LONGS_EQUAL(SOLIDSYSLOG_ADDRESS_POOL_SIZE, ConfigLockFake_LockCallCount());
    LONGS_EQUAL(SOLIDSYSLOG_ADDRESS_POOL_SIZE, ConfigLockFake_UnlockCallCount());
}

TEST(SolidSyslogPosixAddressPool, DestroyOfPooledHandleLocksOnce)
{
    pooled[0] = SolidSyslogPosixAddress_Create();
    ConfigLockFake_Install();

    SolidSyslogPosixAddress_Destroy(pooled[0]);
    pooled[0] = nullptr;

    CALLED_FAKE(ConfigLockFake_Lock, ONCE);
    CALLED_FAKE(ConfigLockFake_Unlock, ONCE);
}

TEST(SolidSyslogPosixAddressPool, DestroyOfUnknownHandleDoesNotLock)
{
    ConfigLockFake_Install();
    char stranger = 0;

    SolidSyslogPosixAddress_Destroy((struct SolidSyslogAddress*) &stranger);

    CALLED_FAKE(ConfigLockFake_Lock, NEVER);
    CALLED_FAKE(ConfigLockFake_Unlock, NEVER);
}

TEST(SolidSyslogPosixAddressPool, DestroyOfUnknownHandleReportsWarning)
{
    ErrorHandlerFake_Install(nullptr);
    char stranger = 0;

    SolidSyslogPosixAddress_Destroy((struct SolidSyslogAddress*) &stranger);

    CALLED_FAKE(ErrorHandlerFake_Handle, ONCE);
    LONGS_EQUAL(SOLIDSYSLOG_SEVERITY_WARNING, ErrorHandlerFake_LastSeverity());
    POINTERS_EQUAL(&PosixAddressErrorSource, ErrorHandlerFake_LastSource());
    UNSIGNED_LONGS_EQUAL(POSIXADDRESS_ERROR_UNKNOWN_DESTROY, ErrorHandlerFake_LastCode());
}

TEST(SolidSyslogPosixAddressPool, DestroyOfStaleHandleReportsWarning)
{
    pooled[0] = SolidSyslogPosixAddress_Create();
    SolidSyslogPosixAddress_Destroy(pooled[0]);
    ErrorHandlerFake_Install(nullptr);

    SolidSyslogPosixAddress_Destroy(pooled[0]);
    pooled[0] = nullptr;

    CALLED_FAKE(ErrorHandlerFake_Handle, ONCE);
    LONGS_EQUAL(SOLIDSYSLOG_SEVERITY_WARNING, ErrorHandlerFake_LastSeverity());
    POINTERS_EQUAL(&PosixAddressErrorSource, ErrorHandlerFake_LastSource());
    UNSIGNED_LONGS_EQUAL(POSIXADDRESS_ERROR_UNKNOWN_DESTROY, ErrorHandlerFake_LastCode());
}
