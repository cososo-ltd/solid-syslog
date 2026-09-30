/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogVxWorks64Mutex.h"

#include <stdbool.h>

#include "semLib.h"

#include "SolidSyslogError.h"
#include "SolidSyslogMutexDefinition.h"
#include "SolidSyslogVxWorks64MutexPrivate.h"

const struct SolidSyslogErrorSource SolidSyslogVxWorks64MutexErrorSource = {"VxWorks64Mutex"};

static inline struct SolidSyslogVxWorks64Mutex* VxWorks64Mutex_SelfFromBase(struct SolidSyslogMutex* base);

bool SolidSyslogVxWorks64Mutex_Initialise(struct SolidSyslogMutex* base)
{
    struct SolidSyslogVxWorks64Mutex* self = VxWorks64Mutex_SelfFromBase(base);
    self->Id = semMCreate(SEM_Q_PRIORITY | SEM_INVERSION_SAFE | SEM_DELETE_SAFE);
    return true;
}

static inline struct SolidSyslogVxWorks64Mutex* VxWorks64Mutex_SelfFromBase(struct SolidSyslogMutex* base)
{
    return (struct SolidSyslogVxWorks64Mutex*) base;
}

void SolidSyslogVxWorks64Mutex_Cleanup(struct SolidSyslogMutex* base)
{
    (void) base;
}
