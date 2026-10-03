/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#ifndef SOLIDSYSLOGVXWORKS64DATAGRAMPRIVATE_H
#define SOLIDSYSLOGVXWORKS64DATAGRAMPRIVATE_H

#include <stdint.h>

#include <stdbool.h>

#include "SolidSyslogDatagramDefinition.h"
#include "SolidSyslogError.h"
#include "SolidSyslogVxWorks64DatagramErrors.h"
#include "SolidSyslogPrival.h"

struct SolidSyslogVxWorks64Datagram
{
    struct SolidSyslogDatagram Base;
    int Fd;
};

void SolidSyslogVxWorks64Datagram_Initialise(struct SolidSyslogDatagram* base);
void SolidSyslogVxWorks64Datagram_Cleanup(struct SolidSyslogDatagram* base);

static inline void VxWorks64Datagram_Report(
    enum SolidSyslogSeverity severity,
    uint16_t category,
    enum SolidSyslogDatagramErrors code
)
{
    SolidSyslog_Error(severity, &SolidSyslogVxWorks64DatagramErrorSource, category, (int32_t) code);
}

#endif /* SOLIDSYSLOGVXWORKS64DATAGRAMPRIVATE_H */
