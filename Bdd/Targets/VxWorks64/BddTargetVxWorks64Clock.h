/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */
#ifndef BDDTARGETVXWORKS64CLOCK_H
#define BDDTARGETVXWORKS64CLOCK_H

#include <stdbool.h>

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    /* Sets the real-time clock to this many seconds since the epoch, UTC. */
    bool BddTargetVxWorks64Clock_Set(unsigned long seconds);

SOLIDSYSLOG_EXTERN_C_END

#endif /* BDDTARGETVXWORKS64CLOCK_H */
