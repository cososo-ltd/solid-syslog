/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "lwip/opt.h"

#if LWIP_SOCKET && LWIP_TCP

#include "SolidSyslogError.h"
#include "SolidSyslogLwipSocketTcpStreamErrors.h"
#include "SolidSyslogLwipSocketTcpStreamPrivate.h"
#include "SolidSyslogNullStream.h"

const struct SolidSyslogErrorSource SolidSyslogLwipSocketTcpStreamErrorSource = {"LwipSocketTcpStream"};

struct SolidSyslogAddress;

void SolidSyslogLwipSocketTcpStream_Initialise(
    struct SolidSyslogStream* base,
    const struct SolidSyslogLwipSocketTcpStreamConfig* config
)
{
    (void) base;
    (void) config;
}

void SolidSyslogLwipSocketTcpStream_Cleanup(struct SolidSyslogStream* base)
{
    /* Overwrite the abstract base with the shared NullStream vtable so
     * use-after-destroy is a safe no-op rather than a NULL-fn-pointer crash. */
    *base = *SolidSyslogNullStream_Get();
}

#else

/* ISO C forbids an empty translation unit. */
typedef int LwipSocketTcpStream_EmptyTranslationUnit;

#endif /* LWIP_SOCKET && LWIP_TCP */
