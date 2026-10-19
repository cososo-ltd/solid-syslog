/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  The no-op Store Null object (no store-and-forward): Write returns false and IsTransient
 *  returns true, so Service sends a buffered record directly instead of storing it.
 *  ReadNextUnsent and HasUnsent report empty. */
#ifndef SOLIDSYSLOGNULLSTORE_H
#define SOLIDSYSLOGNULLSTORE_H

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    /** Never retains a record. Write returns false (not held by this store) and IsTransient
     *  true, so Service sends each buffered record directly: the constrained-system
     *  configuration with no store-and-forward. A record the sender refuses is not lost at
     *  once. Service holds it and retries it on every pass, and the buffer is the only queue
     *  behind it, applying its own discard policy as it fills; see SolidSyslog_Service.
     *  ReadNextUnsent and HasUnsent report empty. */
    struct SolidSyslogStore* SolidSyslogNullStore_Get(void);

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGNULLSTORE_H */
