/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#ifndef SOLIDSYSLOGLWIPSOCKETDATAGRAMPRIVATE_H
#define SOLIDSYSLOGLWIPSOCKETDATAGRAMPRIVATE_H

#include <stdint.h>

#include "SolidSyslogDatagramDefinition.h"
#include "SolidSyslogError.h"
#include "SolidSyslogLwipSocketDatagramErrors.h"
#include "SolidSyslogPrival.h"

struct SolidSyslogLwipSocketDatagram
{
    struct SolidSyslogDatagram Base;
    int Fd;
};

void SolidSyslogLwipSocketDatagram_Initialise(struct SolidSyslogDatagram* base);
void SolidSyslogLwipSocketDatagram_Cleanup(struct SolidSyslogDatagram* base);

static inline void LwipSocketDatagram_Report(
    enum SolidSyslogSeverity severity,
    uint16_t category,
    enum SolidSyslogDatagramErrors code
)
{
    SolidSyslog_Error(severity, &SolidSyslogLwipSocketDatagramErrorSource, category, (int32_t) code);
}

#endif /* SOLIDSYSLOGLWIPSOCKETDATAGRAMPRIVATE_H */
