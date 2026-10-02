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
const int UNTOUCHED = 0xA5;
} // namespace

// Built without LWIP_TCP_KEEPALIVE, so keep_intvl and keep_cnt are not pcb
// fields and the stack applies its own compile-time values for both. Only the
// idle period is the library's to set, in milliseconds.
TEST_GROUP(SolidSyslogLwipRawTcpKeepaliveIdleOnly)
{
    struct tcp_pcb pcb;

    void setup() override
    {
        memset(&pcb, UNTOUCHED, sizeof(pcb));
    }
};

TEST(SolidSyslogLwipRawTcpKeepaliveIdleOnly, SetsKeepIdleFromTheTunableInMilliseconds)
{
    SolidSyslogLwipRawTcpStream_ApplyKeepalive(&pcb);

    LONGS_EQUAL(SOLIDSYSLOG_TCP_KEEPALIVE_IDLE_SECONDS * MILLISECONDS_PER_SECOND, pcb.keep_idle);
}

TEST(SolidSyslogLwipRawTcpKeepaliveIdleOnly, LeavesTheRestOfThePcbToTheStack)
{
    struct tcp_pcb expected;
    memset(&expected, UNTOUCHED, sizeof(expected));
    expected.keep_idle = SOLIDSYSLOG_TCP_KEEPALIVE_IDLE_SECONDS * MILLISECONDS_PER_SECOND;

    SolidSyslogLwipRawTcpStream_ApplyKeepalive(&pcb);

    MEMCMP_EQUAL(&expected, &pcb, sizeof(pcb));
}
