/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "lwip/opt.h"

#if LWIP_SOCKET

#include "SolidSyslogError.h"
#include "SolidSyslogLwipSocketAddressErrors.h"
#include "SolidSyslogLwipSocketAddressPrivate.h"

const struct SolidSyslogErrorSource SolidSyslogLwipSocketAddressErrorSource = {"LwipSocketAddress"};

struct SolidSyslogAddress;

void SolidSyslogLwipSocketAddress_Initialise(struct SolidSyslogAddress* base)
{
    (void) base;
}

void SolidSyslogLwipSocketAddress_Cleanup(struct SolidSyslogAddress* base)
{
    (void) base;
}

#else

/* ISO C forbids an empty translation unit. */
typedef int LwipSocketAddress_EmptyTranslationUnit;

#endif /* LWIP_SOCKET */
