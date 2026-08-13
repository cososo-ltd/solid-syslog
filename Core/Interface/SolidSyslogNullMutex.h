/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  The no-op Mutex Null object: Lock and Unlock are no-ops, giving unsynchronised access
 *  for single-task targets that need no mutual exclusion. */
#ifndef SOLIDSYSLOGNULLMUTEX_H
#define SOLIDSYSLOGNULLMUTEX_H

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    /** Lock and Unlock are no-ops, giving unsynchronised access for single-task
     *  targets that need no mutual exclusion. */
    struct SolidSyslogMutex* SolidSyslogNullMutex_Get(void);

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGNULLMUTEX_H */
