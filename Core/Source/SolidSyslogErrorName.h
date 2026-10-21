/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#ifndef SOLIDSYSLOGERRORNAME_H
#define SOLIDSYSLOGERRORNAME_H

#include <stdint.h>

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    /* The lookup behind every name function: names[detail], or NULL outside
 * [0, count). */
    const char* SolidSyslogErrorName_Lookup(const char* const * names, int32_t count, int32_t detail);

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGERRORNAME_H */
