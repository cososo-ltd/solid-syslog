/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogStructuredDataDefinition.h"
#include "SolidSyslogStructuredData.h"

struct SolidSyslogSdElement;

void SolidSyslogStructuredData_Format(struct SolidSyslogStructuredData* sd, struct SolidSyslogSdElement* element)
{
    sd->Format(sd, element);
}
