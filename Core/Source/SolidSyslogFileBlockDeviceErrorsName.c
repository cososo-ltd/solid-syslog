/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogFileBlockDeviceErrors.h"

#include <stdint.h>

#include "SolidSyslogErrorName.h"
#include "SolidSyslogMacros.h"

static const char* const FileBlockDeviceErrorsName_Names[] = {
    [SOLIDSYSLOG_FILE_BLOCK_DEVICE_ERROR_POOL_EXHAUSTED] = "POOL_EXHAUSTED",
    [SOLIDSYSLOG_FILE_BLOCK_DEVICE_ERROR_UNKNOWN_DESTROY] = "UNKNOWN_DESTROY",
};

SOLIDSYSLOG_STATIC_ASSERT(
    (sizeof(FileBlockDeviceErrorsName_Names) / sizeof(FileBlockDeviceErrorsName_Names[0])) ==
        SOLIDSYSLOG_FILE_BLOCK_DEVICE_ERROR_MAX,
    every_file_block_device_error_code_has_a_name
);

const char* SolidSyslogFileBlockDeviceErrors_Name(int32_t detail)
{
    return SolidSyslogErrorName_Lookup(
        FileBlockDeviceErrorsName_Names,
        (int32_t) SOLIDSYSLOG_FILE_BLOCK_DEVICE_ERROR_MAX,
        detail
    );
}
