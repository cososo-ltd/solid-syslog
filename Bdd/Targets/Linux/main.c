#include <pthread.h>
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
#include "BddTargetLanguage.h"
#include "BddTargetMtlsConfig.h"
#include "BddTargetServiceThread.h"
#include "BddTargetStderrErrorHandler.h"
#include "BddTargetSwitchConfig.h"
#include "BddTargetTcpConfig.h"
#include "BddTargetTlsConfig.h"
#include "BddTargetTlsSender.h"
#include "BddTargetUdpConfig.h"
#include "SolidSyslog.h"
#include "SolidSyslogAtomicCounter.h"
#include "SolidSyslogTunables.h"
#include "SolidSyslogConfig.h"
#include "SolidSyslogCrc16Policy.h"
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
#include "SolidSyslogPosixProcessId.h"
#include "SolidSyslogPosixSleep.h"
#include "SolidSyslogPosixSysUpTime.h"
#include "SolidSyslogGetAddrInfoResolver.h"
#include "SolidSyslogPosixDatagram.h"
#include "SolidSyslogPosixTcpStream.h"
#include "SolidSyslogSwitchingSender.h"
#include "SolidSyslogStreamSender.h"
#include "SolidSyslogUdpSender.h"
#include "SolidSyslogTimeQuality.h"

struct SolidSyslogStore;

static const char* const                STORE_PATH_PREFIX     = "/tmp/STORE";
static const char* const                THRESHOLD_MARKER_PATH = "/tmp/solidsyslog_threshold_marker.log";
static struct SolidSyslogFile*          storeFile;
static struct SolidSyslogBlockDevice*   storeBlockDevice;
static SolidSyslogPosixTcpStreamStorage plainTcpStreamStorage;
static struct SolidSyslogStream*        plainTcpStream;
static SolidSyslogStreamSenderStorage   plainTcpSenderStorage;
static struct SolidSyslogSender*        plainTcpSender;

static void GetTimeQuality(struct SolidSyslogTimeQuality* timeQuality)
{
    timeQuality->tzKnown                  = true;
    timeQuality->isSynced                 = true;
    timeQuality->syncAccuracyMicroseconds = SOLIDSYSLOG_SYNC_ACCURACY_OMIT;
}

static volatile bool shutdown_flag;

static void* ServiceThreadEntry(void* arg)
{
    volatile bool* shutdown = (volatile bool*) arg;
    BddTargetServiceThread_Run(shutdown, SolidSyslogPosixSleep);
    return NULL;
}

static struct SolidSyslogSender* CreateSender(const struct BddTargetOptions* options)
{
    bool mtlsModeActive = (strcmp(options->transport, "mtls") == 0);

    struct SolidSyslogResolver* resolver = SolidSyslogGetAddrInfoResolver_Create();

    static struct SolidSyslogUdpSenderConfig udpConfig = {0};
    udpConfig.resolver                                 = resolver;
    udpConfig.datagram                                 = SolidSyslogPosixDatagram_Create();
    udpConfig.endpoint                                 = BddTargetUdpConfig_GetEndpoint;
    udpConfig.endpointVersion                          = BddTargetUdpConfig_GetEndpointVersion;
    struct SolidSyslogSender* udpSender                = SolidSyslogUdpSender_Create(&udpConfig);

    plainTcpStream                                        = SolidSyslogPosixTcpStream_Create(&plainTcpStreamStorage);
    static struct SolidSyslogStreamSenderConfig tcpConfig = {0};
    tcpConfig.resolver                                    = resolver;
    tcpConfig.stream                                      = plainTcpStream;
    tcpConfig.endpoint                                    = BddTargetTcpConfig_GetEndpoint;
    tcpConfig.endpointVersion                             = BddTargetTcpConfig_GetEndpointVersion;
    plainTcpSender                                        = SolidSyslogStreamSender_Create(&plainTcpSenderStorage, &tcpConfig);

    struct SolidSyslogSender* tlsSender = BddTargetTlsSender_Create(resolver, mtlsModeActive);

    static struct SolidSyslogSender* inners[BDD_TARGET_SWITCH_COUNT];
    inners[BDD_TARGET_SWITCH_UDP] = udpSender;
    inners[BDD_TARGET_SWITCH_TCP] = plainTcpSender;
    inners[BDD_TARGET_SWITCH_TLS] = tlsSender;

    static struct SolidSyslogSwitchingSenderConfig switchConfig = {0};
    switchConfig.senders                                        = inners;
    switchConfig.senderCount                                    = BDD_TARGET_SWITCH_COUNT;
    switchConfig.selector                                       = BddTargetSwitchConfig_Selector;

    BddTargetSwitchConfig_SetByName(options->transport);
    return SolidSyslogSwitchingSender_Create(&switchConfig);
}

static enum SolidSyslogDiscardPolicy MapDiscardPolicy(const char* policy)
{
    if (strcmp(policy, "newest") == 0)
    {
        return SOLIDSYSLOG_DISCARD_NEWEST;
    }
    if (strcmp(policy, "halt") == 0)
    {
        return SOLIDSYSLOG_HALT;
    }
    return SOLIDSYSLOG_DISCARD_OLDEST;
}

static volatile bool haltExit;

static void OnStoreFull(void* context)
{
    (void) context;
    if (haltExit)
    {
        _exit(2);
    }
}

static size_t GetCapacityThreshold(void* context)
{
    return *(const size_t*) context;
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

static struct SolidSyslogStore* CreateStore(const struct BddTargetOptions* options)
{
    bool useFile = (strcmp(options->store, "file") == 0);

    if (useFile)
    {
        static SolidSyslogPosixFileStorage fileStorage;
        storeFile = SolidSyslogPosixFile_Create(&fileStorage);

        static SolidSyslogFileBlockDeviceStorage blockDeviceStorage;
        storeBlockDevice = SolidSyslogFileBlockDevice_Create(&blockDeviceStorage, storeFile, STORE_PATH_PREFIX);

        static size_t capacityThreshold;
        capacityThreshold                                     = options->capacityThreshold;
        static struct SolidSyslogBlockStoreConfig storeConfig = {0};
        storeConfig.blockDevice                               = storeBlockDevice;
        storeConfig.maxBlockSize                              = options->maxBlockSize;
        storeConfig.maxBlocks                                 = options->maxBlocks;
        storeConfig.discardPolicy                             = MapDiscardPolicy(options->discardPolicy);
        storeConfig.securityPolicy                            = SolidSyslogCrc16Policy_Create();
        storeConfig.onStoreFull                               = OnStoreFull;
        storeConfig.getCapacityThreshold                      = GetCapacityThreshold;
        storeConfig.onThresholdCrossed                        = OnThresholdCrossed;
        storeConfig.thresholdContext                          = &capacityThreshold;

        static SolidSyslogBlockStoreStorage storeStorage;
        return SolidSyslogBlockStore_Create(&storeStorage, &storeConfig);
    }

    return SolidSyslogNullStore_Create();
}

static void DestroySender(void)
{
    SolidSyslogSwitchingSender_Destroy();
    BddTargetTlsSender_Destroy();
    SolidSyslogStreamSender_Destroy(plainTcpSender);
    SolidSyslogPosixTcpStream_Destroy(plainTcpStream);
    SolidSyslogUdpSender_Destroy();
    SolidSyslogPosixDatagram_Destroy();
    SolidSyslogGetAddrInfoResolver_Destroy();
}

static void DestroyStore(struct SolidSyslogStore* store, const struct BddTargetOptions* options)
{
    bool useFile = (strcmp(options->store, "file") == 0);

    if (useFile)
    {
        SolidSyslogBlockStore_Destroy(store);
        SolidSyslogFileBlockDevice_Destroy(storeBlockDevice);
        SolidSyslogCrc16Policy_Destroy();
        SolidSyslogPosixFile_Destroy(storeFile);
    }
    else
    {
        SolidSyslogNullStore_Destroy();
    }
}

int main(int argc, char* argv[])
{
    BddTargetStderrErrorHandler_Install();

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

    /* Honour --app-name when supplied (BDD scenarios pin it for record-size
       parity across runners); otherwise derive from argv[0] as before. */
    BddTargetAppName_Set((options.appName != NULL) ? options.appName : argv[0]);

    struct SolidSyslogSender* sender = CreateSender(&options);
    struct SolidSyslogStore*  store  = CreateStore(&options);

    struct SolidSyslogBuffer*        buffer     = SolidSyslogPosixMessageQueueBuffer_Create(SOLIDSYSLOG_MAX_MESSAGE_SIZE, 10);
    struct SolidSyslogAtomicCounter* counter    = SolidSyslogAtomicCounter_Create();
    struct SolidSyslogMetaSdConfig   metaConfig = {
          .counter      = counter,
          .getSysUpTime = SolidSyslogPosixSysUpTime_Get,
          .getLanguage  = BddTargetLanguage_Get,
    };
    struct SolidSyslogStructuredData* metaSd = SolidSyslogMetaSd_Create(&metaConfig);

    struct SolidSyslogStructuredData* timeQuality  = SolidSyslogTimeQualitySd_Create(GetTimeQuality);
    struct SolidSyslogOriginSdConfig  originConfig = {
         .software     = "SolidSyslogBddTarget",
         .swVersion    = "0.7.0",
         .enterpriseId = BDD_TARGET_ENTERPRISE_ID,
         .getIpCount   = BddTargetIps_Count,
         .getIpAt      = BddTargetIps_At,
    };
    struct SolidSyslogStructuredData* originSd = SolidSyslogOriginSd_Create(&originConfig);

    struct SolidSyslogStructuredData* sdList[3] = {metaSd, timeQuality, originSd};
    size_t                            sdCount   = options.noSd ? 1 : 3;

    struct SolidSyslogConfig config = {
        .buffer       = buffer,
        .sender       = sender,
        .clock        = SolidSyslogPosixClock_GetTimestamp,
        .getHostname  = SolidSyslogPosixHostname_Get,
        .getAppName   = BddTargetAppName_Get,
        .getProcessId = SolidSyslogPosixProcessId_Get,
        .store        = store,
        .sd           = sdList,
        .sdCount      = sdCount,
    };
    SolidSyslog_Create(&config);

    shutdown_flag = false;
    haltExit      = options.haltExit;

    pthread_t serviceThread = 0;
    pthread_create(&serviceThread, NULL, ServiceThreadEntry, (void*) &shutdown_flag);

    struct SolidSyslogMessage message = {
        .facility  = options.facility,
        .severity  = options.severity,
        .messageId = options.messageId,
        .msg       = options.msg,
    };

    BddTargetInteractive_Run(&message, stdin, BddTargetSwitchConfig_SetByName, NULL);

    shutdown_flag = true;
    pthread_join(serviceThread, NULL);

    SolidSyslog_Destroy();
    SolidSyslogOriginSd_Destroy();
    SolidSyslogTimeQualitySd_Destroy();
    SolidSyslogMetaSd_Destroy();
    SolidSyslogAtomicCounter_Destroy();
    DestroyStore(store, &options);
    SolidSyslogPosixMessageQueueBuffer_Destroy();
    DestroySender();

    return 0;
}
