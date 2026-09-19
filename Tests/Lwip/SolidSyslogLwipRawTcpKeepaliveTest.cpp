#include "lwip/opt.h"
#include "lwip/tcp.h"

#include <cstring>

#include "CppUTest/TestHarness.h"

extern "C"
{
#include "SolidSyslogLwipRawTcpStreamPrivate.h"
#include "SolidSyslogTunables.h"
}

namespace
{
const u32_t MILLISECONDS_PER_SECOND = 1000;
}

// Built with LWIP_TCP_KEEPALIVE=1, so keep_intvl and keep_cnt are pcb fields
// and all three timings are the library's to set. The variant for a build
// without it is exercised through SolidSyslogLwipRawTcpStreamTest.
TEST_GROUP(SolidSyslogLwipRawTcpKeepalive)
{
    struct tcp_pcb pcb;

    void setup() override
    {
        memset(&pcb, 0, sizeof(pcb));
    }
};

TEST(SolidSyslogLwipRawTcpKeepalive, SetsKeepIdleFromTheTunable)
{
    SolidSyslogLwipRawTcpStream_ApplyKeepalive(&pcb);

    LONGS_EQUAL(SOLIDSYSLOG_TCP_KEEPALIVE_IDLE_SECONDS * MILLISECONDS_PER_SECOND, pcb.keep_idle);
}

TEST(SolidSyslogLwipRawTcpKeepalive, SetsKeepIntervalFromTheTunable)
{
    SolidSyslogLwipRawTcpStream_ApplyKeepalive(&pcb);

    LONGS_EQUAL(SOLIDSYSLOG_TCP_KEEPALIVE_INTERVAL_SECONDS * MILLISECONDS_PER_SECOND, pcb.keep_intvl);
}

TEST(SolidSyslogLwipRawTcpKeepalive, SetsProbeCountFromTheTunable)
{
    SolidSyslogLwipRawTcpStream_ApplyKeepalive(&pcb);

    LONGS_EQUAL(SOLIDSYSLOG_TCP_KEEPALIVE_PROBE_COUNT, pcb.keep_cnt);
}
