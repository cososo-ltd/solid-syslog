#include "BddTargetWindows.h"
#include "BddTargetAppName.h"
#include "BddTargetEnterpriseId.h"
#include "BddTargetInteractive.h"
#include "BddTargetIps.h"
#include "BddTargetLanguage.h"
#include "BddTargetMtlsConfig.h"
#include "BddTargetServiceThread.h"
#include "BddTargetStderrErrorHandler.h"
#include "BddTargetSwitchConfig.h"
#include "BddTargetTlsConfig.h"
#include "BddTargetTlsSender.h"
#include "BddTargetWindowsCommandLine.h"
#include "SolidSyslog.h"
#include "SolidSyslogWindowsAtomicCounter.h"
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
    BDD_TARGET_PORT = 5514,
    BDD_TARGET_BUFFER_MESSAGES = 10
};

/* Store-and-forward backing files. Relative to the working directory; the BDD
   harness runs from the project root and Bdd/output is already a writable
   shared dir on both runners. The matching POSIX paths in the Linux Threaded
   example are /tmp/STORE and /tmp/solidsyslog_threshold_marker.log. */
static const char* const STORE_PATH_PREFIX = "Bdd/output/STORE";
static const char* const THRESHOLD_MARKER_PATH = "Bdd/output/solidsyslog_threshold_marker.log";

static SolidSyslogWinsockTcpStreamStorage tcpStreamStorage;
static uint8_t bufferRing[SOLIDSYSLOG_CIRCULAR_BUFFER_RING_BYTES(BDD_TARGET_BUFFER_MESSAGES)];
static SolidSyslogWindowsMutexStorage mutexStorage;
static SolidSyslogWindowsAtomicCounterStorage counterStorage;
static volatile bool shutdownFlag;

/* Created in CreateSender, destroyed in DestroySender — held in file scope so
   teardown can reach them after the SwitchingSender wraps them all. */
static struct SolidSyslogStream* plainTcpStream;
static struct SolidSyslogSender* plainTcpSender;
static struct SolidSyslogDatagram* udpDatagram;
static struct SolidSyslogSender* udpSender;
static struct SolidSyslogSender* switchingSender;

/* Block-store backing — created in CreateStore, released in DestroyStore. */
static struct SolidSyslogFile* storeFile;
static struct SolidSyslogBlockDevice* storeBlockDevice;

// NOLINTNEXTLINE(readability-non-const-parameter) -- _beginthreadex thread-entry signature requires void*
static unsigned __stdcall ServiceThreadEntry(void* arg)
{
    BddTargetServiceThread_Run((volatile bool*) arg, SolidSyslogWindowsSleep);
    return 0;
}

static const char* GetHost(void)
{
    return "127.0.0.1";
}

static int GetPort(void)
{
    return BDD_TARGET_PORT;
}

static void GetEndpoint(struct SolidSyslogEndpoint* endpoint)
{
    SolidSyslogFormatter_BoundedString(endpoint->Host, GetHost(), SOLIDSYSLOG_MAX_HOST_SIZE);
    endpoint->Port = (uint16_t) GetPort();
}

static uint32_t GetEndpointVersion(void)
{
    return 0;
}

static void GetTimeQuality(struct SolidSyslogTimeQuality* timeQuality)
{
    timeQuality->TzKnown = true;
    timeQuality->IsSynced = true;
    timeQuality->SyncAccuracyMicroseconds = SOLIDSYSLOG_SYNC_ACCURACY_OMIT;
}

/* MSVC's getenv triggers C4996; getenv_s is the strict-mode equivalent.
   Static buffer is single-thread-safe (called once from _Run before threads
   start) and large enough for any reasonable hostname. Returns the buffer
   when the env var is set and non-empty, NULL otherwise. */
static const char* GetEnvVar(char* buffer, size_t bufferSize, const char* name)
{
    size_t requiredSize = 0;
    errno_t err = getenv_s(&requiredSize, buffer, bufferSize, name);
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
        return SOLIDSYSLOG_DISCARD_POLICY_NEWEST;
    }
    if (strcmp(policy, "halt") == 0)
    {
        return SOLIDSYSLOG_DISCARD_POLICY_HALT;
    }
    return SOLIDSYSLOG_DISCARD_POLICY_OLDEST;
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
    FILE* fp = NULL;
    errno_t err = fopen_s(&fp, THRESHOLD_MARKER_PATH, "a");
    if ((err == 0) && (fp != NULL))
    {
        fputs("crossed\n", fp);
        (void) fclose(fp);
    }
}

static struct SolidSyslogSender* CreateSender(const struct BddTargetWindowsOptions* options)
{
    bool mtlsModeActive = (strcmp(options->Transport, "mtls") == 0);

    struct SolidSyslogResolver* resolver = SolidSyslogWinsockResolver_Create();

    udpDatagram = SolidSyslogWinsockDatagram_Create();
    static struct SolidSyslogUdpSenderConfig udpConfig = {0};
    udpConfig.Resolver = resolver;
    udpConfig.Datagram = udpDatagram;
    udpConfig.Endpoint = GetEndpoint;
    udpConfig.EndpointVersion = GetEndpointVersion;
    udpSender = SolidSyslogUdpSender_Create(&udpConfig);

    plainTcpStream = SolidSyslogWinsockTcpStream_Create(&tcpStreamStorage);
    static struct SolidSyslogStreamSenderConfig tcpConfig = {0};
    tcpConfig.Resolver = resolver;
    tcpConfig.Stream = plainTcpStream;
    tcpConfig.Endpoint = GetEndpoint;
    tcpConfig.EndpointVersion = GetEndpointVersion;
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

static void DestroySender(void)
{
    SolidSyslogSwitchingSender_Destroy(switchingSender);
    BddTargetTlsSender_Destroy();
    SolidSyslogStreamSender_Destroy(plainTcpSender);
    SolidSyslogWinsockTcpStream_Destroy(plainTcpStream);
    SolidSyslogUdpSender_Destroy(udpSender);
    SolidSyslogWinsockDatagram_Destroy();
    SolidSyslogWinsockResolver_Destroy();
}

static struct SolidSyslogStore* CreateStore(const struct BddTargetWindowsOptions* options)
{
    bool useFile = (strcmp(options->Store, "file") == 0);

    if (useFile)
    {
        static SolidSyslogWindowsFileStorage fileStorage;
        storeFile = SolidSyslogWindowsFile_Create(&fileStorage);

        storeBlockDevice = SolidSyslogFileBlockDevice_Create(storeFile, STORE_PATH_PREFIX);

        static size_t capacityThreshold;
        capacityThreshold = options->CapacityThreshold;
        static struct SolidSyslogBlockStoreConfig storeConfig = {0};
        storeConfig.BlockDevice = storeBlockDevice;
        storeConfig.MaxBlockSize = options->MaxBlockSize;
        storeConfig.MaxBlocks = options->MaxBlocks;
        storeConfig.DiscardPolicy = MapDiscardPolicy(options->DiscardPolicy);
        storeConfig.SecurityPolicy = SolidSyslogCrc16Policy_Create();
        storeConfig.OnStoreFull = OnStoreFull;
        storeConfig.GetCapacityThreshold = GetCapacityThreshold;
        storeConfig.OnThresholdCrossed = OnThresholdCrossed;
        storeConfig.ThresholdContext = &capacityThreshold;

        return SolidSyslogBlockStore_Create(&storeConfig);
    }

    return SolidSyslogNullStore_Get();
}

static void DestroyStore(struct SolidSyslogStore* store, const struct BddTargetWindowsOptions* options)
{
    bool useFile = (strcmp(options->Store, "file") == 0);

    if (useFile)
    {
        SolidSyslogBlockStore_Destroy(store);
        SolidSyslogFileBlockDevice_Destroy(storeBlockDevice);
        SolidSyslogCrc16Policy_Destroy();
        SolidSyslogWindowsFile_Destroy(storeFile);
    }
    /* else: NullStore is shared and immutable — nothing to destroy. */
}

int BddTargetWindows_Run(int argc, char* argv[])
{
    BddTargetStderrErrorHandler_Install();

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
    BddTargetTlsConfig_SetHost(GetEnvVar(tlsHostBuffer, sizeof tlsHostBuffer, "SOLIDSYSLOG_BDD_TLS_HOST"));
    BddTargetMtlsConfig_SetHost(GetEnvVar(mtlsHostBuffer, sizeof mtlsHostBuffer, "SOLIDSYSLOG_BDD_MTLS_HOST"));

    struct BddTargetWindowsOptions options;
    BddTargetWindowsCommandLine_Parse(argc, argv, &options);

    /* Honour --app-name when supplied (BDD scenarios pin it for record-size
       parity across runners); otherwise derive from argv[0] as before. */
    BddTargetAppName_Set((options.AppName != NULL) ? options.AppName : argv[0]);

    haltExit = options.HaltExit;

    struct SolidSyslogSender* sender = CreateSender(&options);
    struct SolidSyslogStore* store = CreateStore(&options);

    struct SolidSyslogMutex* mutex = SolidSyslogWindowsMutex_Create(&mutexStorage);
    struct SolidSyslogBuffer* buffer = SolidSyslogCircularBuffer_Create(mutex, bufferRing, sizeof(bufferRing));
    struct SolidSyslogAtomicCounter* counter = SolidSyslogWindowsAtomicCounter_Create(&counterStorage);
    struct SolidSyslogMetaSdConfig metaConfig = {
        .Counter = counter,
        .GetSysUpTime = SolidSyslogWindowsSysUpTime_Get,
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

    /* MetaSd is always present so seqId-based BDD scenarios work; the
       --no-sd flag (matching the Linux Threaded example) suppresses the
       optional timeQuality and origin SDs to keep records under the
       BlockStore's per-block packing budget for store-capacity tests. */
    struct SolidSyslogStructuredData* sdList[3] = {metaSd, timeQuality, originSd};
    size_t sdCount = options.NoSd ? 1 : 3;

    struct SolidSyslogConfig config = {
        .Buffer = buffer,
        .Sender = sender,
        .Clock = SolidSyslogWindowsClock_GetTimestamp,
        .GetHostname = SolidSyslogWindowsHostname_Get,
        .GetAppName = BddTargetAppName_Get,
        .GetProcessId = SolidSyslogWindowsProcessId_Get,
        .Store = store,
        .Sd = sdList,
        .SdCount = sdCount,
    };
    SolidSyslog_Create(&config);

    shutdownFlag = false;
    HANDLE serviceThread = (HANDLE) _beginthreadex(NULL, 0, ServiceThreadEntry, (void*) &shutdownFlag, 0, NULL);

    struct SolidSyslogMessage message = {
        .Facility = options.Facility,
        .Severity = options.Severity,
        .MessageId = options.MessageId,
        .Msg = options.Msg,
    };

    BddTargetInteractive_Run(&message, stdin, BddTargetSwitchConfig_SetByName, NULL);

    shutdownFlag = true;
    WaitForSingleObject(serviceThread, INFINITE);
    CloseHandle(serviceThread);

    SolidSyslog_Destroy();
    SolidSyslogOriginSd_Destroy(originSd);
    SolidSyslogTimeQualitySd_Destroy(timeQuality);
    SolidSyslogMetaSd_Destroy(metaSd);
    SolidSyslogWindowsAtomicCounter_Destroy(counter);
    DestroyStore(store, &options);
    SolidSyslogCircularBuffer_Destroy(buffer);
    SolidSyslogWindowsMutex_Destroy(mutex);
    DestroySender();

    WSACleanup();

    return 0;
}
