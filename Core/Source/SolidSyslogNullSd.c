/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogNullSd.h"

#include "SolidSyslogStructuredDataDefinition.h"

struct SolidSyslogSdElement;

static void NullSd_Format(struct SolidSyslogStructuredData* base, struct SolidSyslogSdElement* element);

struct SolidSyslogStructuredData* SolidSyslogNullSd_Get(void)
{
    static struct SolidSyslogStructuredData instance = {.Format = NullSd_Format};
    return &instance;
}

static void NullSd_Format(struct SolidSyslogStructuredData* base, struct SolidSyslogSdElement* element)
{
    (void) base;
    (void) element;
}
