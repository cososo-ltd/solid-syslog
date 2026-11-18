/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogNullNorFlash.h"

#include <stdbool.h>
#include <stddef.h>

#include "SolidSyslogNorFlashDefinition.h"

// NOLINTBEGIN(bugprone-easily-swappable-parameters) -- vtable signature mirrored from NorFlash contract
static bool NullNorFlash_Read(struct SolidSyslogNorFlash* base, size_t blockIndex, size_t offset, void* buf, size_t count);
static bool NullNorFlash_Program(
    struct SolidSyslogNorFlash* base,
    size_t blockIndex,
    size_t offset,
    const void* buf,
    size_t count
);
// NOLINTEND(bugprone-easily-swappable-parameters)
static bool NullNorFlash_Erase(struct SolidSyslogNorFlash* base, size_t blockIndex);
static size_t NullNorFlash_GetBlockSize(struct SolidSyslogNorFlash* base);
static size_t NullNorFlash_GetBlockCount(struct SolidSyslogNorFlash* base);

struct SolidSyslogNorFlash* SolidSyslogNullNorFlash_Get(void)
{
    static struct SolidSyslogNorFlash instance = {
        .Read = NullNorFlash_Read,
        .Program = NullNorFlash_Program,
        .Erase = NullNorFlash_Erase,
        .GetBlockSize = NullNorFlash_GetBlockSize,
        .GetBlockCount = NullNorFlash_GetBlockCount,
    };
    return &instance;
}

// NOLINTBEGIN(bugprone-easily-swappable-parameters) -- vtable signature mirrored from NorFlash contract
static bool NullNorFlash_Read(struct SolidSyslogNorFlash* base, size_t blockIndex, size_t offset, void* buf, size_t count)
{
    (void) base;
    (void) blockIndex;
    (void) offset;
    (void) buf;
    (void) count;
    return false;
}

static bool NullNorFlash_Program(
    struct SolidSyslogNorFlash* base,
    size_t blockIndex,
    size_t offset,
    const void* buf,
    size_t count
)
{
    (void) base;
    (void) blockIndex;
    (void) offset;
    (void) buf;
    (void) count;
    return false;
}

// NOLINTEND(bugprone-easily-swappable-parameters)

static bool NullNorFlash_Erase(struct SolidSyslogNorFlash* base, size_t blockIndex)
{
    (void) base;
    (void) blockIndex;
    return false;
}

static size_t NullNorFlash_GetBlockSize(struct SolidSyslogNorFlash* base)
{
    (void) base;
    return 0U;
}

static size_t NullNorFlash_GetBlockCount(struct SolidSyslogNorFlash* base)
{
    (void) base;
    return 0U;
}
