/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogSharedScratch.h"

#include <stddef.h>

#include "SolidSyslogError.h"
#include "SolidSyslogMutex.h"
#include "SolidSyslogScratch.h"
#include "SolidSyslogSharedScratchErrors.h"
#include "SolidSyslogSharedScratchPrivate.h"
#include "SolidSyslogStackScratch.h"
#include "SolidSyslogTunables.h"

const struct SolidSyslogErrorSource SolidSyslogSharedScratchErrorSource = {
    "SharedScratch",
    SolidSyslogSharedScratchErrors_Name
};

static void SharedScratch_WithScratch(struct SolidSyslogScratch* base, SolidSyslogScratchUser use, void* context);

static inline struct SolidSyslogSharedScratch* SharedScratch_SelfFromBase(struct SolidSyslogScratch* base);

void SolidSyslogSharedScratch_Initialise(struct SolidSyslogScratch* base, struct SolidSyslogMutex* mutex)
{
    struct SolidSyslogSharedScratch* self = SharedScratch_SelfFromBase(base);
    self->Base.WithScratch = SharedScratch_WithScratch;
    self->Mutex = mutex;
}

void SolidSyslogSharedScratch_Cleanup(struct SolidSyslogScratch* base)
{
    /* A stale handle formats on the stack after Destroy, which is safe, rather
     * than through a NULL function pointer or a mutex that may be gone. */
    *base = *SolidSyslogStackScratch_Get();
}

static void SharedScratch_WithScratch(struct SolidSyslogScratch* base, SolidSyslogScratchUser use, void* context)
{
    struct SolidSyslogSharedScratch* self = SharedScratch_SelfFromBase(base);
    SolidSyslogMutex_Lock(self->Mutex);
    use(self->Storage, SOLIDSYSLOG_MAX_MESSAGE_SIZE, context);
    SolidSyslogMutex_Unlock(self->Mutex);
}

static inline struct SolidSyslogSharedScratch* SharedScratch_SelfFromBase(struct SolidSyslogScratch* base)
{
    return (struct SolidSyslogSharedScratch*) base;
}
