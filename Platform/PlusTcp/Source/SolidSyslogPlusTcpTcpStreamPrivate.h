/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#ifndef SOLIDSYSLOGPLUSTCPTCPSTREAMPRIVATE_H
#define SOLIDSYSLOGPLUSTCPTCPSTREAMPRIVATE_H

#include <stdint.h>
#include "SolidSyslogError.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogPlusTcpTcpStreamErrors.h"

#include "FreeRTOS.h"
#include "FreeRTOS_Sockets.h"

#include "SolidSyslogPlusTcpTcpStream.h"
#include "SolidSyslogStreamDefinition.h"

struct SolidSyslogPlusTcpTcpStream
{
    struct SolidSyslogStream Base;
    struct SolidSyslogPlusTcpTcpStreamConfig Config;
    Socket_t Socket;
};

void SolidSyslogPlusTcpTcpStream_Initialise(
    struct SolidSyslogStream* base,
    const struct SolidSyslogPlusTcpTcpStreamConfig* config
);
void SolidSyslogPlusTcpTcpStream_Cleanup(struct SolidSyslogStream* base);

static inline void PlusTcpTcpStream_Report(
    enum SolidSyslogSeverity severity,
    uint16_t category,
    enum SolidSyslogPlusTcpTcpStreamErrors code
)
{
    SolidSyslog_Error(severity, &PlusTcpTcpStreamErrorSource, category, (int32_t) code);
}

#endif /* SOLIDSYSLOGPLUSTCPTCPSTREAMPRIVATE_H */
