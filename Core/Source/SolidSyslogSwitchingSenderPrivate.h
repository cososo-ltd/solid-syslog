/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#ifndef SOLIDSYSLOGSWITCHINGSENDERPRIVATE_H
#define SOLIDSYSLOGSWITCHINGSENDERPRIVATE_H

#include <stdint.h>

#include "SolidSyslogError.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogSenderDefinition.h"
#include "SolidSyslogSwitchingSender.h"
#include "SolidSyslogSwitchingSenderErrors.h"

struct SolidSyslogSwitchingSender
{
    struct SolidSyslogSender Base;
    struct SolidSyslogSwitchingSenderConfig Config;
    struct SolidSyslogSender* CurrentSender;
};

void SolidSyslogSwitchingSender_Initialise(
    struct SolidSyslogSender* base,
    const struct SolidSyslogSwitchingSenderConfig* config
);
void SolidSyslogSwitchingSender_Cleanup(struct SolidSyslogSender* base);

static inline void SwitchingSender_Report(
    enum SolidSyslogSeverity severity,
    uint16_t category,
    enum SolidSyslogSwitchingSenderErrors code
)
{
    SolidSyslog_Error(severity, &SolidSyslogSwitchingSenderErrorSource, category, (int32_t) code);
}

#endif /* SOLIDSYSLOGSWITCHINGSENDERPRIVATE_H */
