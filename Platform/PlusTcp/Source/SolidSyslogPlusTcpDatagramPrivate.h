#ifndef SOLIDSYSLOGPLUSTCPDATAGRAMPRIVATE_H
#define SOLIDSYSLOGPLUSTCPDATAGRAMPRIVATE_H

#include <stdint.h>
#include "SolidSyslogError.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogPlusTcpDatagramErrors.h"

#include "FreeRTOS.h"
#include "FreeRTOS_Sockets.h"

#include "SolidSyslogDatagramDefinition.h"

struct SolidSyslogPlusTcpDatagram
{
    struct SolidSyslogDatagram Base;
    Socket_t Socket;
};

void PlusTcpDatagram_Initialise(struct SolidSyslogDatagram* base);
void PlusTcpDatagram_Cleanup(struct SolidSyslogDatagram* base);

static inline void PlusTcpDatagram_Report(
    enum SolidSyslogSeverity severity,
    uint16_t category,
    enum SolidSyslogPlusTcpDatagramErrors code
)
{
    SolidSyslog_Error(severity, &PlusTcpDatagramErrorSource, category, code);
}

#endif /* SOLIDSYSLOGPLUSTCPDATAGRAMPRIVATE_H */
