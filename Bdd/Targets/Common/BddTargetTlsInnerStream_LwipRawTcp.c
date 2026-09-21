/* The lwIP Raw API byte transport under the BDD target's TLS stream.
 *
 * Pack half of the seam BddTargetTlsSender_MbedTls.c calls; everything else
 * about that sender is shared. */

#include "BddTargetTlsInnerStream.h"

#include "BddTargetOsPrimitives.h"
#include "SolidSyslogLwipRawAddress.h"
#include "SolidSyslogLwipRawTcpStream.h"

#include <stddef.h>

struct SolidSyslogStream* BddTargetTlsInnerStream_CreateStream(void)
{
    /* The shared sleep drives the bounded synchronous-connect spin; the connect
     * timeout comes from the SOLIDSYSLOG_TCP_CONNECT_TIMEOUT_MS tunable
     * (GetConnectTimeoutMs NULL). All lwIP-core touches inside the adapter are
     * marshalled onto the tcpip thread via the SolidSyslogLwipRaw_SetMarshal hop
     * main.c installs. */
    static struct SolidSyslogLwipRawTcpStreamConfig config;
    config = (struct SolidSyslogLwipRawTcpStreamConfig) {0};
    config.GetConnectTimeoutMs = NULL;
    config.ConnectTimeoutContext = NULL;
    config.Sleep = BddTargetOsPrimitives_Sleep;
    return SolidSyslogLwipRawTcpStream_Create(&config);
}

void BddTargetTlsInnerStream_DestroyStream(struct SolidSyslogStream* stream)
{
    SolidSyslogLwipRawTcpStream_Destroy(stream);
}

struct SolidSyslogAddress* BddTargetTlsInnerStream_CreateAddress(void)
{
    return SolidSyslogLwipRawAddress_Create();
}

void BddTargetTlsInnerStream_DestroyAddress(struct SolidSyslogAddress* address)
{
    SolidSyslogLwipRawAddress_Destroy(address);
}
