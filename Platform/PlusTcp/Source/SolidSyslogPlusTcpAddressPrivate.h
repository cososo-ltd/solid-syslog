/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#ifndef SOLIDSYSLOGPLUSTCPADDRESSPRIVATE_H
#define SOLIDSYSLOGPLUSTCPADDRESSPRIVATE_H

#include <stdint.h>

#include "FreeRTOS.h"
#include "FreeRTOS_Sockets.h"
#include "SolidSyslogError.h"
#include "SolidSyslogPlusTcpAddressErrors.h"
#include "SolidSyslogPrival.h"

struct SolidSyslogAddress;

struct SolidSyslogPlusTcpAddress
{
    struct freertos_sockaddr Sockaddr;
};

void SolidSyslogPlusTcpAddress_Initialise(struct SolidSyslogAddress* base);
void SolidSyslogPlusTcpAddress_Cleanup(struct SolidSyslogAddress* base);

static inline struct freertos_sockaddr* SolidSyslogPlusTcpAddress_AsFreertosSockaddr(struct SolidSyslogAddress* base)
{
    return &((struct SolidSyslogPlusTcpAddress*) base)->Sockaddr;
}

static inline const struct freertos_sockaddr* SolidSyslogPlusTcpAddress_AsConstFreertosSockaddr(
    const struct SolidSyslogAddress* base
)
{
    return &((const struct SolidSyslogPlusTcpAddress*) base)->Sockaddr;
}

static inline void PlusTcpAddress_Report(
    enum SolidSyslogSeverity severity,
    uint16_t category,
    enum SolidSyslogPlusTcpAddressErrors code
)
{
    SolidSyslog_Error(severity, &SolidSyslogPlusTcpAddressErrorSource, category, (int32_t) code);
}

#endif /* SOLIDSYSLOGPLUSTCPADDRESSPRIVATE_H */
