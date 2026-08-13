/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogError.h"
#include "SolidSyslogLwipRawAddressErrors.h"
#include "SolidSyslogLwipRawAddressPrivate.h"

#include <string.h>

const struct SolidSyslogErrorSource LwipRawAddressErrorSource = {"LwipRawAddress"};

struct SolidSyslogAddress;

void LwipRawAddress_Initialise(struct SolidSyslogAddress* base)
{
    struct SolidSyslogLwipRawAddress* self = SolidSyslogLwipRawAddress_As(base);
    (void) memset(self, 0, sizeof *self);
}

void LwipRawAddress_Cleanup(struct SolidSyslogAddress* base)
{
    (void) base;
}
