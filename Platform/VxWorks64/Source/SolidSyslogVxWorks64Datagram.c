/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogVxWorks64Datagram.h"

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "vxWorks.h"

#include <netinet/in.h>
#include <sys/socket.h>

#include "arpLib.h"
#include "inetLib.h"
#include "ioLib.h"
#include "sockLib.h"
#include "sysLib.h"

#include "SolidSyslogDatagramCategories.h"
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
    VXWORKS64_DATAGRAM_NO_SOCKET = -1,
    /* arpResolve fills an Ethernet address, six bytes, through 16-bit
     * accesses. */
    VXWORKS64_DATAGRAM_LINK_ADDRESS_HALFWORDS = 3,
    /* Two tries make one wait for a reply between them; arpResolve does not
     * wait after its last, so one try sends a request and does not wait. */
    VXWORKS64_DATAGRAM_RESOLVE_TRIES_WAITING = 2,
    VXWORKS64_DATAGRAM_RESOLVE_TRIES_NOT_WAITING = 1,
    VXWORKS64_DATAGRAM_RESOLVE_WAIT_MS = 100,
    VXWORKS64_DATAGRAM_MILLISECONDS_PER_SECOND = 1000
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
static inline bool VxWorks64Datagram_ResolveNextHop(
    struct SolidSyslogVxWorks64Datagram* self,
    const struct SolidSyslogAddress* addr
);
static inline int VxWorks64Datagram_TicksFor(int milliseconds);
static inline void VxWorks64Datagram_ReportUnresolved(int resolveErrno);

void SolidSyslogVxWorks64Datagram_Initialise(struct SolidSyslogDatagram* base)
{
    struct SolidSyslogVxWorks64Datagram* self = VxWorks64Datagram_SelfFromBase(base);
    self->Base.Open = VxWorks64Datagram_Open;
    self->Base.SendTo = VxWorks64Datagram_SendTo;
    self->Base.MaxPayload = VxWorks64Datagram_MaxPayload;
    self->Base.Close = VxWorks64Datagram_Close;
    self->Fd = VXWORKS64_DATAGRAM_NO_SOCKET;
    self->ResolveFailing = false;
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
    /* Use-after-destroy lands on the NullDatagram vtable. */
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
    /* The stack has no don't-fragment option, so an oversize record is refused
     * here. */
    enum SolidSyslogDatagramSendResult result = SOLIDSYSLOG_DATAGRAM_SEND_RESULT_OVERSIZE;
    if (size <= VxWorks64Datagram_MaxPayload(base))
    {
        struct SolidSyslogVxWorks64Datagram* self = VxWorks64Datagram_SelfFromBase(base);
        result = SOLIDSYSLOG_DATAGRAM_SEND_RESULT_FAILED;
        if (VxWorks64Datagram_ResolveNextHop(self, addr))
        {
            result = VxWorks64Datagram_SendToStack(self, buffer, size, addr);
        }
    }
    return result;
}

/* The stack holds at most one datagram for a next hop it is still resolving,
 * replacing it with each later one, yet sendto accepts them all. A record is
 * handed over only once its next hop is resolved. */
static inline bool VxWorks64Datagram_ResolveNextHop(
    struct SolidSyslogVxWorks64Datagram* self,
    const struct SolidSyslogAddress* addr
)
{
    char nextHop[INET_ADDR_LEN];
    unsigned short linkAddress[VXWORKS64_DATAGRAM_LINK_ADDRESS_HALFWORDS];
    inet_ntoa_b(SolidSyslogVxWorks64Address_AsConstSockaddrIn(addr)->sin_addr, nextHop);
    STATUS status = arpResolve(
        nextHop,
        (char*) linkAddress,
        self->ResolveFailing ? VXWORKS64_DATAGRAM_RESOLVE_TRIES_NOT_WAITING : VXWORKS64_DATAGRAM_RESOLVE_TRIES_WAITING,
        VxWorks64Datagram_TicksFor(VXWORKS64_DATAGRAM_RESOLVE_WAIT_MS)
    );
    int resolveErrno = (status == ERROR) ? errno : 0;
    bool resolved = status == OK;
    if (!resolved && !self->ResolveFailing)
    {
        VxWorks64Datagram_ReportUnresolved(resolveErrno);
    }
    self->ResolveFailing = !resolved;
    return resolved;
}

/* Rounded up, so a wait shorter than a tick is a tick rather than none. */
static inline int VxWorks64Datagram_TicksFor(int milliseconds)
{
    int64_t ticks =
        (((int64_t) milliseconds * (int64_t) sysClkRateGet()) + (VXWORKS64_DATAGRAM_MILLISECONDS_PER_SECOND - 1)) /
        VXWORKS64_DATAGRAM_MILLISECONDS_PER_SECOND;
    return (int) ticks;
}

static inline void VxWorks64Datagram_ReportUnresolved(int resolveErrno)
{
    SolidSyslog_Error(
        SOLIDSYSLOG_DATAGRAM_NEXT_HOP_UNRESOLVED_SEVERITY,
        &SolidSyslogVxWorks64DatagramErrorSource,
        SOLIDSYSLOG_CAT_DATAGRAM_NEXT_HOP_UNRESOLVED,
        (int32_t) SOLIDSYSLOG_DATAGRAM_ERROR_NEXT_HOP_UNRESOLVED
    );
    SolidSyslog_Error(
        SOLIDSYSLOG_DATAGRAM_NEXT_HOP_UNRESOLVED_SEVERITY,
        &SolidSyslogVxWorks64DatagramErrorSource,
        SOLIDSYSLOG_CAT_NATIVE_ERROR,
        (int32_t) resolveErrno
    );
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
    /* No path-MTU query exists for UDP, so this returns the contract's
     * unknown-path figure. */
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
