#include "TestUtils.h"
#include "CppUTest/TestHarness.h"

using namespace CososoTesting;

#include "lwip/sockets.h"

#include <cstdint>

#include "ConfigLockFake.h"
#include "ErrorHandlerFake.h"
#include "LwipSocketsFake.h"
#include "SolidSyslogErrorCategory.h"
#include "SolidSyslogLwipSocketAddress.h"
#include "SolidSyslogLwipSocketAddressPrivate.h"
#include "SolidSyslogLwipSocketTcpStream.h"
#include "SolidSyslogLwipSocketTcpStreamErrors.h"
#include "SolidSyslogNullStream.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogStream.h"
#include "SolidSyslogTunables.h"

static const struct SolidSyslogLwipSocketTcpStreamConfig config = {nullptr, nullptr};

// clang-format off
// 6514 rather than 514: 514 is 0x0202, so host and network order are the same
// bytes and an assertion on the port would hold whichever the adapter wrote.
static const uint16_t TEST_PORT       = 6514U;
static const uint32_t TEST_IPV4       = 0xC000020AU;
static const int      TEST_DESCRIPTOR = 7;
// clang-format on

// clang-format off
TEST_GROUP(SolidSyslogLwipSocketTcpStream)
{
    struct SolidSyslogStream*  stream  = nullptr;
    struct SolidSyslogAddress* address = nullptr;

    void setup() override
    {
        LwipSocketsFake_Reset();
        stream  = SolidSyslogLwipSocketTcpStream_Create(&config);
        address = SolidSyslogLwipSocketAddress_Create();
        struct sockaddr_in* sin = SolidSyslogLwipSocketAddress_AsSockaddrIn(address);
        sin->sin_family         = AF_INET;
        sin->sin_port           = lwip_htons(TEST_PORT);
        sin->sin_addr.s_addr    = lwip_htonl(TEST_IPV4);
        // Installed after the pool draws above, so an event count starts clean.
        ErrorHandlerFake_Install(nullptr);
    }

    void teardown() override
    {
        SolidSyslogLwipSocketAddress_Destroy(address);
        SolidSyslogLwipSocketTcpStream_Destroy(stream);
    }

    bool Open() const
    {
        return SolidSyslogStream_Open(stream, address);
    }
};

// clang-format on

TEST(SolidSyslogLwipSocketTcpStream, OpenTakesAnIpv4TcpSocket)
{
    CHECK_TRUE(Open());

    LONGS_EQUAL(1U, LwipSocketsFake_SocketCallCount());
    LONGS_EQUAL(AF_INET, LwipSocketsFake_LastSocketDomain());
    LONGS_EQUAL(SOCK_STREAM, LwipSocketsFake_LastSocketType());
    LONGS_EQUAL(0, LwipSocketsFake_LastSocketProtocol());
}


// clang-format off
TEST_GROUP(SolidSyslogLwipSocketTcpStreamPool)
{
    struct SolidSyslogStream* pooled[SOLIDSYSLOG_TCP_STREAM_POOL_SIZE] = {};
    struct SolidSyslogStream* overflow                               = nullptr;

    void teardown() override
    {
        for (auto* handle : pooled)
        {
            if (handle != nullptr)
            {
                SolidSyslogLwipSocketTcpStream_Destroy(handle);
            }
        }
        ConfigLockFake_Uninstall();
    }

    void FillPool()
    {
        for (auto*& slot : pooled)
        {
            slot = SolidSyslogLwipSocketTcpStream_Create(&config);
        }
    }
};

// clang-format on

TEST(SolidSyslogLwipSocketTcpStreamPool, FillingPoolThenOverflowReturnsTheNullStream)
{
    FillPool();

    overflow = SolidSyslogLwipSocketTcpStream_Create(&config);

    POINTERS_EQUAL(SolidSyslogNullStream_Get(), overflow);
}

TEST(SolidSyslogLwipSocketTcpStreamPool, ExhaustedCreateReportsError)
{
    ErrorHandlerFake_Install(nullptr);
    FillPool();

    overflow = SolidSyslogLwipSocketTcpStream_Create(&config);

    CHECK_ERROR_REPORTED_ONCE(
        SOLIDSYSLOG_SEVERITY_CRITICAL,
        &SolidSyslogLwipSocketTcpStreamErrorSource,
        SOLIDSYSLOG_CAT_POOL_EXHAUSTED,
        SOLIDSYSLOG_TCP_STREAM_ERROR_POOL_EXHAUSTED
    );
}

TEST(SolidSyslogLwipSocketTcpStreamPool, CreateAcquiresAndReleasesConfigLockOnFirstFreeSlot)
{
    ConfigLockFake_Install();

    pooled[0] = SolidSyslogLwipSocketTcpStream_Create(&config);

    CALLED_FAKE(ConfigLockFake_Lock, ONCE);
    CALLED_FAKE(ConfigLockFake_Unlock, ONCE);
}

TEST(SolidSyslogLwipSocketTcpStreamPool, CreateLocksOncePerSlotProbedWhenPoolIsFull)
{
    FillPool();
    ConfigLockFake_Install();

    overflow = SolidSyslogLwipSocketTcpStream_Create(&config);

    LONGS_EQUAL(SOLIDSYSLOG_TCP_STREAM_POOL_SIZE, ConfigLockFake_LockCallCount());
    LONGS_EQUAL(SOLIDSYSLOG_TCP_STREAM_POOL_SIZE, ConfigLockFake_UnlockCallCount());
}

TEST(SolidSyslogLwipSocketTcpStreamPool, DestroyOfPooledHandleLocksOnce)
{
    pooled[0] = SolidSyslogLwipSocketTcpStream_Create(&config);
    ConfigLockFake_Install();

    SolidSyslogLwipSocketTcpStream_Destroy(pooled[0]);
    pooled[0] = nullptr;

    CALLED_FAKE(ConfigLockFake_Lock, ONCE);
    CALLED_FAKE(ConfigLockFake_Unlock, ONCE);
}

TEST(SolidSyslogLwipSocketTcpStreamPool, DestroyOfUnknownHandleDoesNotLock)
{
    ConfigLockFake_Install();
    char stranger = 0;

    SolidSyslogLwipSocketTcpStream_Destroy((struct SolidSyslogStream*) &stranger);

    CALLED_FAKE(ConfigLockFake_Lock, NEVER);
    CALLED_FAKE(ConfigLockFake_Unlock, NEVER);
}

TEST(SolidSyslogLwipSocketTcpStreamPool, DestroyOfUnknownHandleReportsWarning)
{
    ErrorHandlerFake_Install(nullptr);
    char stranger = 0;

    SolidSyslogLwipSocketTcpStream_Destroy((struct SolidSyslogStream*) &stranger);

    CHECK_ERROR_REPORTED_ONCE(
        SOLIDSYSLOG_SEVERITY_WARNING,
        &SolidSyslogLwipSocketTcpStreamErrorSource,
        SOLIDSYSLOG_CAT_UNKNOWN_DESTROY,
        SOLIDSYSLOG_TCP_STREAM_ERROR_UNKNOWN_DESTROY
    );
}

TEST(SolidSyslogLwipSocketTcpStreamPool, DestroyOfStaleHandleReportsWarning)
{
    pooled[0] = SolidSyslogLwipSocketTcpStream_Create(&config);
    SolidSyslogLwipSocketTcpStream_Destroy(pooled[0]);
    ErrorHandlerFake_Install(nullptr);

    SolidSyslogLwipSocketTcpStream_Destroy(pooled[0]);
    pooled[0] = nullptr;

    CHECK_ERROR_REPORTED_ONCE(
        SOLIDSYSLOG_SEVERITY_WARNING,
        &SolidSyslogLwipSocketTcpStreamErrorSource,
        SOLIDSYSLOG_CAT_UNKNOWN_DESTROY,
        SOLIDSYSLOG_TCP_STREAM_ERROR_UNKNOWN_DESTROY
    );
}

