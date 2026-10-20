/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogErrorName.h"

#include <stddef.h>
#include <stdint.h>

const char* SolidSyslogErrorName_Lookup(const char* const * names, int32_t count, int32_t detail)
{
    const char* name = NULL;
    if ((detail >= 0) && (detail < count))
    {
        name = names[detail];
    }
    return name;
}
