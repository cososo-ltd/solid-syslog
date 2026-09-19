/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogCmsisRtosMutex.h"

#include <stddef.h>
#include <stdint.h>

#include "cmsis_os2.h"

#include "SolidSyslogCmsisRtosMutexPrivate.h"
#include "SolidSyslogMutexDefinition.h"
#include "SolidSyslogNullMutex.h"

const struct SolidSyslogErrorSource SolidSyslogCmsisRtosMutexErrorSource = {"CmsisRtosMutex"};

void SolidSyslogCmsisRtosMutex_Initialise(struct SolidSyslogMutex* base, void* controlBlock, uint32_t controlBlockBytes)
{
    osMutexAttr_t attributes = {NULL, 0, controlBlock, controlBlockBytes};
    (void) osMutexNew(&attributes);
    *base = *SolidSyslogNullMutex_Get();
}

void SolidSyslogCmsisRtosMutex_Cleanup(struct SolidSyslogMutex* base)
{
    *base = *SolidSyslogNullMutex_Get();
}
