/* The FreeRTOS-Plus-TCP byte transport under the BDD target's TLS stream.
 *
 * Pack half of the seam BddTargetTlsSender_MbedTls.c calls; everything else
 * about that sender is shared. */

#include "BddTargetTlsInnerStream.h"

#include "SolidSyslogPlusTcpAddress.h"
#include "SolidSyslogPlusTcpTcpStream.h"

#include <stddef.h>

struct SolidSyslogStream* BddTargetTlsInnerStream_CreateStream(void)
{
    /* NULL config: the connect timeout comes from the
     * SOLIDSYSLOG_TCP_CONNECT_TIMEOUT_MS tunable, and this stream needs nothing
     * else decided for it. */
    return SolidSyslogPlusTcpTcpStream_Create(NULL);
}

void BddTargetTlsInnerStream_DestroyStream(struct SolidSyslogStream* stream)
{
    SolidSyslogPlusTcpTcpStream_Destroy(stream);
}

struct SolidSyslogAddress* BddTargetTlsInnerStream_CreateAddress(void)
{
    return SolidSyslogPlusTcpAddress_Create();
}

void BddTargetTlsInnerStream_DestroyAddress(struct SolidSyslogAddress* address)
{
    SolidSyslogPlusTcpAddress_Destroy(address);
}
