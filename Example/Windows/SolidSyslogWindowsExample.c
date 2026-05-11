#include "SolidSyslogWindowsExample.h"
#include "ExampleAppName.h"
#include "ExampleEnterpriseId.h"
#include "ExampleInteractive.h"
#include "ExampleIps.h"
#include "ExampleLanguage.h"
#include "ExampleMtlsConfig.h"
#include "ExampleServiceThread.h"
#include "ExampleStderrErrorHandler.h"
#include "ExampleSwitchConfig.h"
#include "ExampleTlsConfig.h"
#include "ExampleTlsSender.h"
#include "ExampleWindowsCommandLine.h"
#include "SolidSyslog.h"
#include "SolidSyslogAtomicCounter.h"
#include "SolidSyslogBlockStore.h"
#include "SolidSyslogCircularBuffer.h"
#include "SolidSyslogConfig.h"
#include "SolidSyslogCrc16Policy.h"
#include "SolidSyslogEndpoint.h"
#include "SolidSyslogFileBlockDevice.h"
#include "SolidSyslogFormatter.h"
#include "SolidSyslogMetaSd.h"
#include "SolidSyslogNullStore.h"
#include "SolidSyslogOriginSd.h"
#include "SolidSyslogStreamSender.h"
#include "SolidSyslogSwitchingSender.h"
#include "SolidSyslogTimeQualitySd.h"
#include "SolidSyslogUdpSender.h"
#include "SolidSyslogWindowsClock.h"
#include "SolidSyslogWindowsFile.h"
#include "SolidSyslogWindowsHostname.h"
#include "SolidSyslogWindowsMutex.h"
#include "SolidSyslogWindowsProcessId.h"
#include "SolidSyslogWindowsSleep.h"
#include "SolidSyslogWindowsSysUpTime.h"
#include "SolidSyslogWinsockDatagram.h"
#include "SolidSyslogWinsockResolver.h"
#include "SolidSyslogWinsockTcpStream.h"

#include <process.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
// windows.h must follow winsock2.h to avoid winsock1/2 declaration conflicts
#include <windows.h>

enum
{
    /* Unprivileged mirror of SOLIDSYSLOG_UDP_DEFAULT_PORT (514) /
       SOLIDSYSLOG_TCP_DEFAULT_PORT (601). The BDD oracle listens on both
       UDP and TCP at 5514. */
    EXAMPLE_PORT            = 5514,
    EXAMPLE_BUFFER_MESSAGES = 10
};

/* Store-and-forward backing files. Relative to the working directory; the BDD
   harness runs from the project root and Bdd/output is already a writable
   shared dir on both runners. The matching POSIX paths in the Linux Threaded
   example are /tmp/STORE and /tmp/solidsyslog_threshold_marker.log. */
static const char* const STORE_PATH_PREFIX     = "Bdd/output/STORE";
static const char* const THRESHOLD_MARKER_PATH = "Bdd/output/solidsyslog_threshold_marker.log";

static SolidSyslogWinsockTcpStreamStorage tcpStreamStorage;
static SolidSyslogStreamSenderStorage     tcpSenderStorage;
static SolidSyslogCircularBufferStorage   bufferStorage[SOLIDSYSLOG_CIRCULARBUFFER_STORAGE_SIZE(EXAMPLE_BUFFER_MESSAGES)];
static SolidSyslogWindowsMutexStorage     mutexStorage;
static volatile bool                      shutdownFlag;

/* Created in CreateSender, destroyed in DestroySender — held in file scope so
   teardown can reach them after the SwitchingSender wraps them all. */
static struct SolidSyslogStream*   plainTcpStream;
static struct SolidSyslogSender*   plainTcpSender;
static struct SolidSyslogDatagram* udpDatagram;

/* Block-store backing — created in CreateStore, released in DestroyStore. */
static struct SolidSyslogFile*        storeReadFile;
static struct SolidSyslogFile*        storeWriteFile;
static struct SolidSyslogBlockDevice* storeBlockDevice;

// NOLINTNEXTLINE(readability-non-const-parameter) -- _beginthreadex thread-entry signature requires void*
static unsigned __stdcall ServiceThreadEntry(void* arg)
{
    ExampleServiceThread_Run((volatile bool*) arg, SolidSyslogWindowsSleep);
    return 0;
}

static const char* GetHost(void)
{
    return "127.0.0.1";
}

static int GetPort(void)
{
    return EXAMPLE_PORT;
}

static void GetEndpoint(struct SolidSyslogEndpoint* endpoint)
{
    SolidSyslogFormatter_BoundedString(endpoint->host, GetHost(), SOLIDSYSLOG_MAX_HOST_SIZE);
    endpoint->port = (uint16_t) GetPort();
}

static uint32_t GetEndpointVersion(void)
{
    return 0;
}

static void GetTimeQuality(struct SolidSyslogTimeQuality* timeQuality)
{
    timeQuality->tzKnown                  = true;
    timeQuality->isSynced                 = true;
    timeQuality->syncAccuracyMicroseconds = SOLIDSYSLOG_SYNC_ACCURACY_OMIT;
}

/* MSVC's getenv triggers C4996; getenv_s is the strict-mode equivalent.
   Static buffer is single-thread-safe (called once from _Run before threads
   start) and large enough for any reasonable hostname. Returns the buffer
   when the env var is set and non-empty, NULL otherwise. */
static const char* GetEnvVar(char* buffer, size_t bufferSize, const char* name)
{
    size_t  requiredSize = 0;
    errno_t err          = getenv_s(&requiredSize, buffer, bufferSize, name);
    if ((err != 0) || (requiredSize == 0))
    {
        return NULL;
    }
    return buffer;
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
    /* fopen_s avoids MSVC C4996; same append-and-flush behaviour as the
       Linux example so the BDD harness sees the marker file appear. */
    FILE*   fp  = NULL;
    errno_t err = fopen_s(&fp, THRESHOLD_MARKER_PATH, "a");
    if ((err == 0) && (fp != NULL))
    {
        fputs("crossed\n", fp);
        (void) fclose(fp);
    }
}

static struct SolidSyslogSender* CreateSender(const struct WindowsExampleOptions* options)
{
    bool mtlsModeActive = (strcmp(options->transport, "mtls") == 0);

    struct SolidSyslogResolver* resolver = SolidSyslogWinsockResolver_Create();

    udpDatagram                                        = SolidSyslogWinsockDatagram_Create();
    static struct SolidSyslogUdpSenderConfig udpConfig = {0};
    udpConfig.resolver                                 = resolver;
    udpConfig.datagram                                 = udpDatagram;
    udpConfig.endpoint                                 = GetEndpoint;
    udpConfig.endpointVersion                          = GetEndpointVersion;
    struct SolidSyslogSender* udpSender                = SolidSyslogUdpSender_Create(&udpConfig);

    plainTcpStream                                        = SolidSyslogWinsockTcpStream_Create(&tcpStreamStorage);
    static struct SolidSyslogStreamSenderConfig tcpConfig = {0};
    tcpConfig.resolver                                    = resolver;
    tcpConfig.stream                                      = plainTcpStream;
    tcpConfig.endpoint                                    = GetEndpoint;
    tcpConfig.endpointVersion                             = GetEndpointVersion;
    plainTcpSender                                        = SolidSyslogStreamSender_Create(&tcpSenderStorage, &tcpConfig);

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

static void DestroySender(void)
{
    SolidSyslogSwitchingSender_Destroy();
    ExampleTlsSender_Destroy();
    SolidSyslogStreamSender_Destroy(plainTcpSender);
    SolidSyslogWinsockTcpStream_Destroy(plainTcpStream);
    SolidSyslogUdpSender_Destroy();
    SolidSyslogWinsockDatagram_Destroy();
    SolidSyslogWinsockResolver_Destroy();
}

static struct SolidSyslogStore* CreateStore(const struct WindowsExampleOptions* options)
{
    bool useFile = (strcmp(options->store, "file") == 0);

    if (useFile)
    {
        static SolidSyslogWindowsFileStorage readStorage;
        static SolidSyslogWindowsFileStorage writeStorage;
        storeReadFile  = SolidSyslogWindowsFile_Create(&readStorage);
        storeWriteFile = SolidSyslogWindowsFile_Create(&writeStorage);

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

static void DestroyStore(struct SolidSyslogStore* store, const struct WindowsExampleOptions* options)
{
    bool useFile = (strcmp(options->store, "file") == 0);

    if (useFile)
    {
        SolidSyslogBlockStore_Destroy(store);
        SolidSyslogFileBlockDevice_Destroy(storeBlockDevice);
        SolidSyslogCrc16Policy_Destroy();
        SolidSyslogWindowsFile_Destroy(storeWriteFile);
        SolidSyslogWindowsFile_Destroy(storeReadFile);
    }
    else
    {
        SolidSyslogNullStore_Destroy();
    }
}

int SolidSyslogWindowsExample_Run(int argc, char* argv[])
{
    ExampleStderrErrorHandler_Install();

    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        return 1;
    }

    /* BDD harness can override the TLS/mTLS host (defaults to "syslog-ng",
       the Linux compose service name). Same env-var contract as the Threaded
       example so behave can target either oracle. */
    static char tlsHostBuffer[256];
    static char mtlsHostBuffer[256];
    ExampleTlsConfig_SetHost(GetEnvVar(tlsHostBuffer, sizeof tlsHostBuffer, "SOLIDSYSLOG_BDD_TLS_HOST"));
    ExampleMtlsConfig_SetHost(GetEnvVar(mtlsHostBuffer, sizeof mtlsHostBuffer, "SOLIDSYSLOG_BDD_MTLS_HOST"));

    struct WindowsExampleOptions options;
    ExampleWindowsCommandLine_Parse(argc, argv, &options);

    /* Honour --app-name when supplied (BDD scenarios pin it for record-size
       parity across runners); otherwise derive from argv[0] as before. */
    ExampleAppName_Set((options.appName != NULL) ? options.appName : argv[0]);

    haltExit = options.haltExit;

    struct SolidSyslogSender* sender = CreateSender(&options);
    struct SolidSyslogStore*  store  = CreateStore(&options);

    struct SolidSyslogMutex*         mutex      = SolidSyslogWindowsMutex_Create(&mutexStorage);
    struct SolidSyslogBuffer*        buffer     = SolidSyslogCircularBuffer_Create(bufferStorage, sizeof(bufferStorage), mutex);
    struct SolidSyslogAtomicCounter* counter    = SolidSyslogAtomicCounter_Create();
    struct SolidSyslogMetaSdConfig   metaConfig = {
          .counter      = counter,
          .getSysUpTime = SolidSyslogWindowsSysUpTime_Get,
          .getLanguage  = ExampleLanguage_Get,
    };
    struct SolidSyslogStructuredData* metaSd       = SolidSyslogMetaSd_Create(&metaConfig);
    struct SolidSyslogStructuredData* timeQuality  = SolidSyslogTimeQualitySd_Create(GetTimeQuality);
    struct SolidSyslogOriginSdConfig  originConfig = {
         .software     = "SolidSyslogExample",
         .swVersion    = "0.7.0",
         .enterpriseId = EXAMPLE_ENTERPRISE_ID,
         .getIpCount   = ExampleIps_Count,
         .getIpAt      = ExampleIps_At,
    };
    struct SolidSyslogStructuredData* originSd = SolidSyslogOriginSd_Create(&originConfig);

    /* MetaSd is always present so seqId-based BDD scenarios work; the
       --no-sd flag (matching the Linux Threaded example) suppresses the
       optional timeQuality and origin SDs to keep records under the
       BlockStore's per-block packing budget for store-capacity tests. */
    struct SolidSyslogStructuredData* sdList[3] = {metaSd, timeQuality, originSd};
    size_t                            sdCount   = options.noSd ? 1 : 3;

    struct SolidSyslogConfig config = {
        .buffer       = buffer,
        .sender       = sender,
        .clock        = SolidSyslogWindowsClock_GetTimestamp,
        .getHostname  = SolidSyslogWindowsHostname_Get,
        .getAppName   = ExampleAppName_Get,
        .getProcessId = SolidSyslogWindowsProcessId_Get,
        .store        = store,
        .sd           = sdList,
        .sdCount      = sdCount,
    };
    SolidSyslog_Create(&config);

    shutdownFlag         = false;
    HANDLE serviceThread = (HANDLE) _beginthreadex(NULL, 0, ServiceThreadEntry, (void*) &shutdownFlag, 0, NULL);

    struct SolidSyslogMessage message = {
        .facility  = options.facility,
        .severity  = options.severity,
        .messageId = options.messageId,
        .msg       = options.msg,
    };

    ExampleInteractive_Run(&message, stdin, ExampleSwitchConfig_SetByName, NULL);

    shutdownFlag = true;
    WaitForSingleObject(serviceThread, INFINITE);
    CloseHandle(serviceThread);

    SolidSyslog_Destroy();
    SolidSyslogOriginSd_Destroy();
    SolidSyslogTimeQualitySd_Destroy();
    SolidSyslogMetaSd_Destroy();
    SolidSyslogAtomicCounter_Destroy();
    DestroyStore(store, &options);
    SolidSyslogCircularBuffer_Destroy(buffer);
    SolidSyslogWindowsMutex_Destroy(mutex);
    DestroySender();

    WSACleanup();

    return 0;
}
