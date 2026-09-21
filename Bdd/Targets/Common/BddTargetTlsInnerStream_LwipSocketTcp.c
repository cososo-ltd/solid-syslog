/* The lwIP Sockets API byte transport under the BDD target's TLS stream.
 *
 * Pack half of the seam BddTargetTlsSender_MbedTls.c calls; everything else
 * about that sender is shared. */

#include "BddTargetTlsInnerStream.h"

#include "SolidSyslogLwipSocketAddress.h"
#include "SolidSyslogLwipSocketTcpStream.h"

#include <stddef.h>

struct SolidSyslogStream* BddTargetTlsInnerStream_CreateStream(void)
{
    /* NULL config: the connect deadline comes from the
     * SOLIDSYSLOG_TCP_CONNECT_TIMEOUT_MS tunable, and the stream waits on the
     * socket itself rather than on a sleep this target would have to supply. */
    return SolidSyslogLwipSocketTcpStream_Create(NULL);
}

void BddTargetTlsInnerStream_DestroyStream(struct SolidSyslogStream* stream)
{
    SolidSyslogLwipSocketTcpStream_Destroy(stream);
}

struct SolidSyslogAddress* BddTargetTlsInnerStream_CreateAddress(void)
{
    return SolidSyslogLwipSocketAddress_Create();
}

void BddTargetTlsInnerStream_DestroyAddress(struct SolidSyslogAddress* address)
{
    SolidSyslogLwipSocketAddress_Destroy(address);
}
