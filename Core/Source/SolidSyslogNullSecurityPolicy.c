/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogNullSecurityPolicy.h"

#include <stdbool.h>
#include <stdint.h>

#include "SolidSyslogSecurityPolicyDefinition.h"

static bool NullSecurityPolicy_NullSealRecord(
    struct SolidSyslogSecurityPolicy* self,
    const struct SolidSyslogSecurityRecord* record
)
{
    (void) self;
    (void) record;
    return true;
}

static bool NullSecurityPolicy_NullOpenRecord(
    struct SolidSyslogSecurityPolicy* self,
    const struct SolidSyslogSecurityRecord* record
)
{
    (void) self;
    (void) record;
    return true;
}

struct SolidSyslogSecurityPolicy* SolidSyslogNullSecurityPolicy_Get(void)
{
    static struct SolidSyslogSecurityPolicy instance = {
        0,
        NullSecurityPolicy_NullSealRecord,
        NullSecurityPolicy_NullOpenRecord,
    };
    return &instance;
}
