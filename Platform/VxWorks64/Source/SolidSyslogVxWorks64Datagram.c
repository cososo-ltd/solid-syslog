/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogVxWorks64Datagram.h"

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>

#include "vxWorks.h"

#include <netinet/in.h>
#include <sys/socket.h>

#include "ioLib.h"
#include "sockLib.h"

#include "SolidSyslogDatagramDefinition.h"
#include "SolidSyslogError.h"
#include "SolidSyslogNullDatagram.h"
#include "SolidSyslogUdpPayload.h"
#include "SolidSyslogVxWorks64AddressPrivate.h"
#include "SolidSyslogVxWorks64DatagramErrors.h"
#include "SolidSyslogVxWorks64DatagramPrivate.h"

const struct SolidSyslogErrorSource SolidSyslogVxWorks64DatagramErrorSource = {"VxWorks64Datagram"};

enum
{
    VXWORKS64_DATAGRAM_NO_SOCKET = -1
};

static bool VxWorks64Datagram_Open(struct SolidSyslogDatagram* base);
static enum SolidSyslogDatagramSendResult VxWorks64Datagram_SendTo(
    struct SolidSyslogDatagram* base,
    const void* buffer,
    size_t size,
    const struct SolidSyslogAddress* addr
);

static size_t VxWorks64Datagram_MaxPayload(struct SolidSyslogDatagram* base);
static void VxWorks64Datagram_Close(struct SolidSyslogDatagram* base);

static inline struct SolidSyslogVxWorks64Datagram* VxWorks64Datagram_SelfFromBase(struct SolidSyslogDatagram* base);
static inline bool VxWorks64Datagram_HasSocket(const struct SolidSyslogVxWorks64Datagram* self);
static inline enum SolidSyslogDatagramSendResult VxWorks64Datagram_SendToStack(
    const struct SolidSyslogVxWorks64Datagram* self,
    const void* buffer,
    size_t size,
    const struct SolidSyslogAddress* addr
);

void SolidSyslogVxWorks64Datagram_Initialise(struct SolidSyslogDatagram* base)
{
    struct SolidSyslogVxWorks64Datagram* self = VxWorks64Datagram_SelfFromBase(base);
    self->Base.Open = VxWorks64Datagram_Open;
    self->Base.SendTo = VxWorks64Datagram_SendTo;
    self->Base.MaxPayload = VxWorks64Datagram_MaxPayload;
    self->Base.Close = VxWorks64Datagram_Close;
    self->Fd = VXWORKS64_DATAGRAM_NO_SOCKET;
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
    VxWorks64Datagram_Close(base);
    /* Overwrite the abstract base with the shared NullDatagram vtable so
     * use-after-destroy is a safe no-op rather than a send on a closed
     * socket. */
    *base = *SolidSyslogNullDatagram_Get();
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
    /* The stack has no don't-fragment option, so a record it would fragment
     * is refused here instead, and the sender trims it to fit. */
    enum SolidSyslogDatagramSendResult result = SOLIDSYSLOG_DATAGRAM_SEND_RESULT_OVERSIZE;
    if (size <= VxWorks64Datagram_MaxPayload(base))
    {
        result = VxWorks64Datagram_SendToStack(VxWorks64Datagram_SelfFromBase(base), buffer, size, addr);
    }
    return result;
}

static inline enum SolidSyslogDatagramSendResult VxWorks64Datagram_SendToStack(
    const struct SolidSyslogVxWorks64Datagram* self,
    const void* buffer,
    size_t size,
    const struct SolidSyslogAddress* addr
)
{
    const struct sockaddr_in* sin = SolidSyslogVxWorks64Address_AsConstSockaddrIn(addr);
    /* sockLib takes a non-const char buffer and address that it only reads
     * (D.006, D.013). */
    int sent = sendto(self->Fd, (char*) buffer, (int) size, 0, (struct sockaddr*) sin, (int) sizeof(*sin));
    /* Read errno straight after the call that set it, with nothing between
     * (MISRA 22.10). */
    int sendErrno = (sent == ERROR) ? errno : 0;
    enum SolidSyslogDatagramSendResult result = SOLIDSYSLOG_DATAGRAM_SEND_RESULT_FAILED;
    if (sent != ERROR)
    {
        result = SOLIDSYSLOG_DATAGRAM_SEND_RESULT_SENT;
    }
    else if (sendErrno == EMSGSIZE)
    {
        result = SOLIDSYSLOG_DATAGRAM_SEND_RESULT_OVERSIZE;
    }
    else
    {
        /* Any other failure - result stays FAILED. */
    }
    return result;
}

static size_t VxWorks64Datagram_MaxPayload(struct SolidSyslogDatagram* base)
{
    /* The stack offers no path-MTU query for UDP, so the conservative figure
     * the Datagram contract asks for is the only honest one. */
    (void) base;
    return SolidSyslogUdpPayload_UnknownPath(false);
}

static void VxWorks64Datagram_Close(struct SolidSyslogDatagram* base)
{
    struct SolidSyslogVxWorks64Datagram* self = VxWorks64Datagram_SelfFromBase(base);
    if (VxWorks64Datagram_HasSocket(self) == true)
    {
        (void) close(self->Fd);
        self->Fd = VXWORKS64_DATAGRAM_NO_SOCKET;
    }
}
