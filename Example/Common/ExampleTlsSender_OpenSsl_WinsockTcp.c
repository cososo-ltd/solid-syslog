#include <stdbool.h>

#include "ExampleMtlsConfig.h"
#include "ExampleTlsConfig.h"
#include "ExampleTlsSender.h"
#include "SolidSyslogStreamSender.h"
#include "SolidSyslogTlsStream.h"
#include "SolidSyslogWindowsSleep.h"
#include "SolidSyslogWinsockTcpStream.h"

struct SolidSyslogResolver;

static SolidSyslogWinsockTcpStreamStorage underlyingStreamStorage;
static struct SolidSyslogStream*          underlyingStream;
static SolidSyslogTlsStreamStorage        tlsStreamStorage;
static struct SolidSyslogStream*          tlsStream;
static SolidSyslogStreamSenderStorage     senderStorage;
static struct SolidSyslogSender*          sender;

struct SolidSyslogSender* ExampleTlsSender_Create(struct SolidSyslogResolver* resolver, bool mtls)
{
    underlyingStream = SolidSyslogWinsockTcpStream_Create(&underlyingStreamStorage);

    static struct SolidSyslogTlsStreamConfig tlsStreamConfig;
    tlsStreamConfig           = (struct SolidSyslogTlsStreamConfig) {0};
    tlsStreamConfig.transport = underlyingStream;
    tlsStreamConfig.sleep     = SolidSyslogWindowsSleep;
    if (mtls)
    {
        tlsStreamConfig.caBundlePath        = ExampleMtlsConfig_GetCaBundlePath();
        tlsStreamConfig.serverName          = ExampleMtlsConfig_GetServerName();
        tlsStreamConfig.clientCertChainPath = ExampleMtlsConfig_GetClientCertChainPath();
        tlsStreamConfig.clientKeyPath       = ExampleMtlsConfig_GetClientKeyPath();
    }
    else
    {
        tlsStreamConfig.caBundlePath = ExampleTlsConfig_GetCaBundlePath();
        tlsStreamConfig.serverName   = ExampleTlsConfig_GetServerName();
    }
    tlsStream = SolidSyslogTlsStream_Create(&tlsStreamStorage, &tlsStreamConfig);

    static struct SolidSyslogStreamSenderConfig senderConfig;
    senderConfig                 = (struct SolidSyslogStreamSenderConfig) {0};
    senderConfig.resolver        = resolver;
    senderConfig.stream          = tlsStream;
    senderConfig.endpoint        = mtls ? ExampleMtlsConfig_GetEndpoint : ExampleTlsConfig_GetEndpoint;
    senderConfig.endpointVersion = mtls ? ExampleMtlsConfig_GetEndpointVersion : ExampleTlsConfig_GetEndpointVersion;
    sender                       = SolidSyslogStreamSender_Create(&senderStorage, &senderConfig);

    return sender;
}

void ExampleTlsSender_Destroy(void)
{
    SolidSyslogStreamSender_Destroy(sender);
    SolidSyslogTlsStream_Destroy(tlsStream);
    SolidSyslogWinsockTcpStream_Destroy(underlyingStream);
}
