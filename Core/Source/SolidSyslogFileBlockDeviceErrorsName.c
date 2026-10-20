/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogFileBlockDeviceErrors.h"

#include <stddef.h>
#include <stdint.h>

#include "SolidSyslogMacros.h"

static const char* const FileBlockDeviceErrorsName_Names[] = {
    "POOL_EXHAUSTED",
    "UNKNOWN_DESTROY",
};

SOLIDSYSLOG_STATIC_ASSERT(
    (sizeof(FileBlockDeviceErrorsName_Names) / sizeof(FileBlockDeviceErrorsName_Names[0])) ==
        SOLIDSYSLOG_FILE_BLOCK_DEVICE_ERROR_MAX,
    every_file_block_device_error_code_has_a_name
);

const char* SolidSyslogFileBlockDeviceErrors_Name(int32_t detail)
{
    const char* name = NULL;
    if ((detail >= 0) && (detail < (int32_t) SOLIDSYSLOG_FILE_BLOCK_DEVICE_ERROR_MAX))
    {
        name = FileBlockDeviceErrorsName_Names[detail];
    }
    return name;
}
