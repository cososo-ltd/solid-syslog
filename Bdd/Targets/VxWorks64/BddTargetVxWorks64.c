/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/* Built at C99 into the BDD target's own archive (bddtarget-vxworks64.mk).
 * BddTargetVxWorks64Headers.c is the C89 proof of the public headers. */

#include "vxWorks.h"

#include "BddTargetVxWorks64.h"

#include "errnoLib.h"
#include "routeLib.h"
#include "taskLib.h"

#include <stdint.h>
#include <stdio.h>

#include "BddTargetInteractive.h"
#include "BddTargetMessageSettings.h"
#include "SolidSyslog.h"
#include "SolidSyslogCircularBuffer.h"
#include "SolidSyslogConfig.h"
#include "SolidSyslogError.h"
#include "SolidSyslogNullStore.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogUdpSender.h"
#include "SolidSyslogVxWorks64Address.h"
#include "SolidSyslogVxWorks64Datagram.h"
#include "SolidSyslogVxWorks64Mutex.h"
#include "SolidSyslogVxWorks64Resolver.h"

#define BDD_TARGET_TAG "SolidSyslog VxWorks 6.4 BDD target: "

enum
{
    TASK_PRIORITY = 100,
    INTERACTIVE_STACK_BYTES = 16384,
    SERVICE_STACK_BYTES = 8192
};

static void BddTargetVxWorks64_RunCore(void);
static void BddTargetVxWorks64_BringUpNetwork(void);
static void BddTargetVxWorks64_ReportStep(const char* step, STATUS status);
static void BddTargetVxWorks64_BuildPipeline(void);
static void BddTargetVxWorks64_SpawnTasks(void);
static int BddTargetVxWorks64_InteractiveTask(void);
static int BddTargetVxWorks64_ServiceTask(void);

/* Every field NULL: Core falls back to its Null buffer and sender. */
static const struct SolidSyslogConfig CORE_ONLY_CONFIG;

/* The QEMU gateway, until the harness names the collector with `set host`. */
static const char DEFAULT_HOST[] = "10.0.2.2";

static uint8_t bufferRing[SOLIDSYSLOG_CIRCULAR_BUFFER_RING_BYTES(8)];
static struct SolidSyslogAddress* address;
static struct SolidSyslogResolver* resolver;
static struct SolidSyslogDatagram* datagram;
static struct SolidSyslogSender* sender;
static struct SolidSyslogMutex* bufferMutex;
static struct SolidSyslogBuffer* buffer;
static struct SolidSyslog* logger;

void BddTargetVxWorks64_Init(void)
{
    BddTargetVxWorks64_RunCore();
    BddTargetVxWorks64_BringUpNetwork();
    BddTargetVxWorks64_BuildPipeline();
    BddTargetVxWorks64_SpawnTasks();
}

void BddTargetVxWorks64_Teardown(void)
{
    SolidSyslog_Destroy(logger);
    SolidSyslogCircularBuffer_Destroy(buffer);
    SolidSyslogVxWorks64Mutex_Destroy(bufferMutex);
    SolidSyslogUdpSender_Destroy(sender);
    SolidSyslogVxWorks64Datagram_Destroy(datagram);
    SolidSyslogVxWorks64Resolver_Destroy(resolver);
    SolidSyslogVxWorks64Address_Destroy(address);
}

void BddTargetVxWorks64_RunConsole(FILE* input)
{
    BddTargetInteractive_Run(
        logger,
        BddTargetMessageSettings_Message(),
        input,
        NULL,
        BddTargetMessageSettings_SetByName
    );
}

static void BddTargetVxWorks64_RunCore(void)
{
    struct SolidSyslog* coreOnly;
    struct SolidSyslogMessage message;

    message.Facility = SOLIDSYSLOG_FACILITY_USER;
    message.Severity = SOLIDSYSLOG_SEVERITY_INFORMATIONAL;
    message.MessageId = "BOOT";
    message.Msg = "VxWorks 6.4 BDD target";

    coreOnly = SolidSyslog_Create(&CORE_ONLY_CONFIG);
    SolidSyslog_Log(coreOnly, &message);
    SolidSyslog_Destroy(coreOnly);

    printf(BDD_TARGET_TAG "Core ran\n");
}

/* INCLUDE_ADDIF has already put the PCnet interface on QEMU's user network, as
 * 10.0.2.15/24. The collector lies beyond that network, so the default route
 * goes through QEMU's gateway, which is always 10.0.2.2. */
static void BddTargetVxWorks64_BringUpNetwork(void)
{
    char anyDestination[] = "0.0.0.0";
    char gateway[] = "10.0.2.2";

    BddTargetVxWorks64_ReportStep("default route", routeAdd(anyDestination, gateway));
}

static void BddTargetVxWorks64_ReportStep(const char* step, STATUS status)
{
    if (status == OK)
    {
        printf(BDD_TARGET_TAG "network %s set\n", step);
    }
    else
    {
        printf(BDD_TARGET_TAG "network %s failed, errno 0x%x\n", step, (unsigned) errnoGet());
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

    config.Buffer = buffer;
    config.Sender = sender;
    config.Store = SolidSyslogNullStore_Get();
    config.GetAppName = BddTargetMessageSettings_GetAppName;
    logger = SolidSyslog_Create(&config);
}

static void BddTargetVxWorks64_SpawnTasks(void)
{
    char interactiveName[] = "tSsInteractive";
    char serviceName[] = "tSsService";

    (void) taskSpawn(
        interactiveName,
        TASK_PRIORITY,
        0,
        INTERACTIVE_STACK_BYTES,
        (FUNCPTR) BddTargetVxWorks64_InteractiveTask,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0
    );
    (void) taskSpawn(
        serviceName,
        TASK_PRIORITY,
        0,
        SERVICE_STACK_BYTES,
        (FUNCPTR) BddTargetVxWorks64_ServiceTask,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0
    );
}

static int BddTargetVxWorks64_InteractiveTask(void)
{
    BddTargetVxWorks64_RunConsole(stdin);
    return 0;
}

static int BddTargetVxWorks64_ServiceTask(void)
{
    return 0;
}

void BddTargetVxWorks64_Sleep(int milliseconds)
{
    (void) milliseconds;
    (void) taskDelay(1);
}
