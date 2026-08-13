/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include <stdbool.h>
#include <stddef.h>

#include "SolidSyslogSenderDefinition.h"
#include "SolidSyslogSender.h"

bool SolidSyslogSender_Send(struct SolidSyslogSender* sender, const void* buffer, size_t size)
{
    return sender->Send(sender, buffer, size);
}

void SolidSyslogSender_Disconnect(struct SolidSyslogSender* sender)
{
    sender->Disconnect(sender);
}
