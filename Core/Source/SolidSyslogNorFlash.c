/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include <stdbool.h>
#include <stddef.h>

#include "SolidSyslogNorFlash.h"
#include "SolidSyslogNorFlashDefinition.h"

bool SolidSyslogNorFlash_Read(
    struct SolidSyslogNorFlash* flash,
    size_t blockIndex,
    size_t offset,
    void* buf,
    size_t count
)
{
    return flash->Read(flash, blockIndex, offset, buf, count);
}

bool SolidSyslogNorFlash_Program(
    struct SolidSyslogNorFlash* flash,
    size_t blockIndex,
    size_t offset,
    const void* buf,
    size_t count
)
{
    return flash->Program(flash, blockIndex, offset, buf, count);
}

bool SolidSyslogNorFlash_Erase(struct SolidSyslogNorFlash* flash, size_t blockIndex)
{
    return flash->Erase(flash, blockIndex);
}

size_t SolidSyslogNorFlash_GetBlockSize(struct SolidSyslogNorFlash* flash)
{
    return flash->GetBlockSize(flash);
}

size_t SolidSyslogNorFlash_GetBlockCount(struct SolidSyslogNorFlash* flash)
{
    return flash->GetBlockCount(flash);
}
