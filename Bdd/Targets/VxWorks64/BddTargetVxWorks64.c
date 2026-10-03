/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/* Built at C99 into the BDD target's own archive (bddtarget-vxworks64.mk).
 * BddTargetVxWorks64Headers.c is the C89 proof of the public headers. */

#include "vxWorks.h"

#include "errnoLib.h"
#include "routeLib.h"
#include "taskLib.h"

#include <stdio.h>

#include "SolidSyslog.h"
#include "SolidSyslogConfig.h"
#include "SolidSyslogError.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogVxWorks64Mutex.h"

#define BDD_TARGET_TAG "SolidSyslog VxWorks 6.4 BDD target: "

enum
{
    TASK_PRIORITY = 100,
    INTERACTIVE_STACK_BYTES = 16384,
    SERVICE_STACK_BYTES = 8192
};

void BddTargetVxWorks64_Init(void);

static void BddTargetVxWorks64_RunCore(void);
static void BddTargetVxWorks64_BringUpNetwork(void);
static void BddTargetVxWorks64_ReportStep(const char* step, STATUS status);
static void BddTargetVxWorks64_SpawnTasks(void);
static int BddTargetVxWorks64_InteractiveTask(void);
static int BddTargetVxWorks64_ServiceTask(void);

/* Every field NULL: Core falls back to its Null buffer and sender. */
static const struct SolidSyslogConfig CORE_ONLY_CONFIG;

static struct SolidSyslogMutex* bufferMutex;

void BddTargetVxWorks64_Init(void)
{
    BddTargetVxWorks64_RunCore();
    BddTargetVxWorks64_BringUpNetwork();
    bufferMutex = SolidSyslogVxWorks64Mutex_Create();
    BddTargetVxWorks64_SpawnTasks();
}

static void BddTargetVxWorks64_RunCore(void)
{
    struct SolidSyslog* logger;
    struct SolidSyslogMessage message;

    message.Facility = SOLIDSYSLOG_FACILITY_USER;
    message.Severity = SOLIDSYSLOG_SEVERITY_INFORMATIONAL;
    message.MessageId = "BOOT";
    message.Msg = "VxWorks 6.4 BDD target";

    logger = SolidSyslog_Create(&CORE_ONLY_CONFIG);
    SolidSyslog_Log(logger, &message);
    SolidSyslog_Destroy(logger);

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
    return 0;
}

static int BddTargetVxWorks64_ServiceTask(void)
{
    return 0;
}
