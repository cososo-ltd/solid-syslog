/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#ifndef SOLIDSYSLOGPOSIXTCPSTREAMPRIVATE_H
#define SOLIDSYSLOGPOSIXTCPSTREAMPRIVATE_H

#include <stdint.h>

#include "SolidSyslogError.h"
#include "SolidSyslogPosixTcpStream.h"
#include "SolidSyslogPosixTcpStreamErrors.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogStreamDefinition.h"

struct SolidSyslogPosixTcpStream
{
    struct SolidSyslogStream Base;
    struct SolidSyslogPosixTcpStreamConfig Config;
    int Fd;
};

void SolidSyslogPosixTcpStream_Initialise(
    struct SolidSyslogStream* base,
    const struct SolidSyslogPosixTcpStreamConfig* config
);
void SolidSyslogPosixTcpStream_Cleanup(struct SolidSyslogStream* base);

static inline void PosixTcpStream_Report(
    enum SolidSyslogSeverity severity,
    uint16_t category,
    enum SolidSyslogPosixTcpStreamErrors code
)
{
    SolidSyslog_Error(severity, &PosixTcpStreamErrorSource, category, (int32_t) code);
}

#endif /* SOLIDSYSLOGPOSIXTCPSTREAMPRIVATE_H */
