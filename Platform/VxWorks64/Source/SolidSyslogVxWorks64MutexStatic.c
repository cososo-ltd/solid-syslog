/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogVxWorks64Mutex.h"

#include "semLib.h"

#include "SolidSyslogNullMutex.h"

struct SolidSyslogMutex* SolidSyslogVxWorks64Mutex_Create(void)
{
    (void) semMCreate(SEM_Q_PRIORITY | SEM_INVERSION_SAFE | SEM_DELETE_SAFE);
    return SolidSyslogNullMutex_Get();
}

void SolidSyslogVxWorks64Mutex_Destroy(struct SolidSyslogMutex* base)
{
    (void) base;
}
