/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogVxWorks64AtomicCounter.h"

#include "SolidSyslogVxWorks64AtomicCounterPrivate.h"

static struct SolidSyslogVxWorks64AtomicCounter VxWorks64AtomicCounter_Instance;

struct SolidSyslogAtomicCounter* SolidSyslogVxWorks64AtomicCounter_Create(void)
{
    SolidSyslogVxWorks64AtomicCounter_Initialise(&VxWorks64AtomicCounter_Instance.Base);
    return &VxWorks64AtomicCounter_Instance.Base;
}

void SolidSyslogVxWorks64AtomicCounter_Destroy(struct SolidSyslogAtomicCounter* base)
{
    (void) base;
}
