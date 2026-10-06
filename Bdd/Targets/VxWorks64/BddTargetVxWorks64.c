/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/* Built at C99 into the BDD target's own archive (bddtarget-vxworks64.mk).
 * BddTargetVxWorks64Headers.c is the C89 proof of the public headers. */

#include "vxWorks.h"

#include "BddTargetVxWorks64.h"

#include "edrLib.h"
#include "errnoLib.h"
#include "hostLib.h"
#include "routeLib.h"
#include "semLib.h"
#include "taskLib.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "BddTargetEnterpriseId.h"
#include "BddTargetVxWorks64Clock.h"
#include "BddTargetErrorText.h"
#include "BddTargetInteractive.h"
#include "BddTargetIps.h"
#include "BddTargetLanguage.h"
#include "BddTargetMessageSettings.h"
#include "BddTargetStoreSettings.h"
#include "BddTargetSwitchConfig.h"
#include "BddTargetVxWorks64Store.h"
#include "SolidSyslog.h"
#include "SolidSyslogBlockStore.h"
#include "SolidSyslogCircularBuffer.h"
#include "SolidSyslogConfig.h"
#include "SolidSyslogCrc16Policy.h"
#include "SolidSyslogError.h"
#include "SolidSyslogFileBlockDevice.h"
#include "SolidSyslogMetaSd.h"
#include "SolidSyslogNullSecurityPolicy.h"
#include "SolidSyslogNullStore.h"
#include "SolidSyslogOriginSd.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogStreamSender.h"
#include "SolidSyslogSwitchingSender.h"
#include "SolidSyslogTimeQuality.h"
#include "SolidSyslogTimeQualitySd.h"
#include "SolidSyslogUdpSender.h"
#include "SolidSyslogVxWorks64Address.h"
#include "SolidSyslogVxWorks64AtomicCounter.h"
#include "SolidSyslogVxWorks64Clock.h"
#include "SolidSyslogVxWorks64Datagram.h"
#include "SolidSyslogVxWorks64File.h"
#include "SolidSyslogVxWorks64Hostname.h"
#include "SolidSyslogVxWorks64Mutex.h"
#include "SolidSyslogVxWorks64Resolver.h"
#include "SolidSyslogVxWorks64Sleep.h"
#include "SolidSyslogVxWorks64SysUpTime.h"
#include "SolidSyslogVxWorks64TcpStream.h"

#define BDD_TARGET_TAG "SolidSyslog VxWorks 6.4 BDD target: "

enum
{
    TASK_PRIORITY = 100,
    INTERACTIVE_STACK_BYTES = 16384,
    SERVICE_STACK_BYTES = 8192,
    IDLE_YIELD_MILLISECONDS = 1,
    READY_YIELD_MILLISECONDS = 0
};

static FILE* BddTargetVxWorks64_Reports(void);
static void BddTargetVxWorks64_ReportError(void* context, const struct SolidSyslogErrorEvent* event);
static void BddTargetVxWorks64_ShowFaults(void);
static void BddTargetVxWorks64_CreateLoggerLock(void);
static void BddTargetVxWorks64_RunCore(void);
static void BddTargetVxWorks64_BringUpNetwork(void);
static void BddTargetVxWorks64_ReportStep(const char* step, STATUS status);
static void BddTargetVxWorks64_BuildPipeline(void);
static struct SolidSyslogSender* BddTargetVxWorks64_CreateSenders(void);
static void BddTargetVxWorks64_SpawnTasks(void);
static void BddTargetVxWorks64_Spawn(char* name, int stackBytes, FUNCPTR entry);
static int BddTargetVxWorks64_InteractiveTask(void);
static int BddTargetVxWorks64_ServiceTask(void);
static void BddTargetVxWorks64_GetTimeQuality(struct SolidSyslogTimeQuality* timeQuality);
static bool BddTargetVxWorks64_SetByName(const char* name, const char* value);
static bool BddTargetVxWorks64_SetTime(const char* value);
static bool BddTargetVxWorks64_SetStore(const char* value);
static bool BddTargetVxWorks64_ReadyTheDisk(void);
static bool BddTargetVxWorks64_CarriesThePolicy(void);
static void BddTargetVxWorks64_UseFileStore(void);
static void BddTargetVxWorks64_DestroyStore(void);
static void BddTargetVxWorks64_OnStoreFull(void* context);
static void BddTargetVxWorks64_OnThresholdCrossed(void* context);
static enum SolidSyslogServiceStatus BddTargetVxWorks64_ServiceStep(void);

/* Every field NULL: Core falls back to its Null buffer and sender. */
static const struct SolidSyslogConfig CORE_ONLY_CONFIG = {0};

/* The QEMU gateway, until the harness names the collector with `set host`. */
static const char DEFAULT_HOST[] = "10.0.2.2";

/* The store's block files, at the root of the target's disk: /ata0a/STORE00.log
 * and on, which fit dosFs's 8.3 names. */
static const char STORE_PATH_PREFIX[] = "/ata0a/STORE";

static uint8_t bufferRing[SOLIDSYSLOG_CIRCULAR_BUFFER_RING_BYTES(8)];
static struct SolidSyslogResolver* resolver;
static struct SolidSyslogAddress* udpAddress;
static struct SolidSyslogDatagram* datagram;
static struct SolidSyslogSender* udpSender;
static struct SolidSyslogAddress* tcpAddress;
static struct SolidSyslogStream* tcpStream;
static struct SolidSyslogSender* tcpSender;
static struct SolidSyslogSender* senders[BDD_TARGET_SWITCH_TCP + 1];
static struct SolidSyslogSender* sender;
static struct SolidSyslogMutex* bufferMutex;
static struct SolidSyslogBuffer* buffer;
static struct SolidSyslogAtomicCounter* counter;
static struct SolidSyslogStructuredData* metaSd;
static struct SolidSyslogStructuredData* timeQualitySd;
static struct SolidSyslogStructuredData* originSd;
static struct SolidSyslogStructuredData* sdList[3];
static struct SolidSyslogConfig loggerConfig;
static struct SolidSyslog* logger;
/* The NullStore until `set store file`, and the file store after. */
static struct SolidSyslogStore* store;
static bool storeIsFile;
static struct SolidSyslogFile* storeFile;
static struct SolidSyslogBlockDevice* storeBlockDevice;
static struct SolidSyslogSecurityPolicy* storePolicy;
/* Held while the logger is serviced or replaced. SolidSyslog is a one-slot
 * pool, so a replaced logger occupies the slot the console task was handed;
 * the lock keeps the service task out while it is rebuilt. */
static SEM_ID loggerLock;
/* Set when the console ends, which stops the service task. */
static volatile bool consoleEnded;
/* Where the target's own reports go; NULL means stdout, the console. */
static FILE* reportStream;

void BddTargetVxWorks64_Init(void)
{
    BddTargetVxWorks64_ShowFaults();
    BddTargetVxWorks64_RunCore();
    /* After the Core-only check, whose deliberately empty config is reported
     * as bad by design. */
    SolidSyslog_SetErrorHandler(BddTargetVxWorks64_ReportError, NULL);
    BddTargetVxWorks64_BringUpNetwork();
    BddTargetVxWorks64_BuildPipeline();
    BddTargetVxWorks64_CreateLoggerLock();
    consoleEnded = false;
    BddTargetVxWorks64_SpawnTasks();
}

/* A test target has to show its faults. In ED&R's deployed policy a fatal error
 * in a task reboots the target without a word; in its debug policy the task
 * stops and the exception is printed. The boot line's flag cannot select it on
 * this image, so it is set here, and the policy that took is reported. */
static void BddTargetVxWorks64_ShowFaults(void)
{
    edrSystemDebugModeSet(TRUE);
    (void) fprintf(
        BddTargetVxWorks64_Reports(),
        BDD_TARGET_TAG "ED&R debug policy %s\n",
        edrSystemDebugModeGet() ? "on" : "off"
    );
}

/* Without the lock the service task would run against a logger being replaced,
 * so a kernel that cannot make it is reported, as a task that will not start is. */
static void BddTargetVxWorks64_CreateLoggerLock(void)
{
    loggerLock = semMCreate(SEM_Q_PRIORITY | SEM_INVERSION_SAFE | SEM_DELETE_SAFE);
    if (loggerLock == NULL)
    {
        (void) fprintf(
            BddTargetVxWorks64_Reports(),
            BDD_TARGET_TAG "logger lock not created, errno 0x%x\n",
            (unsigned) errnoGet()
        );
    }
}

/* Reports what the library reports, in the form the steps read from every QEMU
 * target, so a failed send shows on the console rather than as a silent timeout. */
static void BddTargetVxWorks64_ReportError(void* context, const struct SolidSyslogErrorEvent* event)
{
    (void) context;
    const char* sourceName = (event->Source != NULL) ? event->Source->Name : "<unknown>";
    (void) fprintf(
        BddTargetVxWorks64_Reports(),
        "[solidsyslog] severity=%d [%s cat=%u detail=%ld] %s\n",
        (int) event->Severity,
        sourceName,
        (unsigned) event->Category,
        (long) event->Detail,
        BddTargetErrorText_Category(event->Category)
    );
}

static FILE* BddTargetVxWorks64_Reports(void)
{
    return (reportStream != NULL) ? reportStream : stdout;
}

static void BddTargetVxWorks64_RunCore(void)
{
    struct SolidSyslogMessage message = {0};

    message.Facility = SOLIDSYSLOG_FACILITY_USER;
    message.Severity = SOLIDSYSLOG_SEVERITY_INFORMATIONAL;
    message.MessageId = "BOOT";
    message.Msg = "VxWorks 6.4 BDD target";

    struct SolidSyslog* coreOnly = SolidSyslog_Create(&CORE_ONLY_CONFIG);
    SolidSyslog_Log(coreOnly, &message);
    SolidSyslog_Destroy(coreOnly);

    (void) fprintf(BddTargetVxWorks64_Reports(), BDD_TARGET_TAG "Core ran\n");
}

/* INCLUDE_ADDIF has already put the PCnet interface on QEMU's user network, as
 * 10.0.2.15/24. The collector lies beyond that network, so the default route
 * goes through QEMU's gateway, which is always 10.0.2.2. */
static void BddTargetVxWorks64_BringUpNetwork(void)
{
    char anyDestination[] = "0.0.0.0";
    char gateway[] = "10.0.2.2";
    char hostname[] = "SolidSyslogVxWorks64";

    BddTargetVxWorks64_ReportStep("default route", routeAdd(anyDestination, gateway));
    BddTargetVxWorks64_ReportStep("hostname", sethostname(hostname, (int) sizeof(hostname)));
}

static void BddTargetVxWorks64_ReportStep(const char* step, STATUS status)
{
    if (status == OK)
    {
        (void) fprintf(BddTargetVxWorks64_Reports(), BDD_TARGET_TAG "network %s set\n", step);
    }
    else
    {
        (void) fprintf(
            BddTargetVxWorks64_Reports(),
            BDD_TARGET_TAG "network %s failed, errno 0x%x\n",
            step,
            (unsigned) errnoGet()
        );
    }
}

/* UDP or TCP to the collector the harness names, behind a circular buffer that
 * the VxWorks mutex guards, because the console task logs and the service task
 * sends. */
static void BddTargetVxWorks64_BuildPipeline(void)
{
    struct SolidSyslogConfig config = {0};

    store = SolidSyslogNullStore_Get();
    storeIsFile = false;
    BddTargetMessageSettings_Reset(DEFAULT_HOST);
    BddTargetStoreSettings_Reset();
    sender = BddTargetVxWorks64_CreateSenders();

    bufferMutex = SolidSyslogVxWorks64Mutex_Create();
    buffer = SolidSyslogCircularBuffer_Create(bufferMutex, bufferRing, sizeof(bufferRing));

    counter = SolidSyslogVxWorks64AtomicCounter_Create();
    struct SolidSyslogMetaSdConfig metaConfig = {0};
    metaConfig.Counter = counter;
    metaConfig.GetSysUpTime = SolidSyslogVxWorks64_GetSysUpTime;
    metaConfig.GetLanguage = BddTargetLanguage_Get;
    metaSd = SolidSyslogMetaSd_Create(&metaConfig);

    struct SolidSyslogOriginSdConfig originConfig = {0};
    originConfig.Software = "SolidSyslogBddTarget";
    originConfig.SwVersion = "0.7.0";
    originConfig.EnterpriseId = BDD_TARGET_ENTERPRISE_ID;
    originConfig.GetIpCount = BddTargetIps_Count;
    originConfig.GetIpAt = BddTargetIps_At;
    originSd = SolidSyslogOriginSd_Create(&originConfig);
    timeQualitySd = SolidSyslogTimeQualitySd_Create(BddTargetVxWorks64_GetTimeQuality);
    sdList[0] = metaSd;
    sdList[1] = timeQualitySd;
    sdList[2] = originSd;

    config.Buffer = buffer;
    config.Sender = sender;
    config.Sd = sdList;
    config.SdCount = sizeof(sdList) / sizeof(sdList[0]);
    config.Store = store;
    config.GetAppName = BddTargetMessageSettings_GetAppName;
    config.Clock = SolidSyslogVxWorks64_GetTimestamp;
    config.GetHostname = SolidSyslogVxWorks64_GetHostname;
    loggerConfig = config;
    logger = SolidSyslog_Create(&loggerConfig);
}

/* Both transports go to the same collector, and `set transport` or `switch`
 * picks between them, UDP to begin with. Selecting a transport this target does
 * not have, such as tls, routes to the Null sender, which drops the record. */
static struct SolidSyslogSender* BddTargetVxWorks64_CreateSenders(void)
{
    struct SolidSyslogUdpSenderConfig udpConfig = {0};
    struct SolidSyslogStreamSenderConfig tcpConfig = {0};
    struct SolidSyslogSwitchingSenderConfig switchConfig = {0};

    resolver = SolidSyslogVxWorks64Resolver_Create();

    udpAddress = SolidSyslogVxWorks64Address_Create();
    datagram = SolidSyslogVxWorks64Datagram_Create();
    udpConfig.Resolver = resolver;
    udpConfig.Datagram = datagram;
    udpConfig.Address = udpAddress;
    udpConfig.Endpoint = BddTargetMessageSettings_GetEndpoint;
    udpConfig.EndpointVersion = BddTargetMessageSettings_GetEndpointVersion;
    udpSender = SolidSyslogUdpSender_Create(&udpConfig);

    tcpAddress = SolidSyslogVxWorks64Address_Create();
    tcpStream = SolidSyslogVxWorks64TcpStream_Create(NULL);
    tcpConfig.Resolver = resolver;
    tcpConfig.Stream = tcpStream;
    tcpConfig.Address = tcpAddress;
    tcpConfig.Endpoint = BddTargetMessageSettings_GetEndpoint;
    tcpConfig.EndpointVersion = BddTargetMessageSettings_GetEndpointVersion;
    tcpSender = SolidSyslogStreamSender_Create(&tcpConfig);

    senders[BDD_TARGET_SWITCH_UDP] = udpSender;
    senders[BDD_TARGET_SWITCH_TCP] = tcpSender;
    switchConfig.Senders = senders;
    switchConfig.SenderCount = sizeof(senders) / sizeof(senders[0]);
    switchConfig.Selector = BddTargetSwitchConfig_Selector;
    BddTargetSwitchConfig_SetByName("udp");
    return SolidSyslogSwitchingSender_Create(&switchConfig);
}

/* The harness sets the clock, in UTC, with `set time` before it sends anything,
 * so the time is in a known zone and synchronised to the host's (RFC 5424
 * §7.1). How closely is not measured, so syncAccuracy is left out. */
static void BddTargetVxWorks64_GetTimeQuality(struct SolidSyslogTimeQuality* timeQuality)
{
    timeQuality->TzKnown = true;
    timeQuality->IsSynced = true;
    timeQuality->SyncAccuracyMicroseconds = SOLIDSYSLOG_SYNC_ACCURACY_OMIT;
}

/* taskSpawn takes a non-const name it only reads, so each lives in an array. */
static void BddTargetVxWorks64_SpawnTasks(void)
{
    char interactiveName[] = "tSsInteractive";
    char serviceName[] = "tSsService";

    BddTargetVxWorks64_Spawn(interactiveName, INTERACTIVE_STACK_BYTES, (FUNCPTR) BddTargetVxWorks64_InteractiveTask);
    BddTargetVxWorks64_Spawn(serviceName, SERVICE_STACK_BYTES, (FUNCPTR) BddTargetVxWorks64_ServiceTask);
}

static void BddTargetVxWorks64_Spawn(char* name, int stackBytes, FUNCPTR entry)
{
    if (taskSpawn(name, TASK_PRIORITY, 0, stackBytes, entry, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0) == ERROR)
    {
        (void) fprintf(
            BddTargetVxWorks64_Reports(),
            BDD_TARGET_TAG "task %s failed to start, errno 0x%x\n",
            name,
            (unsigned) errnoGet()
        );
    }
}

static int BddTargetVxWorks64_InteractiveTask(void)
{
    BddTargetVxWorks64_RunConsole(stdin);
    return 0;
}

static int BddTargetVxWorks64_ServiceTask(void)
{
    BddTargetVxWorks64_RunService();
    return 0;
}

void BddTargetVxWorks64_RunConsole(FILE* input)
{
    BddTargetInteractive_Run(
        logger,
        BddTargetMessageSettings_Message(),
        input,
        BddTargetSwitchConfig_SetByName,
        BddTargetVxWorks64_SetByName
    );
    consoleEnded = true;
}

/* `set time` and `set transport` are this target's; every other setting is the
 * shared one. */
static bool BddTargetVxWorks64_SetByName(const char* name, const char* value)
{
    bool taken = false;

    if (strcmp(name, "time") == 0)
    {
        taken = BddTargetVxWorks64_SetTime(value);
    }
    else if (strcmp(name, "transport") == 0)
    {
        BddTargetSwitchConfig_SetByName(value);
        taken = true;
    }
    else if (strcmp(name, "store") == 0)
    {
        taken = BddTargetVxWorks64_SetStore(value);
    }
    else
    {
        taken = BddTargetMessageSettings_SetByName(name, value) || BddTargetStoreSettings_SetByName(name, value);
    }
    return taken;
}

/* The board has no battery-backed clock, so the harness sets it, in seconds
 * since the epoch, and the system clock tick runs it from there. */
static bool BddTargetVxWorks64_SetTime(const char* value)
{
    unsigned long seconds = 0U;
    bool taken = BddTargetMessageSettings_TryParseNumber(value, &seconds);

    if (taken)
    {
        taken = BddTargetVxWorks64Clock_Set(seconds);
    }
    return taken;
}

/* `set store null` keeps the NullStore the target boots with; `set store file`
 * moves the logger onto the file store on the target's disk. */
static bool BddTargetVxWorks64_SetStore(const char* value)
{
    bool taken = strcmp(value, "null") == 0;
    if (strcmp(value, "file") == 0)
    {
        taken = BddTargetVxWorks64_CarriesThePolicy() && BddTargetVxWorks64_ReadyTheDisk();
        if (taken)
        {
            BddTargetVxWorks64_UseFileStore();
        }
    }
    return taken;
}

/* The kernel's own reason, so a disk that will not mount or format can be told
 * apart from a store the harness refused. */
static bool BddTargetVxWorks64_ReadyTheDisk(void)
{
    bool ready = BddTargetVxWorks64Store_Mount(BDD_TARGET_VXWORKS64_FILE_SYSTEM_DOSFS);
    if (!ready)
    {
        (void) fprintf(
            BddTargetVxWorks64_Reports(),
            BDD_TARGET_TAG "store disk not ready, errno 0x%x\n",
            (unsigned) errnoGet()
        );
    }
    return ready;
}

/* No TLS library comes with this platform, so the policies that need one are
 * refused rather than quietly replaced. */
static bool BddTargetVxWorks64_CarriesThePolicy(void)
{
    enum BddTargetSecurityPolicy policy = BddTargetStoreSettings_SecurityPolicy();
    return (policy == BDD_TARGET_SECURITY_POLICY_CRC16) || (policy == BDD_TARGET_SECURITY_POLICY_NULL);
}

/* The logger is rebuilt over the file store with the settings the harness sent
 * before `set store file`, holding the lock so the service task waits. */
static void BddTargetVxWorks64_UseFileStore(void)
{
    struct SolidSyslogBlockStoreConfig storeConfig = {0};

    (void) semTake(loggerLock, WAIT_FOREVER);
    SolidSyslog_Destroy(logger);
    BddTargetVxWorks64_DestroyStore();

    storeFile = SolidSyslogVxWorks64File_Create();
    storeBlockDevice =
        SolidSyslogFileBlockDevice_Create(storeFile, STORE_PATH_PREFIX, BddTargetStoreSettings_MaxBlockSize());
    storePolicy = (BddTargetStoreSettings_SecurityPolicy() == BDD_TARGET_SECURITY_POLICY_CRC16)
                      ? SolidSyslogCrc16Policy_Create()
                      : SolidSyslogNullSecurityPolicy_Get();
    storeConfig.BlockDevice = storeBlockDevice;
    storeConfig.MaxBlocks = BddTargetStoreSettings_MaxBlocks();
    storeConfig.DiscardPolicy = BddTargetStoreSettings_DiscardPolicy();
    storeConfig.SecurityPolicy = storePolicy;
    storeConfig.OnStoreFull = BddTargetVxWorks64_OnStoreFull;
    storeConfig.GetCapacityThreshold = BddTargetStoreSettings_GetCapacityThreshold;
    storeConfig.OnThresholdCrossed = BddTargetVxWorks64_OnThresholdCrossed;
    store = SolidSyslogBlockStore_Create(&storeConfig);
    storeIsFile = true;

    loggerConfig.Store = store;
    loggerConfig.SdCount = BddTargetStoreSettings_NoSd() ? 1U : (sizeof(sdList) / sizeof(sdList[0]));
    logger = SolidSyslog_Create(&loggerConfig);
    (void) semGive(loggerLock);
}

/* The target cannot hand QEMU an exit status, so it prints the one a halted
 * run ends with, and the harness's console stops the target when it reads it. */
static void BddTargetVxWorks64_OnStoreFull(void* context)
{
    (void) context;
    if (BddTargetStoreSettings_HaltExit())
    {
        (void) fprintf(BddTargetVxWorks64_Reports(), "[EXIT 2]\n");
    }
}

/* The marker the harness's threshold step looks for on the console. */
static void BddTargetVxWorks64_OnThresholdCrossed(void* context)
{
    (void) context;
    (void) fprintf(BddTargetVxWorks64_Reports(), "[THRESHOLD-CROSSED]\n");
}

/* The NullStore is shared and has nothing to release. */
static void BddTargetVxWorks64_DestroyStore(void)
{
    if (storeIsFile)
    {
        SolidSyslogBlockStore_Destroy(store);
        SolidSyslogFileBlockDevice_Destroy(storeBlockDevice);
        if (storePolicy != SolidSyslogNullSecurityPolicy_Get())
        {
            SolidSyslogCrc16Policy_Destroy();
        }
        SolidSyslogVxWorks64File_Destroy(storeFile);
    }
    store = SolidSyslogNullStore_Get();
    storeIsFile = false;
}

/* Once the console ends, what it logged is still sent before the task stops. */
void BddTargetVxWorks64_RunService(void)
{
    while (!consoleEnded)
    {
        /* More work ready now loops at once; otherwise yield, so an idle
         * target does not spin. */
        bool ready = BddTargetVxWorks64_ServiceStep() == SOLIDSYSLOG_SERVICE_READY;
        SolidSyslogVxWorks64_Sleep(ready ? READY_YIELD_MILLISECONDS : IDLE_YIELD_MILLISECONDS);
    }
    while (BddTargetVxWorks64_ServiceStep() == SOLIDSYSLOG_SERVICE_READY)
    {
    }
}

static enum SolidSyslogServiceStatus BddTargetVxWorks64_ServiceStep(void)
{
    (void) semTake(loggerLock, WAIT_FOREVER);
    enum SolidSyslogServiceStatus status = SolidSyslog_Service(logger);
    (void) semGive(loggerLock);
    return status;
}

void BddTargetVxWorks64_Teardown(void)
{
    SolidSyslog_Destroy(logger);
    BddTargetVxWorks64_DestroyStore();
    SolidSyslogOriginSd_Destroy(originSd);
    SolidSyslogTimeQualitySd_Destroy(timeQualitySd);
    SolidSyslogMetaSd_Destroy(metaSd);
    SolidSyslogVxWorks64AtomicCounter_Destroy(counter);
    SolidSyslogCircularBuffer_Destroy(buffer);
    SolidSyslogVxWorks64Mutex_Destroy(bufferMutex);
    SolidSyslogSwitchingSender_Destroy(sender);
    SolidSyslogStreamSender_Destroy(tcpSender);
    SolidSyslogVxWorks64TcpStream_Destroy(tcpStream);
    SolidSyslogVxWorks64Address_Destroy(tcpAddress);
    SolidSyslogUdpSender_Destroy(udpSender);
    SolidSyslogVxWorks64Datagram_Destroy(datagram);
    SolidSyslogVxWorks64Address_Destroy(udpAddress);
    SolidSyslogVxWorks64Resolver_Destroy(resolver);
    (void) semDelete(loggerLock);
    SolidSyslog_SetErrorHandler(NULL, NULL);
}

void BddTargetVxWorks64_ReportTo(FILE* stream)
{
    reportStream = stream;
}
