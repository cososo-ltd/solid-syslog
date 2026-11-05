/* The wolfSSL BDD TLS sender for a target that builds wolfSSL from source.
 *
 * Composes:
 *   - BddTargetTlsInnerStream       inner TCP transport, one file per network pack
 *   - SolidSyslogWolfSslStream      TLS over the injected Stream
 *   - SolidSyslogStreamSender       RFC 6587 octet-counting framing
 *
 * wolfSSL allocates from one static region this file owns. Its heap hint is
 * the stream's Heap, and is also installed as wolfSSL's global hint for the
 * wolfCrypt calls that are given none - the at-rest policies and their RNG.
 * Both are process-global decisions, so the target takes them and the library
 * does not. */

#include "BddTargetTlsSender.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <wolfssl/wolfcrypt/settings.h>
#include <wolfssl/ssl.h>
#include <wolfssl/wolfcrypt/memory.h>

#include "BddTargetMtlsConfig.h"
#include "BddTargetOsPrimitives.h"
#include "BddTargetSwitchConfig.h"
#include "BddTargetTlsConfig.h"
#include "BddTargetTlsInnerStream.h"
#include "BddTargetWolfSslCredentials.h"
#include "BddTargetWolfSslPort.h"
#include "SolidSyslogNullSender.h"
#include "SolidSyslogStream.h"
#include "SolidSyslogStreamSender.h"
#include "SolidSyslogWolfSslStream.h"
#include "SolidSyslogWolfSslStreamErrors.h"

struct SolidSyslogResolver;

/* Sized by trial against wolfSSL's default bucket sizes, not tuned: 96 KiB is
   too little for a handshake with this collector and 128 KiB carries the
   whole lane. */
#ifndef BDD_TARGET_WOLFSSL_MEMORY_BYTES
#define BDD_TARGET_WOLFSSL_MEMORY_BYTES (128U * 1024U)
#endif

static unsigned char wolfSslMemory[BDD_TARGET_WOLFSSL_MEMORY_BYTES];
static WOLFSSL_HEAP_HINT* wolfSslHeap;
static bool wolfSslInitialised;

static struct SolidSyslogStream* underlyingStream;
static struct SolidSyslogStream* tlsStream;
static struct SolidSyslogAddress* address;
static struct SolidSyslogSender* sender;

static void EnsureWolfSslInitialised(void);
static void EnsureWolfSslMemoryLoaded(void);
static void DispatchEndpoint(struct SolidSyslogEndpoint* endpoint, void* context);
static uint32_t DispatchEndpointVersion(void* context);
static void BddTargetTlsSender_Profile(struct SolidSyslogWolfSslProfile* profile, void* context);
static void BddTargetTlsSender_ApplyCipherPolicy(struct SolidSyslogWolfSslProfile* profile);

struct SolidSyslogSender* BddTargetTlsSender_Create(struct SolidSyslogResolver* resolver, bool mtls)
{
    /* Both TLS and mutual TLS share one Switching slot on this target, and
       `set transport mtls` arrives after this call, so the choice is made per
       connection by the dispatchers below and by the credentials. */
    (void) mtls;

    EnsureWolfSslInitialised();
    if (!wolfSslInitialised)
    {
        return SolidSyslogNullSender_Get();
    }

    underlyingStream = BddTargetTlsInnerStream_CreateStream();

    static struct SolidSyslogWolfSslStreamConfig tlsStreamConfig;
    tlsStreamConfig = (struct SolidSyslogWolfSslStreamConfig) {0};
    tlsStreamConfig.Transport = underlyingStream;
    tlsStreamConfig.Sleep = BddTargetOsPrimitives_Sleep;
    tlsStreamConfig.Heap = wolfSslHeap;
    tlsStreamConfig.Version = DispatchEndpointVersion;
    tlsStreamConfig.Profile = BddTargetTlsSender_Profile;
    tlsStreamConfig.Credentials = BddTargetWolfSslCredentials_Get();
    tlsStream = SolidSyslogWolfSslStream_Create(&tlsStreamConfig);

    address = BddTargetTlsInnerStream_CreateAddress();

    static struct SolidSyslogStreamSenderConfig senderConfig;
    senderConfig = (struct SolidSyslogStreamSenderConfig) {0};
    senderConfig.Resolver = resolver;
    senderConfig.Stream = tlsStream;
    senderConfig.Address = address;
    senderConfig.Endpoint = DispatchEndpoint;
    senderConfig.EndpointVersion = DispatchEndpointVersion;
    sender = SolidSyslogStreamSender_Create(&senderConfig);

    return sender;
}

/* The library is brought up here, once, as the integrator's own code would:
   the stream calls no process-global wolfSSL API. The region is loaded before
   wolfSSL_Init so nothing is ever asked for with nowhere to come from. */
static void EnsureWolfSslInitialised(void)
{
    if (!wolfSslInitialised)
    {
        EnsureWolfSslMemoryLoaded();
        if (wolfSslHeap != NULL)
        {
            int initialised = wolfSSL_Init();
            if (initialised != WOLFSSL_SUCCESS)
            {
                (void) printf("[wolfssl] wolfSSL_Init FAILED rc=%d; TLS slot will be unusable\r\n", initialised);
            }
            wolfSslInitialised = initialised == WOLFSSL_SUCCESS;
        }
    }
}

/* Once only, however often bring-up is retried: loading the region a second
   time would rebuild the pool under whatever was already drawn from it. */
static void EnsureWolfSslMemoryLoaded(void)
{
    if (wolfSslHeap == NULL)
    {
        BddTargetWolfSslPort_Initialise();
        int loaded = wc_LoadStaticMemory(&wolfSslHeap, wolfSslMemory, sizeof(wolfSslMemory), WOLFMEM_GENERAL, 1);
        if (loaded != 0)
        {
            (void) printf("[wolfssl] static memory load FAILED rc=%d; TLS slot will be unusable\r\n", loaded);
            wolfSslHeap = NULL;
        }
        else
        {
            (void) wolfSSL_SetGlobalHeapHint(wolfSslHeap);
        }
    }
}

void BddTargetTlsSender_Destroy(void)
{
    /* A failed bring-up handed back the shared NullSender and built nothing. */
    if (sender != NULL)
    {
        SolidSyslogStreamSender_Destroy(sender);
        BddTargetTlsInnerStream_DestroyAddress(address);
        SolidSyslogWolfSslStream_Destroy(tlsStream);
        BddTargetTlsInnerStream_DestroyStream(underlyingStream);
        sender = NULL;
        /* The library and its region survive Destroy -> Create cycles; real
           teardown is process exit, which this target never reaches. */
    }
}

/* tls and mtls share one stream and differ only in the destination port, which
   the harness selects over the UART after Create has run. */
static void DispatchEndpoint(struct SolidSyslogEndpoint* endpoint, void* context)
{
    if (BddTargetSwitchConfig_IsMtlsMode())
    {
        BddTargetMtlsConfig_GetEndpoint(endpoint, context);
    }
    else
    {
        BddTargetTlsConfig_GetEndpoint(endpoint, context);
    }
}

static uint32_t DispatchEndpointVersion(void* context)
{
    return BddTargetSwitchConfig_IsMtlsMode() ? BddTargetMtlsConfig_GetEndpointVersion(context)
                                              : BddTargetTlsConfig_GetEndpointVersion(context);
}

static void BddTargetTlsSender_Profile(struct SolidSyslogWolfSslProfile* profile, void* context)
{
    (void) context;
    profile->ServerName = BddTargetTlsConfig_GetServerName();
    BddTargetTlsSender_ApplyCipherPolicy(profile);
}

/* One intent token, one wolfSSL list. wolfSSL covers both TLS versions with a
   single list, so the offered policy names its suite at either version. The
   unoffered one names a suite this build compiles in and no collector here can
   serve - its certificate is RSA - and names nothing for TLS 1.3, so nothing
   is offered there either. A suite the build lacked would be refused as bad
   configuration before the handshake, which is a different cell. */
static void BddTargetTlsSender_ApplyCipherPolicy(struct SolidSyslogWolfSslProfile* profile)
{
    const char* policy = BddTargetTlsConfig_GetCipherPolicyName();
    if (strcmp(policy, "offered") == 0)
    {
        profile->CipherList = "ECDHE-RSA-AES128-GCM-SHA256:TLS13-AES128-GCM-SHA256";
    }
    else if (strcmp(policy, "unoffered") == 0)
    {
        profile->CipherList = "ECDHE-ECDSA-AES128-GCM-SHA256";
    }
    else
    {
        /* "default" - whatever the build enables, which is what an integrator
           who supplies no policy gets. */
    }
}

const struct SolidSyslogErrorSource* BddTargetTlsSender_ErrorSource(void)
{
    return &SolidSyslogWolfSslStreamErrorSource;
}
