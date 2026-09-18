/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#ifndef SOLIDSYSLOGWINSOCKTCPSTREAMPRIVATE_H
#define SOLIDSYSLOGWINSOCKTCPSTREAMPRIVATE_H

#include <stdint.h>

#include <winsock2.h>

#include "SolidSyslogError.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogStreamDefinition.h"
#include "SolidSyslogWinsockTcpStream.h"
#include "SolidSyslogWinsockTcpStreamErrors.h"

struct SolidSyslogWinsockTcpStream
{
    struct SolidSyslogStream Base;
    struct SolidSyslogWinsockTcpStreamConfig Config;
    SOCKET Fd;
};

void SolidSyslogWinsockTcpStream_Initialise(
    struct SolidSyslogStream* base,
    const struct SolidSyslogWinsockTcpStreamConfig* config
);
void SolidSyslogWinsockTcpStream_Cleanup(struct SolidSyslogStream* base);

static inline void WinsockTcpStream_Report(
    enum SolidSyslogSeverity severity,
    uint16_t category,
    enum SolidSyslogWinsockTcpStreamErrors code
)
{
    SolidSyslog_Error(severity, &SolidSyslogWinsockTcpStreamErrorSource, category, (int32_t) code);
}

#endif /* SOLIDSYSLOGWINSOCKTCPSTREAMPRIVATE_H */
