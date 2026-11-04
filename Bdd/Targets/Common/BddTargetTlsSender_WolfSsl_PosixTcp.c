#ifndef WOLFSSL_USER_SETTINGS
#include <wolfssl/options.h>
#endif
#include <stdbool.h>
#include <stddef.h>
#include <wolfssl/ssl.h>

#include "BddTargetMtlsConfig.h"
#include "BddTargetTlsConfig.h"
#include "BddTargetTlsSender.h"
#include "BddTargetWolfSslCredentials.h"
#include "SolidSyslogPosixAddress.h"
#include "SolidSyslogPosixSleep.h"
#include "SolidSyslogPosixTcpStream.h"
#include "SolidSyslogStreamSender.h"
#include "SolidSyslogWolfSslStream.h"
#include "SolidSyslogWolfSslStreamErrors.h"

struct SolidSyslogResolver;

static struct SolidSyslogStream* underlyingStream;

static struct SolidSyslogStream* tlsStream;
static struct SolidSyslogAddress* address;
static struct SolidSyslogSender* sender;

static void BddTargetTlsSender_TlsProfile(struct SolidSyslogWolfSslProfile* profile, void* context)
{
    (void) context;
    profile->ServerName = BddTargetTlsConfig_GetServerName();
}

static void BddTargetTlsSender_MtlsProfile(struct SolidSyslogWolfSslProfile* profile, void* context)
{
    (void) context;
    profile->ServerName = BddTargetMtlsConfig_GetServerName();
}

struct SolidSyslogSender* BddTargetTlsSender_Create(struct SolidSyslogResolver* resolver, bool mtls)
{
    /* The library is brought up here, once, as the integrator's own code
       would: the stream itself calls no process-global wolfSSL API. */
    (void) wolfSSL_Init();

    if (mtls)
    {
        /* The mutual-TLS transport presents a client credential by default. It
           goes through the same knob a scenario sets, so there is one place the
           credentials backend reads it from and a scenario can still override
           it - which is what the client-credential rotation cell does. */
        (void) BddTargetTlsConfig_SetByName("tls-client", "client");
    }

    underlyingStream = SolidSyslogPosixTcpStream_Create(NULL);

    static struct SolidSyslogWolfSslStreamConfig tlsStreamConfig;
    tlsStreamConfig = (struct SolidSyslogWolfSslStreamConfig) {0};
    tlsStreamConfig.Transport = underlyingStream;
    tlsStreamConfig.Sleep = SolidSyslogPosix_Sleep;
    tlsStreamConfig.Version = BddTargetTlsConfig_GetStreamVersion;
    tlsStreamConfig.Profile = mtls ? BddTargetTlsSender_MtlsProfile : BddTargetTlsSender_TlsProfile;
    tlsStreamConfig.Credentials = BddTargetWolfSslCredentials_Get();
    tlsStream = SolidSyslogWolfSslStream_Create(&tlsStreamConfig);

    address = SolidSyslogPosixAddress_Create();

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
    SolidSyslogPosixAddress_Destroy(address);
    SolidSyslogWolfSslStream_Destroy(tlsStream);
    SolidSyslogPosixTcpStream_Destroy(underlyingStream);
    (void) wolfSSL_Cleanup();
}

const struct SolidSyslogErrorSource* BddTargetTlsSender_ErrorSource(void)
{
    return &SolidSyslogWolfSslStreamErrorSource;
}
