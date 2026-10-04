/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogVxWorks64Hostname.h"

#include "vxWorks.h"

#include "hostLib.h"

#include "SolidSyslogHeaderField.h"

void SolidSyslogVxWorks64_GetHostname(struct SolidSyslogHeaderField* field, void* context)
{
    char hostname[MAXHOSTNAMELEN + 1];

    (void) context;

    (void) gethostname(hostname, (int) sizeof(hostname));
    SolidSyslogHeaderField_PrintUsAscii(field, hostname, sizeof(hostname));
}
