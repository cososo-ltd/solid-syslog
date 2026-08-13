/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogPosixProcessId.h"

#include <unistd.h>
#include <stdint.h>

#include "SolidSyslogHeaderField.h"

struct SolidSyslogHeaderField;

void SolidSyslogPosix_GetProcessId(struct SolidSyslogHeaderField* field, void* context)
{
    (void) context;
    SolidSyslogHeaderField_Uint32(field, (uint32_t) getpid());
}
