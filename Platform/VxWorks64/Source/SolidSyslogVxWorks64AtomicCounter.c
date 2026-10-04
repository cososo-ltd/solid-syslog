/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogVxWorks64AtomicCounter.h"

#include <stdint.h>

#include "SolidSyslogAtomicCounter.h"
#include "SolidSyslogAtomicCounterDefinition.h"
#include "SolidSyslogVxWorks64AtomicCounterPrivate.h"

static uint32_t VxWorks64AtomicCounter_Increment(struct SolidSyslogAtomicCounter* base);
static void VxWorks64AtomicCounter_Init(struct SolidSyslogVxWorks64AtomicCounter* self, uint32_t value);

static inline struct SolidSyslogVxWorks64AtomicCounter* VxWorks64AtomicCounter_SelfFromBase(
    struct SolidSyslogAtomicCounter* base
);

void SolidSyslogVxWorks64AtomicCounter_Initialise(struct SolidSyslogAtomicCounter* base)
{
    struct SolidSyslogVxWorks64AtomicCounter* self = VxWorks64AtomicCounter_SelfFromBase(base);
    self->Base.Increment = VxWorks64AtomicCounter_Increment;
    VxWorks64AtomicCounter_Init(self, 0U);
}

static inline struct SolidSyslogVxWorks64AtomicCounter* VxWorks64AtomicCounter_SelfFromBase(
    struct SolidSyslogAtomicCounter* base
)
{
    return (struct SolidSyslogVxWorks64AtomicCounter*) base;
}

static void VxWorks64AtomicCounter_Init(struct SolidSyslogVxWorks64AtomicCounter* self, uint32_t value)
{
    self->Value = value;
}

static uint32_t VxWorks64AtomicCounter_Increment(struct SolidSyslogAtomicCounter* base)
{
    struct SolidSyslogVxWorks64AtomicCounter* self = VxWorks64AtomicCounter_SelfFromBase(base);
    self->Value = (self->Value >= SOLIDSYSLOG_SEQUENCE_ID_MAX) ? 1U : (self->Value + 1U);
    return self->Value;
}
