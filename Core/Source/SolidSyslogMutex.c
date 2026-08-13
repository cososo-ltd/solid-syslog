/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogMutex.h"

#include "SolidSyslogMutexDefinition.h"

void SolidSyslogMutex_Lock(struct SolidSyslogMutex* mutex)
{
    mutex->Lock(mutex);
}

void SolidSyslogMutex_Unlock(struct SolidSyslogMutex* mutex)
{
    mutex->Unlock(mutex);
}
