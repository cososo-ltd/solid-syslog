/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogVxWorks64Datagram.h"

#include <stdbool.h>
#include <stddef.h>

#include "vxWorks.h"

#include <netinet/in.h>
#include <sys/socket.h>

#include "sockLib.h"

#include "SolidSyslogDatagramDefinition.h"
#include "SolidSyslogError.h"
#include "SolidSyslogVxWorks64AddressPrivate.h"
#include "SolidSyslogVxWorks64DatagramErrors.h"
#include "SolidSyslogVxWorks64DatagramPrivate.h"

const struct SolidSyslogErrorSource SolidSyslogVxWorks64DatagramErrorSource = {"VxWorks64Datagram"};

static bool VxWorks64Datagram_Open(struct SolidSyslogDatagram* base);
static enum SolidSyslogDatagramSendResult VxWorks64Datagram_SendTo(
    struct SolidSyslogDatagram* base,
    const void* buffer,
    size_t size,
    const struct SolidSyslogAddress* addr
);

static inline struct SolidSyslogVxWorks64Datagram* VxWorks64Datagram_SelfFromBase(struct SolidSyslogDatagram* base);
static inline bool VxWorks64Datagram_HasSocket(const struct SolidSyslogVxWorks64Datagram* self);

void SolidSyslogVxWorks64Datagram_Initialise(struct SolidSyslogDatagram* base)
{
    struct SolidSyslogVxWorks64Datagram* self = VxWorks64Datagram_SelfFromBase(base);
    self->Base.Open = VxWorks64Datagram_Open;
    self->Base.SendTo = VxWorks64Datagram_SendTo;
}

static inline struct SolidSyslogVxWorks64Datagram* VxWorks64Datagram_SelfFromBase(struct SolidSyslogDatagram* base)
{
    return (struct SolidSyslogVxWorks64Datagram*) base;
}

static inline bool VxWorks64Datagram_HasSocket(const struct SolidSyslogVxWorks64Datagram* self)
{
    return self->Fd >= 0;
}

void SolidSyslogVxWorks64Datagram_Cleanup(struct SolidSyslogDatagram* base)
{
    (void) base;
}

static bool VxWorks64Datagram_Open(struct SolidSyslogDatagram* base)
{
    struct SolidSyslogVxWorks64Datagram* self = VxWorks64Datagram_SelfFromBase(base);
    self->Fd = socket(AF_INET, SOCK_DGRAM, 0);
    return VxWorks64Datagram_HasSocket(self);
}

static enum SolidSyslogDatagramSendResult VxWorks64Datagram_SendTo(
    struct SolidSyslogDatagram* base,
    const void* buffer,
    size_t size,
    const struct SolidSyslogAddress* addr
)
{
    struct SolidSyslogVxWorks64Datagram* self = VxWorks64Datagram_SelfFromBase(base);
    const struct sockaddr_in* sin = SolidSyslogVxWorks64Address_AsConstSockaddrIn(addr);
    /* sockLib takes a non-const buffer and address it does not modify (D.006). */
    (void) sendto(self->Fd, (char*) buffer, (int) size, 0, (struct sockaddr*) sin, (int) sizeof(*sin));
    return SOLIDSYSLOG_DATAGRAM_SEND_RESULT_FAILED;
}
