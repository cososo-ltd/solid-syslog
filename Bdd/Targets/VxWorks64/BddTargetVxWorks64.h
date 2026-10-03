/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#ifndef BDDTARGETVXWORKS64_H
#define BDDTARGETVXWORKS64_H

#include <stdio.h>

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    /* The component's init routine: builds the pipeline and spawns the tasks. */
    void BddTargetVxWorks64_Init(void);
    /* The interactive task's work, on the given console input. */
    void BddTargetVxWorks64_RunConsole(FILE * input);
    /* Releases everything Init built. */
    void BddTargetVxWorks64_Teardown(void);

SOLIDSYSLOG_EXTERN_C_END

#endif /* BDDTARGETVXWORKS64_H */
