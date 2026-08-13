/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogWindowsAtomicCounter.h"

#include <stdint.h>
#include <windows.h>

#include "SolidSyslogAtomicCounter.h"
#include "SolidSyslogAtomicCounterDefinition.h"
#include "SolidSyslogError.h"
#include "SolidSyslogNullAtomicCounter.h"
#include "SolidSyslogWindowsAtomicCounterErrors.h"
#include "SolidSyslogWindowsAtomicCounterPrivate.h"

const struct SolidSyslogErrorSource WindowsAtomicCounterErrorSource = {"WindowsAtomicCounter"};

static uint32_t WindowsAtomicCounter_Increment(struct SolidSyslogAtomicCounter* base);
static void WindowsAtomicCounter_Init(struct SolidSyslogWindowsAtomicCounter* self, uint32_t value);

static inline struct SolidSyslogWindowsAtomicCounter* WindowsAtomicCounter_SelfFromBase(
    struct SolidSyslogAtomicCounter* base
);

void WindowsAtomicCounter_Initialise(struct SolidSyslogAtomicCounter* base)
{
    struct SolidSyslogWindowsAtomicCounter* self = WindowsAtomicCounter_SelfFromBase(base);
    self->Base.Increment = WindowsAtomicCounter_Increment;
    WindowsAtomicCounter_Init(self, 0U);
}

static inline struct SolidSyslogWindowsAtomicCounter* WindowsAtomicCounter_SelfFromBase(
    struct SolidSyslogAtomicCounter* base
)
{
    return (struct SolidSyslogWindowsAtomicCounter*) base;
}

static void WindowsAtomicCounter_Init(struct SolidSyslogWindowsAtomicCounter* self, uint32_t value)
{
    self->Value = (LONG) value;
}

void WindowsAtomicCounter_Cleanup(struct SolidSyslogAtomicCounter* base)
{
    /* Overwrite the abstract base with the shared NullAtomicCounter vtable so
     * use-after-destroy is a safe no-op rather than a NULL-fn-pointer crash. */
    *base = *SolidSyslogNullAtomicCounter_Get();
}

static uint32_t WindowsAtomicCounter_Increment(struct SolidSyslogAtomicCounter* base)
{
    struct SolidSyslogWindowsAtomicCounter* self = WindowsAtomicCounter_SelfFromBase(base);
    uint32_t current = (uint32_t) InterlockedCompareExchange(&self->Value, 0, 0);
    uint32_t next = 0U;
    do
    {
        next = (current >= SOLIDSYSLOG_SEQUENCE_ID_MAX) ? 1U : (current + 1U);
        uint32_t previous = (uint32_t) InterlockedCompareExchange(&self->Value, (LONG) next, (LONG) current);
        if (previous == current)
        {
            break;
        }
        current = previous;
    } while (1);
    return next;
}
