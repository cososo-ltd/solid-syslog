#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>

#include "ExampleAppName.h"
#include "ExampleCommandLine.h"
#include "ExampleEnterpriseId.h"
#include "ExampleInteractive.h"
#include "ExampleIps.h"
#include "ExampleLanguage.h"
#include "ExampleMtlsConfig.h"
#include "ExampleServiceThread.h"
#include "ExampleStderrErrorHandler.h"
#include "ExampleSwitchConfig.h"
#include "ExampleTcpConfig.h"
#include "ExampleTlsConfig.h"
#include "ExampleTlsSender.h"
#include "ExampleUdpConfig.h"
#include "SolidSyslog.h"
#include "SolidSyslogAtomicCounter.h"
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
static struct SolidSyslogFile*          storeReadFile;
static struct SolidSyslogFile*          storeWriteFile;
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
    ExampleServiceThread_Run(shutdown, SolidSyslogPosixSleep);
    return NULL;
}

static struct SolidSyslogSender* CreateSender(const struct ExampleOptions* options)
{
    bool mtlsModeActive = (strcmp(options->transport, "mtls") == 0);

    struct SolidSyslogResolver* resolver = SolidSyslogGetAddrInfoResolver_Create();

    static struct SolidSyslogUdpSenderConfig udpConfig = {0};
    udpConfig.resolver                                 = resolver;
    udpConfig.datagram                                 = SolidSyslogPosixDatagram_Create();
    udpConfig.endpoint                                 = ExampleUdpConfig_GetEndpoint;
    udpConfig.endpointVersion                          = ExampleUdpConfig_GetEndpointVersion;
    struct SolidSyslogSender* udpSender                = SolidSyslogUdpSender_Create(&udpConfig);

    plainTcpStream                                        = SolidSyslogPosixTcpStream_Create(&plainTcpStreamStorage);
    static struct SolidSyslogStreamSenderConfig tcpConfig = {0};
    tcpConfig.resolver                                    = resolver;
    tcpConfig.stream                                      = plainTcpStream;
    tcpConfig.endpoint                                    = ExampleTcpConfig_GetEndpoint;
    tcpConfig.endpointVersion                             = ExampleTcpConfig_GetEndpointVersion;
    plainTcpSender                                        = SolidSyslogStreamSender_Create(&plainTcpSenderStorage, &tcpConfig);

    struct SolidSyslogSender* tlsSender = ExampleTlsSender_Create(resolver, mtlsModeActive);

    static struct SolidSyslogSender* inners[EXAMPLE_SWITCH_COUNT];
    inners[EXAMPLE_SWITCH_UDP] = udpSender;
    inners[EXAMPLE_SWITCH_TCP] = plainTcpSender;
    inners[EXAMPLE_SWITCH_TLS] = tlsSender;

    static struct SolidSyslogSwitchingSenderConfig switchConfig = {0};
    switchConfig.senders                                        = inners;
    switchConfig.senderCount                                    = EXAMPLE_SWITCH_COUNT;
    switchConfig.selector                                       = ExampleSwitchConfig_Selector;

    ExampleSwitchConfig_SetByName(options->transport);
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

static struct SolidSyslogStore* CreateStore(const struct ExampleOptions* options)
{
    bool useFile = (strcmp(options->store, "file") == 0);

    if (useFile)
    {
        static SolidSyslogPosixFileStorage readStorage;
        static SolidSyslogPosixFileStorage writeStorage;
        storeReadFile  = SolidSyslogPosixFile_Create(&readStorage);
        storeWriteFile = SolidSyslogPosixFile_Create(&writeStorage);

        static SolidSyslogFileBlockDeviceStorage blockDeviceStorage;
        storeBlockDevice = SolidSyslogFileBlockDevice_Create(&blockDeviceStorage, storeReadFile, storeWriteFile, STORE_PATH_PREFIX);

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
    ExampleTlsSender_Destroy();
    SolidSyslogStreamSender_Destroy(plainTcpSender);
    SolidSyslogPosixTcpStream_Destroy(plainTcpStream);
    SolidSyslogUdpSender_Destroy();
    SolidSyslogPosixDatagram_Destroy();
    SolidSyslogGetAddrInfoResolver_Destroy();
}

static void DestroyStore(struct SolidSyslogStore* store, const struct ExampleOptions* options)
{
    bool useFile = (strcmp(options->store, "file") == 0);

    if (useFile)
    {
        SolidSyslogBlockStore_Destroy(store);
        SolidSyslogFileBlockDevice_Destroy(storeBlockDevice);
        SolidSyslogCrc16Policy_Destroy();
        SolidSyslogPosixFile_Destroy(storeWriteFile);
        SolidSyslogPosixFile_Destroy(storeReadFile);
    }
    else
    {
        SolidSyslogNullStore_Destroy();
    }
}

int main(int argc, char* argv[])
{
    ExampleStderrErrorHandler_Install();

    /* BDD harness can override the TLS/mTLS host (defaults to "syslog-ng",
       the Linux compose service name). Same env-var contract as the Windows
       example so behave can target either oracle. */
    ExampleTlsConfig_SetHost(getenv("SOLIDSYSLOG_BDD_TLS_HOST"));
    ExampleMtlsConfig_SetHost(getenv("SOLIDSYSLOG_BDD_MTLS_HOST"));

    struct ExampleOptions options;
    if (ExampleCommandLine_Parse(argc, argv, &options) != 0)
    {
        return 1;
    }

    /* Honour --app-name when supplied (BDD scenarios pin it for record-size
       parity across runners); otherwise derive from argv[0] as before. */
    ExampleAppName_Set((options.appName != NULL) ? options.appName : argv[0]);

    struct SolidSyslogSender* sender = CreateSender(&options);
    struct SolidSyslogStore*  store  = CreateStore(&options);

    struct SolidSyslogBuffer*        buffer     = SolidSyslogPosixMessageQueueBuffer_Create(SOLIDSYSLOG_MAX_MESSAGE_SIZE, 10);
    struct SolidSyslogAtomicCounter* counter    = SolidSyslogAtomicCounter_Create();
    struct SolidSyslogMetaSdConfig   metaConfig = {
          .counter      = counter,
          .getSysUpTime = SolidSyslogPosixSysUpTime_Get,
          .getLanguage  = ExampleLanguage_Get,
    };
    struct SolidSyslogStructuredData* metaSd = SolidSyslogMetaSd_Create(&metaConfig);

    struct SolidSyslogStructuredData* timeQuality  = SolidSyslogTimeQualitySd_Create(GetTimeQuality);
    struct SolidSyslogOriginSdConfig  originConfig = {
         .software     = "SolidSyslogExample",
         .swVersion    = "0.7.0",
         .enterpriseId = EXAMPLE_ENTERPRISE_ID,
         .getIpCount   = ExampleIps_Count,
         .getIpAt      = ExampleIps_At,
    };
    struct SolidSyslogStructuredData* originSd = SolidSyslogOriginSd_Create(&originConfig);

    struct SolidSyslogStructuredData* sdList[3] = {metaSd, timeQuality, originSd};
    size_t                            sdCount   = options.noSd ? 1 : 3;

    struct SolidSyslogConfig config = {
        .buffer       = buffer,
        .sender       = sender,
        .clock        = SolidSyslogPosixClock_GetTimestamp,
        .getHostname  = SolidSyslogPosixHostname_Get,
        .getAppName   = ExampleAppName_Get,
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

    ExampleInteractive_Run(&message, stdin, ExampleSwitchConfig_SetByName, NULL);

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
