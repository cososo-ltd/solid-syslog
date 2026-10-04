/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/* Built at C99 into the BDD target's own archive (bddtarget-vxworks64.mk).
 * BddTargetVxWorks64Headers.c is the C89 proof of the public headers. */

#include "vxWorks.h"

#include "BddTargetVxWorks64.h"

#include "errnoLib.h"
#include "hostLib.h"
#include "routeLib.h"
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
#include "BddTargetMessageSettings.h"
#include "BddTargetServiceThread.h"
#include "SolidSyslog.h"
#include "SolidSyslogCircularBuffer.h"
#include "SolidSyslogConfig.h"
#include "SolidSyslogError.h"
#include "SolidSyslogMetaSd.h"
#include "SolidSyslogNullStore.h"
#include "SolidSyslogOriginSd.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogTimeQuality.h"
#include "SolidSyslogTimeQualitySd.h"
#include "SolidSyslogUdpSender.h"
#include "SolidSyslogVxWorks64Address.h"
#include "SolidSyslogVxWorks64AtomicCounter.h"
#include "SolidSyslogVxWorks64Clock.h"
#include "SolidSyslogVxWorks64Datagram.h"
#include "SolidSyslogVxWorks64Hostname.h"
#include "SolidSyslogVxWorks64Mutex.h"
#include "SolidSyslogVxWorks64Resolver.h"
#include "SolidSyslogVxWorks64Sleep.h"
#include "SolidSyslogVxWorks64SysUpTime.h"

#define BDD_TARGET_TAG "SolidSyslog VxWorks 6.4 BDD target: "

enum
{
    TASK_PRIORITY = 100,
    INTERACTIVE_STACK_BYTES = 16384,
    SERVICE_STACK_BYTES = 8192
};

static FILE* BddTargetVxWorks64_Reports(void);
static void BddTargetVxWorks64_ReportError(void* context, const struct SolidSyslogErrorEvent* event);
static void BddTargetVxWorks64_RunCore(void);
static void BddTargetVxWorks64_BringUpNetwork(void);
static void BddTargetVxWorks64_ReportStep(const char* step, STATUS status);
static void BddTargetVxWorks64_BuildPipeline(void);
static void BddTargetVxWorks64_SpawnTasks(void);
static void BddTargetVxWorks64_Spawn(char* name, int stackBytes, FUNCPTR entry);
static int BddTargetVxWorks64_InteractiveTask(void);
static int BddTargetVxWorks64_ServiceTask(void);
static void BddTargetVxWorks64_GetTimeQuality(struct SolidSyslogTimeQuality* timeQuality);
static bool BddTargetVxWorks64_SetByName(const char* name, const char* value);
static bool BddTargetVxWorks64_SetTime(const char* value);

/* Every field NULL: Core falls back to its Null buffer and sender. */
static const struct SolidSyslogConfig CORE_ONLY_CONFIG = {0};

/* The QEMU gateway, until the harness names the collector with `set host`. */
static const char DEFAULT_HOST[] = "10.0.2.2";

static uint8_t bufferRing[SOLIDSYSLOG_CIRCULAR_BUFFER_RING_BYTES(8)];
static struct SolidSyslogAddress* address;
static struct SolidSyslogResolver* resolver;
static struct SolidSyslogDatagram* datagram;
static struct SolidSyslogSender* sender;
static struct SolidSyslogMutex* bufferMutex;
static struct SolidSyslogBuffer* buffer;
static struct SolidSyslogAtomicCounter* counter;
static struct SolidSyslogStructuredData* metaSd;
static struct SolidSyslogStructuredData* timeQualitySd;
static struct SolidSyslogStructuredData* originSd;
static struct SolidSyslogStructuredData* sdList[3];
static struct SolidSyslog* logger;
/* Set when the console ends, which stops the service task. */
static volatile bool consoleEnded;
/* Where the target's own reports go; NULL means stdout, the console. */
static FILE* reportStream;

void BddTargetVxWorks64_Init(void)
{
    BddTargetVxWorks64_RunCore();
    /* After the Core-only check, whose deliberately empty config is reported
     * as bad by design. */
    SolidSyslog_SetErrorHandler(BddTargetVxWorks64_ReportError, NULL);
    BddTargetVxWorks64_BringUpNetwork();
    BddTargetVxWorks64_BuildPipeline();
    consoleEnded = false;
    BddTargetVxWorks64_SpawnTasks();
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

/* UDP to the collector the harness names, behind a circular buffer that the
 * VxWorks mutex guards, because the console task logs and the service task
 * sends. */
static void BddTargetVxWorks64_BuildPipeline(void)
{
    struct SolidSyslogUdpSenderConfig senderConfig = {0};
    struct SolidSyslogConfig config = {0};

    BddTargetMessageSettings_Reset(DEFAULT_HOST);

    address = SolidSyslogVxWorks64Address_Create();
    resolver = SolidSyslogVxWorks64Resolver_Create();
    datagram = SolidSyslogVxWorks64Datagram_Create();
    senderConfig.Resolver = resolver;
    senderConfig.Datagram = datagram;
    senderConfig.Address = address;
    senderConfig.Endpoint = BddTargetMessageSettings_GetEndpoint;
    senderConfig.EndpointVersion = BddTargetMessageSettings_GetEndpointVersion;
    sender = SolidSyslogUdpSender_Create(&senderConfig);

    bufferMutex = SolidSyslogVxWorks64Mutex_Create();
    buffer = SolidSyslogCircularBuffer_Create(bufferMutex, bufferRing, sizeof(bufferRing));

    counter = SolidSyslogVxWorks64AtomicCounter_Create();
    struct SolidSyslogMetaSdConfig metaConfig = {0};
    metaConfig.Counter = counter;
    metaConfig.GetSysUpTime = SolidSyslogVxWorks64_GetSysUpTime;
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
    config.Store = SolidSyslogNullStore_Get();
    config.GetAppName = BddTargetMessageSettings_GetAppName;
    config.Clock = SolidSyslogVxWorks64_GetTimestamp;
    config.GetHostname = SolidSyslogVxWorks64_GetHostname;
    logger = SolidSyslog_Create(&config);
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
    BddTargetInteractive_Run(logger, BddTargetMessageSettings_Message(), input, NULL, BddTargetVxWorks64_SetByName);
    consoleEnded = true;
}

/* `set time` is this target's; every other setting is the shared one. */
static bool BddTargetVxWorks64_SetByName(const char* name, const char* value)
{
    bool taken = false;

    if (strcmp(name, "time") == 0)
    {
        taken = BddTargetVxWorks64_SetTime(value);
    }
    else
    {
        taken = BddTargetMessageSettings_SetByName(name, value);
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

/* Once the console ends, what it logged is still sent before the task stops. */
void BddTargetVxWorks64_RunService(void)
{
    BddTargetServiceThread_Run(logger, &consoleEnded, SolidSyslogVxWorks64_Sleep);
    while (SolidSyslog_Service(logger) == SOLIDSYSLOG_SERVICE_READY)
    {
    }
}

void BddTargetVxWorks64_Teardown(void)
{
    SolidSyslog_Destroy(logger);
    SolidSyslogOriginSd_Destroy(originSd);
    SolidSyslogTimeQualitySd_Destroy(timeQualitySd);
    SolidSyslogMetaSd_Destroy(metaSd);
    SolidSyslogVxWorks64AtomicCounter_Destroy(counter);
    SolidSyslogCircularBuffer_Destroy(buffer);
    SolidSyslogVxWorks64Mutex_Destroy(bufferMutex);
    SolidSyslogUdpSender_Destroy(sender);
    SolidSyslogVxWorks64Datagram_Destroy(datagram);
    SolidSyslogVxWorks64Resolver_Destroy(resolver);
    SolidSyslogVxWorks64Address_Destroy(address);
    SolidSyslog_SetErrorHandler(NULL, NULL);
}

void BddTargetVxWorks64_ReportTo(FILE* stream)
{
    reportStream = stream;
}
