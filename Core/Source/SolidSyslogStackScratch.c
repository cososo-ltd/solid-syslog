/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogStackScratch.h"

#include <stddef.h>

#include "SolidSyslogFormatter.h"
#include "SolidSyslogScratch.h"
#include "SolidSyslogTunables.h"

static void StackScratch_WithScratch(struct SolidSyslogScratch* self, SolidSyslogScratchUser use, void* context);

struct SolidSyslogScratch* SolidSyslogStackScratch_Get(void)
{
    static struct SolidSyslogScratch instance = {StackScratch_WithScratch};
    return &instance;
}

/* The array lives in this frame and no other, so a logger wired with a
 * different scratch never has it on its stack: this function is reached only
 * through the vtable, which keeps it out of the caller's frame. */
static void StackScratch_WithScratch(struct SolidSyslogScratch* self, SolidSyslogScratchUser use, void* context)
{
    SolidSyslogFormatterStorage storage[SOLIDSYSLOG_FORMATTER_STORAGE_SIZE(SOLIDSYSLOG_MAX_MESSAGE_SIZE)];
    (void) self;
    use(storage, SOLIDSYSLOG_MAX_MESSAGE_SIZE, context);
}
