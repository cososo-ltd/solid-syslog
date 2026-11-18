/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#ifndef SOLIDSYSLOGNORFLASHBLOCKDEVICEPRIVATE_H
#define SOLIDSYSLOGNORFLASHBLOCKDEVICEPRIVATE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "SolidSyslogBlockDeviceDefinition.h"
#include "SolidSyslogError.h"
#include "SolidSyslogNorFlashBlockDeviceErrors.h"
#include "SolidSyslogPrival.h"

struct SolidSyslogNorFlash;

/* Where the data of one block ends. Appending to a block needs it, and finding
 * it is a read back through the block, so the latest answer is kept. */
struct KnownEnd
{
    size_t BlockIndex;
    size_t End;
    bool IsKnown;
};

struct SolidSyslogNorFlashBlockDevice
{
    struct SolidSyslogBlockDevice Base;
    struct SolidSyslogNorFlash* Flash;
    size_t FirstBlock;
    size_t BlockCount;
    size_t BlockSize;
    struct KnownEnd KnownEnd;
};

/* A config already checked: a flash, a run of blocks within it, and a block
 * size a flash block holds. */
struct SolidSyslogNorFlashBlockDeviceGeometry
{
    struct SolidSyslogNorFlash* Flash;
    size_t FirstBlock;
    size_t BlockCount;
    size_t BlockSize;
};

void SolidSyslogNorFlashBlockDevice_Initialise(
    struct SolidSyslogBlockDevice* base,
    const struct SolidSyslogNorFlashBlockDeviceGeometry* geometry
);
void SolidSyslogNorFlashBlockDevice_Cleanup(struct SolidSyslogBlockDevice* base);

static inline void NorFlashBlockDevice_Report(
    enum SolidSyslogSeverity severity,
    uint16_t category,
    enum SolidSyslogNorFlashBlockDeviceErrors code
)
{
    SolidSyslog_Error(severity, &SolidSyslogNorFlashBlockDeviceErrorSource, category, (int32_t) code);
}

#endif /* SOLIDSYSLOGNORFLASHBLOCKDEVICEPRIVATE_H */
