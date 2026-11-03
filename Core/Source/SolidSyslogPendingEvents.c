/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogPendingEvents.h"

#include <stdbool.h>
#include <stddef.h>

void SolidSyslogPendingEvents_Reset(void)
{
}

bool SolidSyslogPendingEvents_Take(size_t index, struct SolidSyslogErrorEvent* event)
{
    (void) index;
    (void) event;
    return false;
}
