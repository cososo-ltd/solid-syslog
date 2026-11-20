/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogNorFlashBlockDeviceErrors.h"

#include <stdint.h>

#include "SolidSyslogErrorName.h"

/* Indexed by the enum, so no name can shift; a code without one is NULL. */
const char* SolidSyslogNorFlashBlockDeviceErrors_Name(int32_t detail)
{
    static const char* const names[SOLIDSYSLOG_NOR_FLASH_BLOCK_DEVICE_ERROR_MAX] = {
        [SOLIDSYSLOG_NOR_FLASH_BLOCK_DEVICE_ERROR_POOL_EXHAUSTED] = "POOL_EXHAUSTED",
        [SOLIDSYSLOG_NOR_FLASH_BLOCK_DEVICE_ERROR_UNKNOWN_DESTROY] = "UNKNOWN_DESTROY",
        [SOLIDSYSLOG_NOR_FLASH_BLOCK_DEVICE_ERROR_NULL_CONFIG] = "NULL_CONFIG",
        [SOLIDSYSLOG_NOR_FLASH_BLOCK_DEVICE_ERROR_NULL_FLASH] = "NULL_FLASH",
        [SOLIDSYSLOG_NOR_FLASH_BLOCK_DEVICE_ERROR_BLOCKS_OUTSIDE_FLASH] = "BLOCKS_OUTSIDE_FLASH",
        [SOLIDSYSLOG_NOR_FLASH_BLOCK_DEVICE_ERROR_BLOCK_SIZE_UNAVAILABLE] = "BLOCK_SIZE_UNAVAILABLE",
        [SOLIDSYSLOG_NOR_FLASH_BLOCK_DEVICE_ERROR_VERIFY_FAILED] = "VERIFY_FAILED",
    };

    return SolidSyslogErrorName_Lookup(names, (int32_t) SOLIDSYSLOG_NOR_FLASH_BLOCK_DEVICE_ERROR_MAX, detail);
}
