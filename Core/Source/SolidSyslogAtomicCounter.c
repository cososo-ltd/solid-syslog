/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogAtomicCounter.h"

#include "SolidSyslogAtomicCounterDefinition.h"

uint32_t SolidSyslogAtomicCounter_Increment(struct SolidSyslogAtomicCounter* base)
{
    return base->Increment(base);
}
