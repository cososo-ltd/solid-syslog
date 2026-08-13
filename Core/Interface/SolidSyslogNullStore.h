/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  The no-op Store Null object (no store-and-forward): Write returns false and IsTransient
 *  returns true, so the Service algorithm falls through to a direct send instead of
 *  buffering. ReadNextUnsent and HasUnsent report empty, IsHalted reports false. */
#ifndef SOLIDSYSLOGNULLSTORE_H
#define SOLIDSYSLOGNULLSTORE_H

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    /** Never retains a record. Write returns false (not held by this store), so the
     *  Service algorithm falls through to a direct send instead of buffering, giving the
     *  constrained-system "one attempt per message, no store-and-forward" configuration.
     *  ReadNextUnsent and HasUnsent report empty; IsHalted reports false. */
    struct SolidSyslogStore* SolidSyslogNullStore_Get(void);

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGNULLSTORE_H */
