/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogNullMutex.h"

#include "SolidSyslogMutexDefinition.h"

static void NullMutex_Lock(struct SolidSyslogMutex* base);
static void NullMutex_Unlock(struct SolidSyslogMutex* base);

struct SolidSyslogMutex* SolidSyslogNullMutex_Get(void)
{
    static struct SolidSyslogMutex instance = {
        .Lock = NullMutex_Lock,
        .Unlock = NullMutex_Unlock,
    };
    return &instance;
}

static void NullMutex_Lock(struct SolidSyslogMutex* base)
{
    (void) base;
}

static void NullMutex_Unlock(struct SolidSyslogMutex* base)
{
    (void) base;
}
