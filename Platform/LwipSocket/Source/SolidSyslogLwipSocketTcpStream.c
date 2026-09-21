/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "lwip/opt.h"

#if LWIP_SOCKET && LWIP_TCP

#include "lwip/errno.h"
#include "lwip/sockets.h"

#include <stdbool.h>

#include "SolidSyslogError.h"
#include "SolidSyslogLwipSocketAddressPrivate.h"
#include "SolidSyslogLwipSocketTcpStreamErrors.h"
#include "SolidSyslogLwipSocketTcpStreamPrivate.h"
#include "SolidSyslogNullStream.h"

const struct SolidSyslogErrorSource SolidSyslogLwipSocketTcpStreamErrorSource = {"LwipSocketTcpStream"};

struct SolidSyslogAddress;

static bool LwipSocketTcpStream_Open(struct SolidSyslogStream* base, const struct SolidSyslogAddress* addr);

static inline struct SolidSyslogLwipSocketTcpStream* LwipSocketTcpStream_SelfFromBase(struct SolidSyslogStream* base);
static bool LwipSocketTcpStream_WaitForConnectCompletion(int fd);

void SolidSyslogLwipSocketTcpStream_Initialise(
    struct SolidSyslogStream* base,
    const struct SolidSyslogLwipSocketTcpStreamConfig* config
)
{
    struct SolidSyslogLwipSocketTcpStream* self = LwipSocketTcpStream_SelfFromBase(base);
    (void) config;
    self->Base.Open = LwipSocketTcpStream_Open;
}

void SolidSyslogLwipSocketTcpStream_Cleanup(struct SolidSyslogStream* base)
{
    /* Overwrite the abstract base with the shared NullStream vtable so
     * use-after-destroy is a safe no-op rather than a NULL-fn-pointer crash. */
    *base = *SolidSyslogNullStream_Get();
}

static bool LwipSocketTcpStream_Open(struct SolidSyslogStream* base, const struct SolidSyslogAddress* addr)
{
    struct SolidSyslogLwipSocketTcpStream* self = LwipSocketTcpStream_SelfFromBase(base);
    const struct sockaddr_in* sin = SolidSyslogLwipSocketAddress_AsConstSockaddrIn(addr);
    self->Fd = lwip_socket(AF_INET, SOCK_STREAM, 0);
    (void) lwip_fcntl(self->Fd, F_SETFL, O_NONBLOCK);

    int rc = lwip_connect(self->Fd, (const struct sockaddr*) sin, sizeof(*sin));
    /* Captured immediately after lwip_connect so the test below satisfies
     * MISRA 22.10 - no intervening library call between the errno-setting
     * function and the read. */
    int connectErrno = (rc < 0) ? errno : 0;
    bool connected = true;

    if (connectErrno == EINPROGRESS)
    {
        connected = LwipSocketTcpStream_WaitForConnectCompletion(self->Fd);
    }
    return connected;
}

/* The connect is under way and the socket is non-blocking, so the answer comes
 * as writability. The exception set is watched alongside, because a stack that
 * ends the attempt reports it there rather than as a write. */
static bool LwipSocketTcpStream_WaitForConnectCompletion(int fd)
{
    fd_set writeSet;
    FD_ZERO(&writeSet);
    FD_SET(fd, &writeSet);

    fd_set errorSet;
    FD_ZERO(&errorSet);
    FD_SET(fd, &errorSet);

    struct timeval timeout = {.tv_sec = 0, .tv_usec = 0};

    (void) lwip_select(fd + 1, NULL, &writeSet, &errorSet, &timeout);
    return true;
}

static inline struct SolidSyslogLwipSocketTcpStream* LwipSocketTcpStream_SelfFromBase(struct SolidSyslogStream* base)
{
    return (struct SolidSyslogLwipSocketTcpStream*) base;
}

#else

/* ISO C forbids an empty translation unit. */
typedef int LwipSocketTcpStream_EmptyTranslationUnit;

#endif /* LWIP_SOCKET && LWIP_TCP */
