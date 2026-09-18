/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogError.h"
#include "SolidSyslogPosixAddressErrors.h"
#include "SolidSyslogPosixAddressPrivate.h"

#include <netinet/in.h>
#include <string.h>

const struct SolidSyslogErrorSource SolidSyslogPosixAddressErrorSource = {"PosixAddress"};

struct SolidSyslogAddress;

void SolidSyslogPosixAddress_Initialise(struct SolidSyslogAddress* base)
{
    struct SolidSyslogPosixAddress* self = (struct SolidSyslogPosixAddress*) base;
    (void) memset(&self->Sockaddr, 0, sizeof(self->Sockaddr));
}

void SolidSyslogPosixAddress_Cleanup(struct SolidSyslogAddress* base)
{
    (void) base;
}
