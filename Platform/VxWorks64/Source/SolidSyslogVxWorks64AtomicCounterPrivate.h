/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#ifndef SOLIDSYSLOGVXWORKS64ATOMICCOUNTERPRIVATE_H
#define SOLIDSYSLOGVXWORKS64ATOMICCOUNTERPRIVATE_H

#include <stdint.h>

#include "SolidSyslogAtomicCounterDefinition.h"

struct SolidSyslogVxWorks64AtomicCounter
{
    struct SolidSyslogAtomicCounter Base;
    uint32_t Value;
};

void SolidSyslogVxWorks64AtomicCounter_Initialise(struct SolidSyslogAtomicCounter* base);

#endif /* SOLIDSYSLOGVXWORKS64ATOMICCOUNTERPRIVATE_H */
