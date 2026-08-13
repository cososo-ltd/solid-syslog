/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#ifndef SOLIDSYSLOGUDPSENDERPRIVATE_H
#define SOLIDSYSLOGUDPSENDERPRIVATE_H

#include <stdbool.h>
#include <stdint.h>

#include "SolidSyslogError.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogSenderDefinition.h"
#include "SolidSyslogUdpSender.h"
#include "SolidSyslogUdpSenderErrors.h"

struct SolidSyslogUdpSender
{
    struct SolidSyslogSender Base;
    struct SolidSyslogUdpSenderConfig Config;
    bool Connected;
    bool DeliveryHealthy;
    uint32_t LastEndpointVersion;
};

void UdpSender_Initialise(struct SolidSyslogSender* base, const struct SolidSyslogUdpSenderConfig* config);
void UdpSender_Cleanup(struct SolidSyslogSender* base);

static inline void UdpSender_Report(
    enum SolidSyslogSeverity severity,
    uint16_t category,
    enum SolidSyslogUdpSenderErrors code
)
{
    SolidSyslog_Error(severity, &UdpSenderErrorSource, category, (int32_t) code);
}

#endif /* SOLIDSYSLOGUDPSENDERPRIVATE_H */
