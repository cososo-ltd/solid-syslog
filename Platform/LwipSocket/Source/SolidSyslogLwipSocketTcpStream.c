/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "lwip/opt.h"

#if LWIP_SOCKET && LWIP_TCP

#include "lwip/sockets.h"

#include <stdbool.h>

#include "SolidSyslogError.h"
#include "SolidSyslogLwipSocketTcpStreamErrors.h"
#include "SolidSyslogLwipSocketTcpStreamPrivate.h"
#include "SolidSyslogNullStream.h"

const struct SolidSyslogErrorSource SolidSyslogLwipSocketTcpStreamErrorSource = {"LwipSocketTcpStream"};

struct SolidSyslogAddress;

static bool LwipSocketTcpStream_Open(struct SolidSyslogStream* base, const struct SolidSyslogAddress* addr);

static inline struct SolidSyslogLwipSocketTcpStream* LwipSocketTcpStream_SelfFromBase(struct SolidSyslogStream* base);

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
    (void) addr;
    self->Fd = lwip_socket(AF_INET, SOCK_STREAM, 0);
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
