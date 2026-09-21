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

/* The LWIP_TCP_KEEPALIVE=1 half of the pair. The whole-file gate means only one
 * of the two keepalive translation units compiles per build, so the variant
 * that the stream test executable does not carry needs its own executable. */

static const struct SolidSyslogLwipSocketTcpStreamConfig config = {nullptr, nullptr};

// clang-format off
TEST_GROUP(SolidSyslogLwipSocketTcpKeepalive)
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

    bool Open() const
    {
        return SolidSyslogStream_Open(stream, address);
    }
};

// clang-format on

TEST(SolidSyslogLwipSocketTcpKeepalive, AStackWithProbeTimingsTakesAllThreeInSeconds)
{
    CHECK_TRUE(Open());

    CHECK_TRUE(LwipSocketsFake_SockOptWasSetTo(IPPROTO_TCP, TCP_KEEPIDLE, SOLIDSYSLOG_TCP_KEEPALIVE_IDLE_SECONDS));
    CHECK_TRUE(
        LwipSocketsFake_SockOptWasSetTo(IPPROTO_TCP, TCP_KEEPINTVL, SOLIDSYSLOG_TCP_KEEPALIVE_INTERVAL_SECONDS)
    );
    CHECK_TRUE(LwipSocketsFake_SockOptWasSetTo(IPPROTO_TCP, TCP_KEEPCNT, SOLIDSYSLOG_TCP_KEEPALIVE_PROBE_COUNT));
}

TEST(SolidSyslogLwipSocketTcpKeepalive, AKeepaliveTimingTheStackRefusesIsReportedLikeAnyOtherOption)
{
    LwipSocketsFake_SetSockOptRefuses(IPPROTO_TCP, TCP_KEEPINTVL);

    CHECK_TRUE(Open());

    CHECK_ERROR_REPORTED_ONCE(
        SOLIDSYSLOG_SEVERITY_WARNING,
        &SolidSyslogLwipSocketTcpStreamErrorSource,
        SOLIDSYSLOG_CAT_STREAM_OPTION_REFUSED,
        SOLIDSYSLOG_TCP_STREAM_ERROR_SOCKET_OPTION_REFUSED
    );
}
