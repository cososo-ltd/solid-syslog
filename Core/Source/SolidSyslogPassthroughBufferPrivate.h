/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#ifndef SOLIDSYSLOGPASSTHROUGHBUFFERPRIVATE_H
#define SOLIDSYSLOGPASSTHROUGHBUFFERPRIVATE_H

#include <stdint.h>

#include "SolidSyslogBufferDefinition.h"
#include "SolidSyslogError.h"
#include "SolidSyslogPassthroughBufferErrors.h"
#include "SolidSyslogPrival.h"

struct SolidSyslogSender;

struct SolidSyslogPassthroughBuffer
{
    struct SolidSyslogBuffer Base;
    struct SolidSyslogSender* Sender;
};

void SolidSyslogPassthroughBuffer_Initialise(struct SolidSyslogBuffer* base, struct SolidSyslogSender* sender);
void SolidSyslogPassthroughBuffer_Cleanup(struct SolidSyslogBuffer* base);

static inline void PassthroughBuffer_Report(
    enum SolidSyslogSeverity severity,
    uint16_t category,
    enum SolidSyslogPassthroughBufferErrors code
)
{
    SolidSyslog_Error(severity, &PassthroughBufferErrorSource, category, (int32_t) code);
}

#endif /* SOLIDSYSLOGPASSTHROUGHBUFFERPRIVATE_H */
