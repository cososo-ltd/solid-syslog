/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogDiscardPolicy.h"

enum SolidSyslogDiscardPolicy SolidSyslogDiscardPolicy_Default(void* context)
{
    (void) context;
    return SOLIDSYSLOG_DISCARD_POLICY_OLDEST;
}
