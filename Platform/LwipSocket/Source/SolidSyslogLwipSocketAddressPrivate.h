/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#ifndef SOLIDSYSLOGLWIPSOCKETADDRESSPRIVATE_H
#define SOLIDSYSLOGLWIPSOCKETADDRESSPRIVATE_H

#include "lwip/sockets.h"

#include <stdint.h>

#include "SolidSyslogError.h"
#include "SolidSyslogLwipSocketAddressErrors.h"
#include "SolidSyslogPrival.h"

struct SolidSyslogAddress;

struct SolidSyslogLwipSocketAddress
{
    struct sockaddr_in Sockaddr;
};

void SolidSyslogLwipSocketAddress_Initialise(struct SolidSyslogAddress* base);
void SolidSyslogLwipSocketAddress_Cleanup(struct SolidSyslogAddress* base);

static inline struct sockaddr_in* SolidSyslogLwipSocketAddress_AsSockaddrIn(struct SolidSyslogAddress* base)
{
    return &((struct SolidSyslogLwipSocketAddress*) base)->Sockaddr;
}

static inline const struct sockaddr_in* SolidSyslogLwipSocketAddress_AsConstSockaddrIn(
    const struct SolidSyslogAddress* base
)
{
    return &((const struct SolidSyslogLwipSocketAddress*) base)->Sockaddr;
}

static inline void LwipSocketAddress_Report(
    enum SolidSyslogSeverity severity,
    uint16_t category,
    enum SolidSyslogAddressErrors code
)
{
    SolidSyslog_Error(severity, &SolidSyslogLwipSocketAddressErrorSource, category, (int32_t) code);
}

#endif /* SOLIDSYSLOGLWIPSOCKETADDRESSPRIVATE_H */
