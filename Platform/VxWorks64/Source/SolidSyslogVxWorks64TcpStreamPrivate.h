/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#ifndef SOLIDSYSLOGVXWORKS64TCPSTREAMPRIVATE_H
#define SOLIDSYSLOGVXWORKS64TCPSTREAMPRIVATE_H

#include <stdint.h>

#include "SolidSyslogError.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogStreamDefinition.h"
#include "SolidSyslogVxWorks64TcpStream.h"
#include "SolidSyslogVxWorks64TcpStreamErrors.h"

struct SolidSyslogVxWorks64TcpStream
{
    struct SolidSyslogStream Base;
    struct SolidSyslogVxWorks64TcpStreamConfig Config;
};

void SolidSyslogVxWorks64TcpStream_Initialise(
    struct SolidSyslogStream* base,
    const struct SolidSyslogVxWorks64TcpStreamConfig* config
);

static inline void VxWorks64TcpStream_Report(
    enum SolidSyslogSeverity severity,
    uint16_t category,
    enum SolidSyslogTcpStreamErrors code
)
{
    SolidSyslog_Error(severity, &SolidSyslogVxWorks64TcpStreamErrorSource, category, (int32_t) code);
}

#endif /* SOLIDSYSLOGVXWORKS64TCPSTREAMPRIVATE_H */
