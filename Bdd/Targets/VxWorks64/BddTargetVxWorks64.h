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
    /* The service task's work: services the logger until the console ends. */
    void BddTargetVxWorks64_RunService(void);
    /* Releases everything Init built. */
    void BddTargetVxWorks64_Teardown(void);
    /* Where the target's own reports go - its steps and library errors. NULL,
       the default, is the console. */
    void BddTargetVxWorks64_ReportTo(FILE * stream);
    /* The service loop's yield. */
    void BddTargetVxWorks64_Sleep(int milliseconds);

SOLIDSYSLOG_EXTERN_C_END

#endif /* BDDTARGETVXWORKS64_H */
