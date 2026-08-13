/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogStdAtomicCounter.h"

#include <stdatomic.h>
#include <stdint.h>

#include "SolidSyslogAtomicCounter.h"
#include "SolidSyslogAtomicCounterDefinition.h"
#include "SolidSyslogError.h"
#include "SolidSyslogNullAtomicCounter.h"
#include "SolidSyslogStdAtomicCounterErrors.h"
#include "SolidSyslogStdAtomicCounterPrivate.h"

const struct SolidSyslogErrorSource StdAtomicCounterErrorSource = {"StdAtomicCounter"};

static uint32_t StdAtomicCounter_Increment(struct SolidSyslogAtomicCounter* base);
static void StdAtomicCounter_Init(struct SolidSyslogStdAtomicCounter* self, uint32_t value);

static inline struct SolidSyslogStdAtomicCounter* StdAtomicCounter_SelfFromBase(struct SolidSyslogAtomicCounter* base);

void StdAtomicCounter_Initialise(struct SolidSyslogAtomicCounter* base)
{
    struct SolidSyslogStdAtomicCounter* self = StdAtomicCounter_SelfFromBase(base);
    self->Base.Increment = StdAtomicCounter_Increment;
    StdAtomicCounter_Init(self, 0U);
}

static inline struct SolidSyslogStdAtomicCounter* StdAtomicCounter_SelfFromBase(struct SolidSyslogAtomicCounter* base)
{
    return (struct SolidSyslogStdAtomicCounter*) base;
}

/* atomic_store rather than atomic_init: a released pool slot is handed out again,
 * so this runs a second time on an object C11 lets you initialise only once. */
static void StdAtomicCounter_Init(struct SolidSyslogStdAtomicCounter* self, uint32_t value)
{
    atomic_store_explicit(&self->Value, value, memory_order_relaxed);
}

void StdAtomicCounter_Cleanup(struct SolidSyslogAtomicCounter* base)
{
    /* Overwrite the abstract base with the shared NullAtomicCounter vtable so
     * use-after-destroy is a safe no-op rather than a NULL-fn-pointer crash. */
    *base = *SolidSyslogNullAtomicCounter_Get();
}

static uint32_t StdAtomicCounter_Increment(struct SolidSyslogAtomicCounter* base)
{
    struct SolidSyslogStdAtomicCounter* self = StdAtomicCounter_SelfFromBase(base);
    uint32_t current = atomic_load_explicit(&self->Value, memory_order_relaxed);
    uint32_t next = 0U;
    do
    {
        next = (current >= SOLIDSYSLOG_SEQUENCE_ID_MAX) ? 1U : (current + 1U);
    } while (!atomic_compare_exchange_strong_explicit(
        &self->Value,
        &current,
        next,
        memory_order_relaxed,
        memory_order_relaxed
    ));
    return next;
}
