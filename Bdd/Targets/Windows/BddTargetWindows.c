#include "BddTargetWindows.h"
#include "BddTargetAppName.h"
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
#include "SolidSyslogNullSecurityPolicy.h"
#include "SolidSyslogOpenSslAesGcmPolicy.h"
#include "SolidSyslogOpenSslHmacSha256Policy.h"
#include "SolidSyslogEndpointHost.h"
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
#include "SolidSyslogWinsockAddress.h"
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

static uint8_t bufferRing[SOLIDSYSLOG_CIRCULAR_BUFFER_RING_BYTES(BDD_TARGET_BUFFER_MESSAGES)];
static volatile bool shutdownFlag;
static struct SolidSyslog* solidSyslog;

/* Created in CreateSender, destroyed in DestroySender - held in file scope so
   teardown can reach them after the SwitchingSender wraps them all. */
static struct SolidSyslogResolver* resolver;
static struct SolidSyslogStream* plainTcpStream;
static struct SolidSyslogAddress* plainTcpAddress;
static struct SolidSyslogSender* plainTcpSender;
static struct SolidSyslogDatagram* udpDatagram;
static struct SolidSyslogAddress* udpAddress;
static struct SolidSyslogSender* udpSender;
static struct SolidSyslogSender* switchingSender;

/* Block-store backing - created in CreateStore, released in DestroyStore. */
static struct SolidSyslogFile* storeFile;
static struct SolidSyslogBlockDevice* storeBlockDevice;
/* Holds the created at-rest SecurityPolicy handle so DestroyStore can release
 * it. The keyed policies (hmac-sha256, aes-256-gcm) need the handle to destroy;
 * crc16 is dispatched by name to its singleton Destroy, and null needs no
 * cleanup. */
static struct SolidSyslogSecurityPolicy* securityPolicy;

// NOLINTNEXTLINE(readability-non-const-parameter) -- _beginthreadex thread-entry signature requires void*
static unsigned __stdcall ServiceThreadEntry(void* arg)
{
    BddTargetServiceThread_Run(solidSyslog, (volatile bool*) arg, SolidSyslogWindows_Sleep);
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

static void GetEndpoint(struct SolidSyslogEndpoint* endpoint, void* context)
{
    (void) context;
    SolidSyslogEndpointHost_String(endpoint->Host, GetHost(), SOLIDSYSLOG_MAX_HOST_SIZE);
    endpoint->Port = (uint16_t) GetPort();
}

static uint32_t GetEndpointVersion(void* context)
{
    (void) context;
    return 0;
}

static void GetTimeQuality(struct SolidSyslogTimeQuality* timeQuality)
{
    timeQuality->TzKnown = true;
    timeQuality->IsSynced = true;
    timeQuality->SyncAccuracyMicroseconds = SOLIDSYSLOG_SYNC_ACCURACY_OMIT;
}

/* MSVC's getenv triggers C4996; getenv_s is the strict-mode equivalent.
   Static buffer is written once from BddTargetWindows_Run before any thread
   starts, so no concurrent access occurs; it is large enough for any reasonable
   hostname. Returns the buffer when the env var is set and non-empty, NULL
   otherwise. */
static const char* GetEnvVar(char* buffer, size_t bufferSize, const char* name)
{
    size_t requiredSize = 0U;
    errno_t err = getenv_s(&requiredSize, buffer, bufferSize, name);
    if ((err != 0) || (requiredSize == 0))
    {
        return NULL;
    }
    return buffer;
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

    resolver = SolidSyslogWinsockResolver_Create();

    udpDatagram = SolidSyslogWinsockDatagram_Create();
    udpAddress = SolidSyslogWinsockAddress_Create();
    static struct SolidSyslogUdpSenderConfig udpConfig = {0};
    udpConfig.Resolver = resolver;
    udpConfig.Datagram = udpDatagram;
    udpConfig.Address = udpAddress;
    udpConfig.Endpoint = GetEndpoint;
    udpConfig.EndpointVersion = GetEndpointVersion;
    udpSender = SolidSyslogUdpSender_Create(&udpConfig);

    plainTcpStream = SolidSyslogWinsockTcpStream_Create(NULL);
    plainTcpAddress = SolidSyslogWinsockAddress_Create();
    static struct SolidSyslogStreamSenderConfig tcpConfig = {0};
    tcpConfig.Resolver = resolver;
    tcpConfig.Stream = plainTcpStream;
    tcpConfig.Address = plainTcpAddress;
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
    SolidSyslogWinsockAddress_Destroy(plainTcpAddress);
    SolidSyslogWinsockTcpStream_Destroy(plainTcpStream);
    SolidSyslogUdpSender_Destroy(udpSender);
    SolidSyslogWinsockAddress_Destroy(udpAddress);
    SolidSyslogWinsockDatagram_Destroy(udpDatagram);
    SolidSyslogWinsockResolver_Destroy(resolver);
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

static struct SolidSyslogStore* CreateStore(const struct BddTargetWindowsOptions* options)
{
    bool useFile = (strcmp(options->Store, "file") == 0);

    if (useFile)
    {
        storeFile = SolidSyslogWindowsFile_Create();

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

static void DestroyStore(struct SolidSyslogStore* store, const struct BddTargetWindowsOptions* options)
{
    bool useFile = (strcmp(options->Store, "file") == 0);

    if (useFile)
    {
        SolidSyslogBlockStore_Destroy(store);
        SolidSyslogFileBlockDevice_Destroy(storeBlockDevice);
        DestroySecurityPolicy();
        SolidSyslogWindowsFile_Destroy(storeFile);
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

int BddTargetWindows_Run(int argc, char* argv[])
{
    BddTargetStderrErrorHandler_Install();
    BddTargetStderrErrorHandler_SetTlsSource(BddTargetTlsSender_ErrorSource());

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

    /* Before the store is created, so a scenario expecting an ERROR from its
       Create can say so - a `set` at the prompt arrives too late for that. */
    if (!BddTargetStderrErrorHandler_SetByName("errors-fatal", options.ErrorsFatal))
    {
        (void) fprintf(stderr, "BDD-TARGET: --errors-fatal takes 0 or 1, not %s\n", options.ErrorsFatal);
        WSACleanup();
        return 1;
    }

    /* Honour --app-name when supplied (BDD scenarios pin it for record-size
       parity across runners); otherwise derive from argv[0] as before. */
    BddTargetAppName_Set((options.AppName != NULL) ? options.AppName : argv[0]);

    struct SolidSyslogSender* sender = CreateSender(&options);
    struct SolidSyslogStore* store = CreateStore(&options);

    struct SolidSyslogMutex* mutex = SolidSyslogWindowsMutex_Create();
    struct SolidSyslogBuffer* buffer = SolidSyslogCircularBuffer_Create(
        mutex,
        bufferRing,
        sizeof(bufferRing),
        BddTargetStoreSettings_GetDiscardPolicy,
        NULL
    );
    struct SolidSyslogAtomicCounter* counter = SolidSyslogWindowsAtomicCounter_Create();
    struct SolidSyslogMetaSdConfig metaConfig = {
        .Counter = counter,
        .GetSysUpTime = SolidSyslogWindows_GetSysUpTime,
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
    size_t sdCount = BddTargetStoreSettings_NoSd() ? 1 : 3;

    struct SolidSyslogConfig config = {
        .Buffer = buffer,
        .Sender = sender,
        .Clock = SolidSyslogWindows_GetTimestamp,
        .GetHostname = SolidSyslogWindows_GetHostname,
        .GetAppName = BddTargetAppName_Get,
        .GetProcessId = SolidSyslogWindows_GetProcessId,
        .Store = store,
        .Sd = sdList,
        .SdCount = sdCount,
        .GetDiscardPolicy = BddTargetStoreSettings_GetDiscardPolicy,
        .OnHalt = OnHalt,
    };
    solidSyslog = SolidSyslog_Create(&config);

    shutdownFlag = false;
    HANDLE serviceThread = (HANDLE) _beginthreadex(NULL, 0, ServiceThreadEntry, (void*) &shutdownFlag, 0, NULL);

    struct SolidSyslogMessage message = {
        .Facility = options.Facility,
        .Severity = options.Severity,
        .MessageId = options.MessageId,
        .Msg = options.Msg,
    };

    BddTargetInteractive_Run(solidSyslog, &message, stdin, BddTargetSwitchConfig_SetByName, OnSet, NULL);

    shutdownFlag = true;
    WaitForSingleObject(serviceThread, INFINITE);
    CloseHandle(serviceThread);

    SolidSyslog_Destroy(solidSyslog);
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
