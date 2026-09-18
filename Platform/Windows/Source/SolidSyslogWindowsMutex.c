/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogWindowsMutex.h"

#include <stddef.h>
#include <windows.h>

#include "SolidSyslogError.h"
#include "SolidSyslogMutexDefinition.h"
#include "SolidSyslogNullMutex.h"
#include "SolidSyslogWindowsMutexErrors.h"
#include "SolidSyslogWindowsMutexPrivate.h"

const struct SolidSyslogErrorSource SolidSyslogWindowsMutexErrorSource = {"WindowsMutex"};

static void WindowsMutex_Lock(struct SolidSyslogMutex* base);
static void WindowsMutex_Unlock(struct SolidSyslogMutex* base);

static inline struct SolidSyslogWindowsMutex* WindowsMutex_SelfFromBase(struct SolidSyslogMutex* base);

void SolidSyslogWindowsMutex_Initialise(struct SolidSyslogMutex* base)
{
    struct SolidSyslogWindowsMutex* self = WindowsMutex_SelfFromBase(base);
    self->Base.Lock = WindowsMutex_Lock;
    self->Base.Unlock = WindowsMutex_Unlock;
    InitializeCriticalSection(&self->Section);
}

void SolidSyslogWindowsMutex_Cleanup(struct SolidSyslogMutex* base)
{
    struct SolidSyslogWindowsMutex* self = WindowsMutex_SelfFromBase(base);
    DeleteCriticalSection(&self->Section);
    /* Overwrite the abstract base with the shared NullMutex vtable so
     * use-after-destroy is a safe no-op rather than a NULL-fn-pointer crash. */
    *base = *SolidSyslogNullMutex_Get();
}

static inline struct SolidSyslogWindowsMutex* WindowsMutex_SelfFromBase(struct SolidSyslogMutex* base)
{
    return (struct SolidSyslogWindowsMutex*) base;
}

static void WindowsMutex_Lock(struct SolidSyslogMutex* base)
{
    struct SolidSyslogWindowsMutex* self = WindowsMutex_SelfFromBase(base);
    EnterCriticalSection(&self->Section);
}

static void WindowsMutex_Unlock(struct SolidSyslogMutex* base)
{
    struct SolidSyslogWindowsMutex* self = WindowsMutex_SelfFromBase(base);
    LeaveCriticalSection(&self->Section);
}
