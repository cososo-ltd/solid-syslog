/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#ifndef SOLIDSYSLOGLWIPSOCKETTCPSTREAMPRIVATE_H
#define SOLIDSYSLOGLWIPSOCKETTCPSTREAMPRIVATE_H

#include <stdint.h>

#include "SolidSyslogError.h"
#include "SolidSyslogLwipSocketTcpStream.h"
#include "SolidSyslogLwipSocketTcpStreamErrors.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogStreamDefinition.h"

struct SolidSyslogLwipSocketTcpStream
{
    struct SolidSyslogStream Base;
    struct SolidSyslogLwipSocketTcpStreamConfig Config;
    int Fd;
};

void SolidSyslogLwipSocketTcpStream_Initialise(
    struct SolidSyslogStream* base,
    const struct SolidSyslogLwipSocketTcpStreamConfig* config
);
void SolidSyslogLwipSocketTcpStream_Cleanup(struct SolidSyslogStream* base);

static inline void LwipSocketTcpStream_Report(
    enum SolidSyslogSeverity severity,
    uint16_t category,
    enum SolidSyslogTcpStreamErrors code
)
{
    SolidSyslog_Error(severity, &SolidSyslogLwipSocketTcpStreamErrorSource, category, (int32_t) code);
}

#endif /* SOLIDSYSLOGLWIPSOCKETTCPSTREAMPRIVATE_H */
