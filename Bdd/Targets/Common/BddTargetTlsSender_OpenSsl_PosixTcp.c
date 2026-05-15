#include <stdbool.h>

#include "BddTargetMtlsConfig.h"
#include "BddTargetTlsConfig.h"
#include "BddTargetTlsSender.h"
#include "SolidSyslogPosixSleep.h"
#include "SolidSyslogPosixTcpStream.h"
#include "SolidSyslogStreamSender.h"
#include "SolidSyslogTlsStream.h"

struct SolidSyslogResolver;

static SolidSyslogPosixTcpStreamStorage underlyingStreamStorage;
static struct SolidSyslogStream* underlyingStream;
static SolidSyslogTlsStreamStorage tlsStreamStorage;
static struct SolidSyslogStream* tlsStream;
static SolidSyslogStreamSenderStorage senderStorage;
static struct SolidSyslogSender* sender;

struct SolidSyslogSender* BddTargetTlsSender_Create(struct SolidSyslogResolver* resolver, bool mtls)
{
    underlyingStream = SolidSyslogPosixTcpStream_Create(&underlyingStreamStorage);

    static struct SolidSyslogTlsStreamConfig tlsStreamConfig;
    tlsStreamConfig = (struct SolidSyslogTlsStreamConfig) {0};
    tlsStreamConfig.Transport = underlyingStream;
    tlsStreamConfig.Sleep = SolidSyslogPosixSleep;
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
    tlsStream = SolidSyslogTlsStream_Create(&tlsStreamStorage, &tlsStreamConfig);

    static struct SolidSyslogStreamSenderConfig senderConfig;
    senderConfig = (struct SolidSyslogStreamSenderConfig) {0};
    senderConfig.Resolver = resolver;
    senderConfig.Stream = tlsStream;
    senderConfig.Endpoint = mtls ? BddTargetMtlsConfig_GetEndpoint : BddTargetTlsConfig_GetEndpoint;
    senderConfig.EndpointVersion =
        mtls ? BddTargetMtlsConfig_GetEndpointVersion : BddTargetTlsConfig_GetEndpointVersion;
    sender = SolidSyslogStreamSender_Create(&senderStorage, &senderConfig);

    return sender;
}

void BddTargetTlsSender_Destroy(void)
{
    SolidSyslogStreamSender_Destroy(sender);
    SolidSyslogTlsStream_Destroy(tlsStream);
    SolidSyslogPosixTcpStream_Destroy(underlyingStream);
}
