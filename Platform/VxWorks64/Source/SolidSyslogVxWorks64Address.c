/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogVxWorks64Address.h"

#include "SolidSyslogError.h"
#include "SolidSyslogVxWorks64AddressErrors.h"
#include "SolidSyslogVxWorks64AddressPrivate.h"

const struct SolidSyslogErrorSource SolidSyslogVxWorks64AddressErrorSource = {"VxWorks64Address"};

void SolidSyslogVxWorks64Address_Initialise(struct SolidSyslogAddress* base)
{
    (void) base;
}

void SolidSyslogVxWorks64Address_Cleanup(struct SolidSyslogAddress* base)
{
    (void) base;
}
