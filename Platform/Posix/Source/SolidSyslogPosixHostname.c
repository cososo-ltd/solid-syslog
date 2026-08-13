/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogPosixHostname.h"

#include <unistd.h>

#include "SolidSyslogHeaderField.h"

struct SolidSyslogHeaderField;

enum
{
    MAX_HOSTNAME_SIZE = 256U
};

void SolidSyslogPosix_GetHostname(struct SolidSyslogHeaderField* field, void* context)
{
    char hostname[MAX_HOSTNAME_SIZE];

    (void) context;

    if (gethostname(hostname, sizeof(hostname)) == 0)
    {
        hostname[sizeof(hostname) - 1U] = '\0';
        SolidSyslogHeaderField_PrintUsAscii(field, hostname, sizeof(hostname));
    }
}
