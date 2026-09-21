#include "TestUtils.h"
#include "CppUTest/TestHarness.h"

using namespace CososoTesting;

#include "lwip/sockets.h"

#include <cstdint>

#include "ErrorHandlerFake.h"
#include "LwipSocketsFake.h"
#include "SolidSyslogErrorCategory.h"
#include "SolidSyslogLwipSocketAddress.h"
#include "SolidSyslogLwipSocketAddressPrivate.h"
#include "SolidSyslogLwipSocketTcpStream.h"
#include "SolidSyslogLwipSocketTcpStreamErrors.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogStream.h"
#include "SolidSyslogStreamCategories.h"
#include "SolidSyslogTunables.h"

/* The half of the pair for a stack built without LWIP_TCP_KEEPALIVE, where the
 * probe interval and count are the stack's own and only the idle period is
 * ours - and it is set in milliseconds, where the other variant's options take
 * seconds. The whole-file gate means only one of the two translation units
 * compiles per build, so each variant gets an executable of its own. */

static const struct SolidSyslogLwipSocketTcpStreamConfig config = {nullptr, nullptr};

static const int MILLISECONDS_PER_SECOND = 1000;

// clang-format off
TEST_GROUP(SolidSyslogLwipSocketTcpKeepaliveIdleOnly)
{
    struct SolidSyslogStream*  stream  = nullptr;
    struct SolidSyslogAddress* address = nullptr;

    void setup() override
    {
        LwipSocketsFake_Reset();
        stream  = SolidSyslogLwipSocketTcpStream_Create(&config);
        address = SolidSyslogLwipSocketAddress_Create();
        ErrorHandlerFake_Install(nullptr);
    }

    void teardown() override
    {
        SolidSyslogLwipSocketAddress_Destroy(address);
        SolidSyslogLwipSocketTcpStream_Destroy(stream);
    }

    [[nodiscard]] bool Open() const
    {
        return SolidSyslogStream_Open(stream, address);
    }
};

// clang-format on

TEST(SolidSyslogLwipSocketTcpKeepaliveIdleOnly, AStackWithNoProbeTimingsTakesTheIdlePeriodInMilliseconds)
{
    CHECK_TRUE(Open());

    CHECK_TRUE(LwipSocketsFake_SockOptWasSetTo(
        IPPROTO_TCP,
        TCP_KEEPALIVE,
        SOLIDSYSLOG_TCP_KEEPALIVE_IDLE_SECONDS * MILLISECONDS_PER_SECOND
    ));
}

TEST(SolidSyslogLwipSocketTcpKeepaliveIdleOnly, AnIdlePeriodTheStackRefusesIsReportedLikeAnyOtherOption)
{
    LwipSocketsFake_SetSockOptRefuses(IPPROTO_TCP, TCP_KEEPALIVE);

    CHECK_TRUE(Open());

    CHECK_ERROR_REPORTED_ONCE(
        SOLIDSYSLOG_SEVERITY_WARNING,
        &SolidSyslogLwipSocketTcpStreamErrorSource,
        SOLIDSYSLOG_CAT_STREAM_OPTION_REFUSED,
        SOLIDSYSLOG_TCP_STREAM_ERROR_SOCKET_OPTION_REFUSED
    );
}
