/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "lwip/opt.h"

#if LWIP_SOCKET && LWIP_UDP

#include "lwip/sockets.h"

#include <stdbool.h>
#include <stddef.h>

#include "SolidSyslogDatagramDefinition.h"
#include "SolidSyslogError.h"
#include "SolidSyslogLwipSocketAddressPrivate.h"
#include "SolidSyslogLwipSocketDatagramErrors.h"
#include "SolidSyslogLwipSocketDatagramPrivate.h"
#include "SolidSyslogNullDatagram.h"

const struct SolidSyslogErrorSource SolidSyslogLwipSocketDatagramErrorSource = {"LwipSocketDatagram"};

struct SolidSyslogAddress;

static bool LwipSocketDatagram_Open(struct SolidSyslogDatagram* base);
static enum SolidSyslogDatagramSendResult LwipSocketDatagram_SendTo(
    struct SolidSyslogDatagram* base,
    const void* buffer,
    size_t size,
    const struct SolidSyslogAddress* addr
);
static void LwipSocketDatagram_Close(struct SolidSyslogDatagram* base);

static inline struct SolidSyslogLwipSocketDatagram* LwipSocketDatagram_SelfFromBase(struct SolidSyslogDatagram* base);

void SolidSyslogLwipSocketDatagram_Initialise(struct SolidSyslogDatagram* base)
{
    struct SolidSyslogLwipSocketDatagram* self = LwipSocketDatagram_SelfFromBase(base);
    self->Base.Open = LwipSocketDatagram_Open;
    self->Base.SendTo = LwipSocketDatagram_SendTo;
    self->Base.Close = LwipSocketDatagram_Close;
}

void SolidSyslogLwipSocketDatagram_Cleanup(struct SolidSyslogDatagram* base)
{
    /* Overwrite the abstract base with the shared NullDatagram vtable so
     * use-after-destroy is a safe no-op rather than a NULL-fn-pointer crash. */
    *base = *SolidSyslogNullDatagram_Get();
}

static bool LwipSocketDatagram_Open(struct SolidSyslogDatagram* base)
{
    struct SolidSyslogLwipSocketDatagram* self = LwipSocketDatagram_SelfFromBase(base);
    self->Fd = lwip_socket(AF_INET, SOCK_DGRAM, 0);
    return true;
}

static inline struct SolidSyslogLwipSocketDatagram* LwipSocketDatagram_SelfFromBase(struct SolidSyslogDatagram* base)
{
    return (struct SolidSyslogLwipSocketDatagram*) base;
}

static enum SolidSyslogDatagramSendResult LwipSocketDatagram_SendTo(
    struct SolidSyslogDatagram* base,
    const void* buffer,
    size_t size,
    const struct SolidSyslogAddress* addr
)
{
    struct SolidSyslogLwipSocketDatagram* self = LwipSocketDatagram_SelfFromBase(base);
    const struct sockaddr_in* sin = SolidSyslogLwipSocketAddress_AsConstSockaddrIn(addr);
    (void) lwip_sendto(self->Fd, buffer, size, 0, (const struct sockaddr*) sin, sizeof(*sin));
    return SOLIDSYSLOG_DATAGRAM_SEND_RESULT_SENT;
}

static void LwipSocketDatagram_Close(struct SolidSyslogDatagram* base)
{
    struct SolidSyslogLwipSocketDatagram* self = LwipSocketDatagram_SelfFromBase(base);
    (void) lwip_close(self->Fd);
}

#else

/* ISO C forbids an empty translation unit. */
typedef int LwipSocketDatagram_EmptyTranslationUnit;

#endif /* LWIP_SOCKET && LWIP_UDP */
