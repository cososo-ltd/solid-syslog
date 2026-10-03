/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/* The VIP compiles this file with its own flags, which select C89, so it stands
 * in for an application: vxWorks.h first, as Wind River code includes it, then
 * the public headers. The VxWorks64 headers are included whether or not they
 * are used, to prove they compile there too. */

#include "vxWorks.h"

#include "errnoLib.h"
#include "routeLib.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "SolidSyslog.h"
#include "SolidSyslogConfig.h"
#include "SolidSyslogEndpoint.h"
#include "SolidSyslogEndpointHost.h"
#include "SolidSyslogError.h"
#include "SolidSyslogNullStore.h"
#include "SolidSyslogPassthroughBuffer.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogUdpSender.h"
#include "SolidSyslogVxWorks64Address.h"
#include "SolidSyslogVxWorks64AddressErrors.h"
#include "SolidSyslogVxWorks64Datagram.h"
#include "SolidSyslogVxWorks64DatagramErrors.h"
#include "SolidSyslogVxWorks64Mutex.h"
#include "SolidSyslogVxWorks64MutexErrors.h"
#include "SolidSyslogVxWorks64Resolver.h"
#include "SolidSyslogVxWorks64ResolverErrors.h"

#define BDD_TARGET_TAG "SolidSyslog VxWorks 6.4 BDD target: "
#define COLLECTOR_COMMAND "collector "
#define COLLECTOR_HOST_SIZE 64
#define COLLECTOR_LINE_SIZE 128
#define MAX_PORT 65535L

void BddTargetVxWorks64_Init(void);

static void BddTargetVxWorks64_RunCore(void);
static void BddTargetVxWorks64_BringUpNetwork(void);
static void BddTargetVxWorks64_ReportStep(const char* step, STATUS status);
static void BddTargetVxWorks64_SendOverUdp(void);
static int BddTargetVxWorks64_ParseCollector(const char* line);
static void BddTargetVxWorks64_Endpoint(struct SolidSyslogEndpoint* endpoint, void* context);
static void BddTargetVxWorks64_PrintError(void* context, const struct SolidSyslogErrorEvent* event);

/* Every field NULL: Core falls back to its Null buffer and sender. */
static const struct SolidSyslogConfig CORE_ONLY_CONFIG;

/* Where the collector is, as the console told us. */
static char collectorHost[COLLECTOR_HOST_SIZE];
static unsigned short collectorPort;

void BddTargetVxWorks64_Init(void)
{
    BddTargetVxWorks64_RunCore();
    BddTargetVxWorks64_BringUpNetwork();
    BddTargetVxWorks64_SendOverUdp();
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

/* Asks the console where the collector is - the development machine answers
 * "collector <host> <port>" - then sends one message there over UDP. */
static void BddTargetVxWorks64_SendOverUdp(void)
{
    char line[COLLECTOR_LINE_SIZE];
    struct SolidSyslogAddress* address;
    struct SolidSyslogResolver* resolver;
    struct SolidSyslogDatagram* datagram;
    struct SolidSyslogUdpSenderConfig senderConfig;
    struct SolidSyslogSender* sender;
    struct SolidSyslogBuffer* buffer;
    struct SolidSyslogConfig config;
    struct SolidSyslog* logger;
    struct SolidSyslogMessage message;

    SolidSyslog_SetErrorHandler(BddTargetVxWorks64_PrintError, NULL);
    printf(BDD_TARGET_TAG "collector?\n");
    fflush(stdout);
    if ((fgets(line, (int) sizeof(line), stdin) == NULL) || !BddTargetVxWorks64_ParseCollector(line))
    {
        printf(BDD_TARGET_TAG "no collector given\n");
    }
    else
    {
        address = SolidSyslogVxWorks64Address_Create();
        resolver = SolidSyslogVxWorks64Resolver_Create();
        datagram = SolidSyslogVxWorks64Datagram_Create();

        memset(&senderConfig, 0, sizeof(senderConfig));
        senderConfig.Resolver = resolver;
        senderConfig.Datagram = datagram;
        senderConfig.Address = address;
        senderConfig.Endpoint = BddTargetVxWorks64_Endpoint;
        sender = SolidSyslogUdpSender_Create(&senderConfig);
        buffer = SolidSyslogPassthroughBuffer_Create(sender);

        memset(&config, 0, sizeof(config));
        config.Buffer = buffer;
        config.Sender = sender;
        config.Store = SolidSyslogNullStore_Get();
        logger = SolidSyslog_Create(&config);

        message.Facility = SOLIDSYSLOG_FACILITY_USER;
        message.Severity = SOLIDSYSLOG_SEVERITY_INFORMATIONAL;
        message.MessageId = "UDP";
        message.Msg = "VxWorks 6.4 BDD target over UDP";
        SolidSyslog_Log(logger, &message);
        printf(BDD_TARGET_TAG "logged over UDP to %s:%u\n", collectorHost, (unsigned) collectorPort);

        SolidSyslog_Destroy(logger);
        SolidSyslogPassthroughBuffer_Destroy(buffer);
        SolidSyslogUdpSender_Destroy(sender);
        SolidSyslogVxWorks64Datagram_Destroy(datagram);
        SolidSyslogVxWorks64Resolver_Destroy(resolver);
        SolidSyslogVxWorks64Address_Destroy(address);
    }
}

/* Takes "collector <host> <port>" into collectorHost and collectorPort; returns
 * zero, and leaves them unset, for anything else. */
static int BddTargetVxWorks64_ParseCollector(const char* line)
{
    const char* host;
    size_t hostLength;
    char* end;
    long port;
    int parsed;

    parsed = 0;
    if (strncmp(line, COLLECTOR_COMMAND, strlen(COLLECTOR_COMMAND)) == 0)
    {
        host = line + strlen(COLLECTOR_COMMAND);
        hostLength = strcspn(host, " ");
        if ((hostLength > 0) && (hostLength < sizeof(collectorHost)) && (host[hostLength] == ' '))
        {
            port = strtol(host + hostLength + 1, &end, 10);
            if ((port > 0) && (port <= MAX_PORT) && (end != host + hostLength + 1))
            {
                memcpy(collectorHost, host, hostLength);
                collectorHost[hostLength] = '\0';
                collectorPort = (unsigned short) port;
                parsed = 1;
            }
        }
    }
    return parsed;
}

static void BddTargetVxWorks64_Endpoint(struct SolidSyslogEndpoint* endpoint, void* context)
{
    (void) context;
    SolidSyslogEndpointHost_String(endpoint->Host, collectorHost, sizeof(collectorHost));
    endpoint->Port = collectorPort;
}

/* Prints what the library reports, so a failed send shows on the console. */
static void BddTargetVxWorks64_PrintError(void* context, const struct SolidSyslogErrorEvent* event)
{
    (void) context;
    printf(
        BDD_TARGET_TAG "error from %s: category %u, detail %ld\n",
        event->Source->Name,
        (unsigned) event->Category,
        (long) event->Detail
    );
}
