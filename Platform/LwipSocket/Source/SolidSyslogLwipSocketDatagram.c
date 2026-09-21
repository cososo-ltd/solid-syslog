/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "lwip/opt.h"

#if LWIP_SOCKET && LWIP_UDP

#include "SolidSyslogError.h"
#include "SolidSyslogLwipSocketDatagramErrors.h"
#include "SolidSyslogLwipSocketDatagramPrivate.h"
#include "SolidSyslogNullDatagram.h"

const struct SolidSyslogErrorSource SolidSyslogLwipSocketDatagramErrorSource = {"LwipSocketDatagram"};

struct SolidSyslogAddress;

void SolidSyslogLwipSocketDatagram_Initialise(struct SolidSyslogDatagram* base)
{
    *base = *SolidSyslogNullDatagram_Get();
}

void SolidSyslogLwipSocketDatagram_Cleanup(struct SolidSyslogDatagram* base)
{
    /* Overwrite the abstract base with the shared NullDatagram vtable so
     * use-after-destroy is a safe no-op rather than a NULL-fn-pointer crash. */
    *base = *SolidSyslogNullDatagram_Get();
}

#else

/* ISO C forbids an empty translation unit. */
typedef int LwipSocketDatagram_EmptyTranslationUnit;

#endif /* LWIP_SOCKET && LWIP_UDP */
