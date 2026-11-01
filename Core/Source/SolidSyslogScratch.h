/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/* The scratch a message is formatted in, and who owns the frame that holds it.
 * Library-private: an integrator picks an implementation through
 * SolidSyslogConfig and never sees this vtable. */
#ifndef SOLIDSYSLOGSCRATCH_H
#define SOLIDSYSLOGSCRATCH_H

#include "SolidSyslogExternC.h"

#include <stddef.h>

#include "SolidSyslogFormatter.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    /* Runs with storage that holds a formatter sized for messageBytes. The
     * storage is valid only until the user returns. */
    typedef void (*SolidSyslogScratchUser)(SolidSyslogFormatterStorage* storage, size_t messageBytes, void* context);

    struct SolidSyslogScratch
    {
        /* Calls use(storage, messageBytes, context) exactly once, synchronously,
         * before returning. */
        void (*WithScratch)(struct SolidSyslogScratch* self, SolidSyslogScratchUser use, void* context);
    };

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGSCRATCH_H */
