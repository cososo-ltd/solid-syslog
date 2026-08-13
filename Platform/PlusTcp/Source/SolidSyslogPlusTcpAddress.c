/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include <string.h>

#include "FreeRTOS_Sockets.h"
#include "SolidSyslogError.h"
#include "SolidSyslogPlusTcpAddressErrors.h"
#include "SolidSyslogPlusTcpAddressPrivate.h"

const struct SolidSyslogErrorSource PlusTcpAddressErrorSource = {"PlusTcpAddress"};

struct SolidSyslogAddress;

void PlusTcpAddress_Initialise(struct SolidSyslogAddress* base)
{
    struct SolidSyslogPlusTcpAddress* self = (struct SolidSyslogPlusTcpAddress*) base;
    (void) memset(&self->Sockaddr, 0, sizeof(self->Sockaddr));
}

void PlusTcpAddress_Cleanup(struct SolidSyslogAddress* base)
{
    (void) base;
}
