/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "lwip/opt.h"

/* This component resolves names through lwIP's getaddrinfo, which the stack
   provides only when built with both the sockets layer and DNS. */
#if LWIP_SOCKET && LWIP_DNS

#include "SolidSyslogError.h"
#include "SolidSyslogLwipSocketResolverErrors.h"
#include "SolidSyslogLwipSocketResolverPrivate.h"
#include "SolidSyslogNullResolver.h"

const struct SolidSyslogErrorSource SolidSyslogLwipSocketResolverErrorSource = {"LwipSocketResolver"};

void SolidSyslogLwipSocketResolver_Initialise(struct SolidSyslogResolver* base)
{
    (void) base;
}

void SolidSyslogLwipSocketResolver_Cleanup(struct SolidSyslogResolver* base)
{
    /* Overwrite the abstract base with the shared NullResolver vtable so
     * use-after-destroy is a safe no-op rather than a NULL-fn-pointer crash. */
    *base = *SolidSyslogNullResolver_Get();
}

#else

/* ISO C forbids an empty translation unit. */
typedef int LwipSocketResolver_EmptyTranslationUnit;

#endif /* LWIP_SOCKET && LWIP_DNS */
