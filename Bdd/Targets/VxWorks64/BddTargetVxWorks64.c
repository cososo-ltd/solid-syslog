/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/* The VIP compiles this file with its own flags, which select C89. */

#include <stdio.h>

#include "SolidSyslog.h"
#include "SolidSyslogConfig.h"
#include "SolidSyslogPrival.h"

void BddTargetVxWorks64_Init(void);

/* Every field NULL: Core falls back to its Null buffer and sender. */
static const struct SolidSyslogConfig CONFIG;

void BddTargetVxWorks64_Init(void)
{
    struct SolidSyslog* logger;
    struct SolidSyslogMessage message;

    message.Facility = SOLIDSYSLOG_FACILITY_USER;
    message.Severity = SOLIDSYSLOG_SEVERITY_INFORMATIONAL;
    message.MessageId = "BOOT";
    message.Msg = "VxWorks 6.4 BDD target";

    logger = SolidSyslog_Create(&CONFIG);
    SolidSyslog_Log(logger, &message);
    SolidSyslog_Destroy(logger);

    printf("SolidSyslog VxWorks 6.4 BDD target: Core ran\n");
}
