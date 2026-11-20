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
#include "SolidSyslogNorFlashBlockDevice.h"
#include "SolidSyslogNorFlashBlockDeviceErrors.h"
#include "SolidSyslogPrival.h"

struct SolidSyslogNorFlashBlockDevice
{
    struct SolidSyslogBlockDevice Base;
    /* As Create was given it and has checked it, with a BlockSize of 0
     * replaced by what a flash block holds. */
    struct SolidSyslogNorFlashBlockDeviceConfig Config;
    /* What a flash block holds after the block's state. */
    size_t DataCapacity;
    /* Where the data of one block ends. Appending to a block needs it, and
     * finding it is a read back through the block, so the latest answer is
     * kept. */
    size_t EndBlock;
    size_t End;
    bool EndIsKnown;
};

/* What a flash block of flashBlockSize bytes holds after the block's state. */
size_t SolidSyslogNorFlashBlockDevice_DataCapacity(size_t flashBlockSize);

/* config has been checked, and its BlockSize is not 0. */
void SolidSyslogNorFlashBlockDevice_Initialise(
    struct SolidSyslogBlockDevice* base,
    const struct SolidSyslogNorFlashBlockDeviceConfig* config
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
