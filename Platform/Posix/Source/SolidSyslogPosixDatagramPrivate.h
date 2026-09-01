/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#ifndef SOLIDSYSLOGPOSIXDATAGRAMPRIVATE_H
#define SOLIDSYSLOGPOSIXDATAGRAMPRIVATE_H

#include <stdint.h>

#include <stdbool.h>

#include "SolidSyslogDatagramDefinition.h"
#include "SolidSyslogError.h"
#include "SolidSyslogPosixDatagramErrors.h"
#include "SolidSyslogPrival.h"

struct SolidSyslogPosixDatagram
{
    struct SolidSyslogDatagram Base;
    int Fd;
    bool Connected;
};

void SolidSyslogPosixDatagram_Initialise(struct SolidSyslogDatagram* base);
void SolidSyslogPosixDatagram_Cleanup(struct SolidSyslogDatagram* base);

static inline void PosixDatagram_Report(
    enum SolidSyslogSeverity severity,
    uint16_t category,
    enum SolidSyslogPosixDatagramErrors code
)
{
    SolidSyslog_Error(severity, &PosixDatagramErrorSource, category, (int32_t) code);
}

#endif /* SOLIDSYSLOGPOSIXDATAGRAMPRIVATE_H */
