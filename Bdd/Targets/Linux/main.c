#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>

#include "BddTargetAppName.h"
#include "BddTargetCommandLine.h"
#include "BddTargetEnterpriseId.h"
#include "BddTargetInteractive.h"
#include "BddTargetIps.h"
#include "BddTargetKey.h"
#include "BddTargetLanguage.h"
#include "BddTargetMtlsConfig.h"
#include "BddTargetServiceThread.h"
#include "BddTargetStderrErrorHandler.h"
#include "BddTargetStoreSettings.h"
#include "BddTargetSwitchConfig.h"
#include "BddTargetTcpConfig.h"
#include "BddTargetTlsConfig.h"
#include "BddTargetTlsSender.h"
#include "BddTargetUdpConfig.h"
#include "SolidSyslog.h"
#include "SolidSyslogStdAtomicCounter.h"
#include "SolidSyslogTunables.h"
#include "SolidSyslogConfig.h"
#include "SolidSyslogCrc16Policy.h"
#include "SolidSyslogNullSecurityPolicy.h"
#include "SolidSyslogOpenSslHmacSha256Policy.h"
#include "SolidSyslogOpenSslAesGcmPolicy.h"
#include "SolidSyslogFileBlockDevice.h"
#include "SolidSyslogBlockStore.h"
#include "SolidSyslogMetaSd.h"
#include "SolidSyslogOriginSd.h"
#include "SolidSyslogTimeQualitySd.h"
#include "SolidSyslogNullStore.h"
#include "SolidSyslogPosixClock.h"
#include "SolidSyslogPosixFile.h"
#include "SolidSyslogPosixHostname.h"
#include "SolidSyslogPosixMessageQueueBuffer.h"
#include "SolidSyslogPosixMutex.h"
#include "SolidSyslogPosixProcessId.h"
#include "SolidSyslogPosixSleep.h"
#include "SolidSyslogPosixSysUpTime.h"
#include "SolidSyslogPosixResolver.h"
#include "SolidSyslogPosixAddress.h"
#include "SolidSyslogPosixDatagram.h"
#include "SolidSyslogPosixTcpStream.h"
#include "SolidSyslogSwitchingSender.h"
#include "SolidSyslogStreamSender.h"
#include "SolidSyslogUdpSender.h"
#include "SolidSyslogTimeQuality.h"

struct SolidSyslogStore;

static const char* const STORE_PATH_PREFIX = "/tmp/STORE";
static const char* const THRESHOLD_MARKER_PATH = "/tmp/solidsyslog_threshold_marker.log";
static struct SolidSyslogFile* storeFile;
static struct SolidSyslogBlockDevice* storeBlockDevice;
static struct SolidSyslogStream* plainTcpStream;
static struct SolidSyslogAddress* plainTcpAddress;
static struct SolidSyslogSender* plainTcpSender;
static struct SolidSyslogSender* udpSender;
static struct SolidSyslogSender* switchingSender;
static struct SolidSyslogDatagram* udpDatagram;
static struct SolidSyslogAddress* udpAddress;
static struct SolidSyslogResolver* sharedResolver;
/* Holds the created at-rest SecurityPolicy handle so DestroyStore can release
 * it. The keyed policies (hmac-sha256, aes-256-gcm) need the handle to destroy;
 * crc16 is dispatched by name to its singleton Destroy, and null needs no
 * cleanup. */
static struct SolidSyslogSecurityPolicy* securityPolicy;

static void GetTimeQuality(struct SolidSyslogTimeQuality* timeQuality)
{
    timeQuality->TzKnown = true;
    timeQuality->IsSynced = true;
    timeQuality->SyncAccuracyMicroseconds = SOLIDSYSLOG_SYNC_ACCURACY_OMIT;
}

static volatile bool shutdown_flag;
static struct SolidSyslog* solidSyslog;

static void* ServiceThreadEntry(void* arg)
{
    volatile bool* shutdown = (volatile bool*) arg;
    BddTargetServiceThread_Run(solidSyslog, shutdown, SolidSyslogPosix_Sleep);
    return NULL;
}

static struct SolidSyslogSender* CreateSender(const struct BddTargetOptions* options)
{
    bool mtlsModeActive = (strcmp(options->Transport, "mtls") == 0);

    sharedResolver = SolidSyslogPosixResolver_Create();
    struct SolidSyslogResolver* resolver = sharedResolver;

    udpDatagram = SolidSyslogPosixDatagram_Create();
    udpAddress = SolidSyslogPosixAddress_Create();
    static struct SolidSyslogUdpSenderConfig udpConfig = {0};
    udpConfig.Resolver = resolver;
    udpConfig.Datagram = udpDatagram;
    udpConfig.Address = udpAddress;
    udpConfig.Endpoint = BddTargetUdpConfig_GetEndpoint;
    udpConfig.EndpointVersion = BddTargetUdpConfig_GetEndpointVersion;
    udpSender = SolidSyslogUdpSender_Create(&udpConfig);

    plainTcpStream = SolidSyslogPosixTcpStream_Create(NULL);
    plainTcpAddress = SolidSyslogPosixAddress_Create();
    static struct SolidSyslogStreamSenderConfig tcpConfig = {0};
    tcpConfig.Resolver = resolver;
    tcpConfig.Stream = plainTcpStream;
    tcpConfig.Address = plainTcpAddress;
    tcpConfig.Endpoint = BddTargetTcpConfig_GetEndpoint;
    tcpConfig.EndpointVersion = BddTargetTcpConfig_GetEndpointVersion;
    plainTcpSender = SolidSyslogStreamSender_Create(&tcpConfig);

    struct SolidSyslogSender* tlsSender = BddTargetTlsSender_Create(resolver, mtlsModeActive);

    static struct SolidSyslogSender* inners[BDD_TARGET_SWITCH_COUNT];
    inners[BDD_TARGET_SWITCH_UDP] = udpSender;
    inners[BDD_TARGET_SWITCH_TCP] = plainTcpSender;
    inners[BDD_TARGET_SWITCH_TLS] = tlsSender;

    static struct SolidSyslogSwitchingSenderConfig switchConfig = {0};
    switchConfig.Senders = inners;
    switchConfig.SenderCount = BDD_TARGET_SWITCH_COUNT;
    switchConfig.Selector = BddTargetSwitchConfig_Selector;

    BddTargetSwitchConfig_SetByName(options->Transport);
    switchingSender = SolidSyslogSwitchingSender_Create(&switchConfig);
    return switchingSender;
}

static void OnHalt(void* context)
{
    (void) context;
    if (BddTargetStoreSettings_HaltExit())
    {
        _exit(2);
    }
}

static void OnThresholdCrossed(void* context)
{
    (void) context;
    FILE* fp = fopen(THRESHOLD_MARKER_PATH, "a");
    if (fp != NULL)
    {
        fputs("crossed\n", fp);
        fclose(fp);
    }
}

static struct SolidSyslogSecurityPolicy* CreateSecurityPolicy(void)
{
    struct SolidSyslogSecurityPolicy* policy = NULL;
    enum BddTargetSecurityPolicy kind = BddTargetStoreSettings_SecurityPolicy();
    if (kind == BDD_TARGET_SECURITY_POLICY_HMAC_SHA256)
    {
        static const struct SolidSyslogOpenSslHmacSha256PolicyConfig hmacConfig = {BddTargetKey_Get, NULL};
        policy = SolidSyslogOpenSslHmacSha256Policy_Create(&hmacConfig);
    }
    else if (kind == BDD_TARGET_SECURITY_POLICY_AES_256_GCM)
    {
        static const struct SolidSyslogOpenSslAesGcmPolicyConfig aesConfig = {BddTargetKey_Get, NULL};
        policy = SolidSyslogOpenSslAesGcmPolicy_Create(&aesConfig);
    }
    else if (kind == BDD_TARGET_SECURITY_POLICY_NULL)
    {
        policy = SolidSyslogNullSecurityPolicy_Get();
    }
    else
    {
        policy = SolidSyslogCrc16Policy_Create();
    }
    return policy;
}

static struct SolidSyslogStore* CreateStore(const struct BddTargetOptions* options)
{
    bool useFile = (strcmp(options->Store, "file") == 0);

    if (useFile)
    {
        storeFile = SolidSyslogPosixFile_Create();

        storeBlockDevice =
            SolidSyslogFileBlockDevice_Create(storeFile, STORE_PATH_PREFIX, BddTargetStoreSettings_MaxBlockSize());

        securityPolicy = CreateSecurityPolicy();
        static struct SolidSyslogBlockStoreConfig storeConfig = {0};
        storeConfig.BlockDevice = storeBlockDevice;
        storeConfig.MaxBlocks = BddTargetStoreSettings_MaxBlocks();
        storeConfig.GetDiscardPolicy = BddTargetStoreSettings_GetDiscardPolicy;
        storeConfig.SecurityPolicy = securityPolicy;
        storeConfig.GetCapacityThreshold = BddTargetStoreSettings_GetCapacityThreshold;
        storeConfig.OnThresholdCrossed = OnThresholdCrossed;
        storeConfig.ThresholdContext = NULL;

        return SolidSyslogBlockStore_Create(&storeConfig);
    }

    return SolidSyslogNullStore_Get();
}

static void DestroySender(void)
{
    SolidSyslogSwitchingSender_Destroy(switchingSender);
    BddTargetTlsSender_Destroy();
    SolidSyslogStreamSender_Destroy(plainTcpSender);
    SolidSyslogPosixAddress_Destroy(plainTcpAddress);
    SolidSyslogPosixTcpStream_Destroy(plainTcpStream);
    SolidSyslogUdpSender_Destroy(udpSender);
    SolidSyslogPosixAddress_Destroy(udpAddress);
    SolidSyslogPosixDatagram_Destroy(udpDatagram);
    SolidSyslogPosixResolver_Destroy(sharedResolver);
}

/* The settings are fixed once the command line is parsed, so the policy to
 * destroy is the one the same setting named at create. */
static void DestroySecurityPolicy(void)
{
    enum BddTargetSecurityPolicy kind = BddTargetStoreSettings_SecurityPolicy();
    if (kind == BDD_TARGET_SECURITY_POLICY_HMAC_SHA256)
    {
        SolidSyslogOpenSslHmacSha256Policy_Destroy(securityPolicy);
    }
    else if (kind == BDD_TARGET_SECURITY_POLICY_AES_256_GCM)
    {
        SolidSyslogOpenSslAesGcmPolicy_Destroy(securityPolicy);
    }
    else if (kind == BDD_TARGET_SECURITY_POLICY_CRC16)
    {
        SolidSyslogCrc16Policy_Destroy();
    }
    /* else "null": the shared NullSecurityPolicy is immutable - nothing to free. */
    securityPolicy = NULL;
}

static void DestroyStore(struct SolidSyslogStore* store, const struct BddTargetOptions* options)
{
    bool useFile = (strcmp(options->Store, "file") == 0);

    if (useFile)
    {
        SolidSyslogBlockStore_Destroy(store);
        SolidSyslogFileBlockDevice_Destroy(storeBlockDevice);
        DestroySecurityPolicy();
        SolidSyslogPosixFile_Destroy(storeFile);
    }
    /* else: NullStore is shared and immutable - nothing to destroy. */
}

/* Three owners of `set` names on this target, tried in turn: the error
   handler's own knob, the demo key, then everything BddTargetTlsConfig owns. */
static bool OnSet(const char* name, const char* value)
{
    bool applied = BddTargetStderrErrorHandler_SetByName(name, value);
    if (!applied)
    {
        applied = BddTargetKey_SetByName(name, value);
    }
    if (!applied)
    {
        applied = BddTargetTlsConfig_SetByName(name, value);
    }
    return applied;
}

int main(int argc, char* argv[])
{
    BddTargetStderrErrorHandler_Install();
    BddTargetStderrErrorHandler_SetTlsSource(BddTargetTlsSender_ErrorSource());

    /* BDD harness can override the TLS/mTLS host (defaults to "syslog-ng",
       the Linux compose service name). Same env-var contract as the Windows
       example so behave can target either oracle. */
    BddTargetTlsConfig_SetHost(getenv("SOLIDSYSLOG_BDD_TLS_HOST"));
    BddTargetMtlsConfig_SetHost(getenv("SOLIDSYSLOG_BDD_MTLS_HOST"));

    struct BddTargetOptions options;
    if (BddTargetCommandLine_Parse(argc, argv, &options) != 0)
    {
        return 1;
    }

    /* Before the store is created, so a scenario expecting an ERROR from its
       Create can say so - a `set` at the prompt arrives too late for that. */
    (void) BddTargetStderrErrorHandler_SetByName("errors-fatal", options.ErrorsFatal);

    /* Honour --app-name when supplied (BDD scenarios pin it for record-size
       parity across runners); otherwise derive from argv[0] as before. */
    BddTargetAppName_Set((options.AppName != NULL) ? options.AppName : argv[0]);

    struct SolidSyslogSender* sender = CreateSender(&options);
    struct SolidSyslogStore* store = CreateStore(&options);

    /* The interactive thread logs while the service thread drains. */
    struct SolidSyslogMutex* bufferMutex = SolidSyslogPosixMutex_Create();
    struct SolidSyslogBuffer* buffer = SolidSyslogPosixMessageQueueBuffer_Create(
        bufferMutex,
        SOLIDSYSLOG_MAX_MESSAGE_SIZE,
        10,
        BddTargetStoreSettings_GetDiscardPolicy,
        NULL
    );
    struct SolidSyslogAtomicCounter* counter = SolidSyslogStdAtomicCounter_Create();
    struct SolidSyslogMetaSdConfig metaConfig = {
        .Counter = counter,
        .GetSysUpTime = SolidSyslogPosix_GetSysUpTime,
        .GetLanguage = BddTargetLanguage_Get,
    };
    struct SolidSyslogStructuredData* metaSd = SolidSyslogMetaSd_Create(&metaConfig);

    struct SolidSyslogStructuredData* timeQuality = SolidSyslogTimeQualitySd_Create(GetTimeQuality);
    struct SolidSyslogOriginSdConfig originConfig = {
        .Software = "SolidSyslogBddTarget",
        .SwVersion = "0.7.0",
        .EnterpriseId = BDD_TARGET_ENTERPRISE_ID,
        .GetIpCount = BddTargetIps_Count,
        .GetIpAt = BddTargetIps_At,
    };
    struct SolidSyslogStructuredData* originSd = SolidSyslogOriginSd_Create(&originConfig);

    struct SolidSyslogStructuredData* sdList[3] = {metaSd, timeQuality, originSd};
    size_t sdCount = BddTargetStoreSettings_NoSd() ? 1 : 3;

    struct SolidSyslogConfig config = {
        .Buffer = buffer,
        .Sender = sender,
        .Clock = SolidSyslogPosix_GetTimestamp,
        .GetHostname = SolidSyslogPosix_GetHostname,
        .GetAppName = BddTargetAppName_Get,
        .GetProcessId = SolidSyslogPosix_GetProcessId,
        .Store = store,
        .Sd = sdList,
        .SdCount = sdCount,
        .GetDiscardPolicy = BddTargetStoreSettings_GetDiscardPolicy,
        .OnHalt = OnHalt,
    };
    solidSyslog = SolidSyslog_Create(&config);

    shutdown_flag = false;

    pthread_t serviceThread = 0;
    pthread_create(&serviceThread, NULL, ServiceThreadEntry, (void*) &shutdown_flag);

    struct SolidSyslogMessage message = {
        .Facility = options.Facility,
        .Severity = options.Severity,
        .MessageId = options.MessageId,
        .Msg = options.Msg,
    };

    BddTargetInteractive_Run(solidSyslog, &message, stdin, BddTargetSwitchConfig_SetByName, OnSet);

    shutdown_flag = true;
    pthread_join(serviceThread, NULL);

    SolidSyslog_Destroy(solidSyslog);
    SolidSyslogOriginSd_Destroy(originSd);
    SolidSyslogTimeQualitySd_Destroy(timeQuality);
    SolidSyslogMetaSd_Destroy(metaSd);
    SolidSyslogStdAtomicCounter_Destroy(counter);
    DestroyStore(store, &options);
    SolidSyslogPosixMessageQueueBuffer_Destroy(buffer);
    SolidSyslogPosixMutex_Destroy(bufferMutex);
    DestroySender();

    return 0;
}
