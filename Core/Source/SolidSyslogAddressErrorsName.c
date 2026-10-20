/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogAddressErrors.h"

#include <stdint.h>

#include "SolidSyslogErrorName.h"
#include "SolidSyslogMacros.h"

static const char* const AddressErrorsName_Names[] = {
    [SOLIDSYSLOG_ADDRESS_ERROR_POOL_EXHAUSTED] = "POOL_EXHAUSTED",
    [SOLIDSYSLOG_ADDRESS_ERROR_UNKNOWN_DESTROY] = "UNKNOWN_DESTROY",
};

SOLIDSYSLOG_STATIC_ASSERT(
    (sizeof(AddressErrorsName_Names) / sizeof(AddressErrorsName_Names[0])) == SOLIDSYSLOG_ADDRESS_ERROR_MAX,
    every_address_error_code_has_a_name
);

const char* SolidSyslogAddressErrors_Name(int32_t detail)
{
    return SolidSyslogErrorName_Lookup(AddressErrorsName_Names, (int32_t) SOLIDSYSLOG_ADDRESS_ERROR_MAX, detail);
}
