/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include <stdbool.h>
#include <stddef.h>

#include "SolidSyslogDatagramDefinition.h"
#include "SolidSyslogDatagram.h"

struct SolidSyslogAddress;

bool SolidSyslogDatagram_Open(struct SolidSyslogDatagram* datagram)
{
    return datagram->Open(datagram);
}

enum SolidSyslogDatagramSendResult SolidSyslogDatagram_SendTo(
    struct SolidSyslogDatagram* datagram,
    const void* buffer,
    size_t size,
    const struct SolidSyslogAddress* addr
)
{
    return datagram->SendTo(datagram, buffer, size, addr);
}

size_t SolidSyslogDatagram_MaxPayload(struct SolidSyslogDatagram* datagram)
{
    return datagram->MaxPayload(datagram);
}

void SolidSyslogDatagram_Close(struct SolidSyslogDatagram* datagram)
{
    datagram->Close(datagram);
}
