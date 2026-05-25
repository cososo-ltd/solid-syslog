#include <stdbool.h>

#include "BddTargetMtlsConfig.h"
#include "BddTargetTlsConfig.h"
#include "BddTargetTlsSender.h"
#include "SolidSyslogStreamSender.h"
#include "SolidSyslogTlsStream.h"
#include "SolidSyslogWindowsSleep.h"
#include "SolidSyslogWinsockAddress.h"
#include "SolidSyslogWinsockTcpStream.h"

struct SolidSyslogResolver;

static struct SolidSyslogStream* underlyingStream;

static struct SolidSyslogStream* tlsStream;
static struct SolidSyslogAddress* address;
static struct SolidSyslogSender* sender;

struct SolidSyslogSender* BddTargetTlsSender_Create(struct SolidSyslogResolver* resolver, bool mtls)
{
    underlyingStream = SolidSyslogWinsockTcpStream_Create(NULL);

    static struct SolidSyslogTlsStreamConfig tlsStreamConfig;
    tlsStreamConfig = (struct SolidSyslogTlsStreamConfig) {0};
    tlsStreamConfig.Transport = underlyingStream;
    tlsStreamConfig.Sleep = SolidSyslogWindowsSleep;
    if (mtls)
    {
        tlsStreamConfig.CaBundlePath = BddTargetMtlsConfig_GetCaBundlePath();
        tlsStreamConfig.ServerName = BddTargetMtlsConfig_GetServerName();
        tlsStreamConfig.ClientCertChainPath = BddTargetMtlsConfig_GetClientCertChainPath();
        tlsStreamConfig.ClientKeyPath = BddTargetMtlsConfig_GetClientKeyPath();
    }
    else
    {
        tlsStreamConfig.CaBundlePath = BddTargetTlsConfig_GetCaBundlePath();
        tlsStreamConfig.ServerName = BddTargetTlsConfig_GetServerName();
    }
    tlsStream = SolidSyslogTlsStream_Create(&tlsStreamConfig);

    address = SolidSyslogWinsockAddress_Create();

    static struct SolidSyslogStreamSenderConfig senderConfig;
    senderConfig = (struct SolidSyslogStreamSenderConfig) {0};
    senderConfig.Resolver = resolver;
    senderConfig.Stream = tlsStream;
    senderConfig.Address = address;
    senderConfig.Endpoint = mtls ? BddTargetMtlsConfig_GetEndpoint : BddTargetTlsConfig_GetEndpoint;
    senderConfig.EndpointVersion =
        mtls ? BddTargetMtlsConfig_GetEndpointVersion : BddTargetTlsConfig_GetEndpointVersion;
    sender = SolidSyslogStreamSender_Create(&senderConfig);

    return sender;
}

void BddTargetTlsSender_Destroy(void)
{
    SolidSyslogStreamSender_Destroy(sender);
    SolidSyslogWinsockAddress_Destroy(address);
    SolidSyslogTlsStream_Destroy(tlsStream);
    SolidSyslogWinsockTcpStream_Destroy(underlyingStream);
}
