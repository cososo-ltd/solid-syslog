/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogAddressErrors.h"

#include <stddef.h>
#include <stdint.h>

#include "SolidSyslogMacros.h"

static const char* const AddressErrorsName_Names[] = {
    "POOL_EXHAUSTED",
    "UNKNOWN_DESTROY",
};

SOLIDSYSLOG_STATIC_ASSERT(
    (sizeof(AddressErrorsName_Names) / sizeof(AddressErrorsName_Names[0])) == SOLIDSYSLOG_ADDRESS_ERROR_MAX,
    every_address_error_code_has_a_name
);

const char* SolidSyslogAddressErrors_Name(int32_t detail)
{
    const char* name = NULL;
    if ((detail >= 0) && (detail < (int32_t) SOLIDSYSLOG_ADDRESS_ERROR_MAX))
    {
        name = AddressErrorsName_Names[detail];
    }
    return name;
}
