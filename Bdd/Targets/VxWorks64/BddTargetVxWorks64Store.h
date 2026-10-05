/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#ifndef BDDTARGETVXWORKS64STORE_H
#define BDDTARGETVXWORKS64STORE_H

#include <stdbool.h>

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    /* Readies the disk the file store lives on: dosFs, formatted on first use. */
    bool BddTargetVxWorks64Store_Mount(void);

SOLIDSYSLOG_EXTERN_C_END

#endif /* BDDTARGETVXWORKS64STORE_H */
