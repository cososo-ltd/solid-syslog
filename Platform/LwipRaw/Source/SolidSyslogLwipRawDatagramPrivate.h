#ifndef SOLIDSYSLOGLWIPRAWDATAGRAMPRIVATE_H
#define SOLIDSYSLOGLWIPRAWDATAGRAMPRIVATE_H

#include <stdint.h>

#include "SolidSyslogDatagramDefinition.h"
#include "SolidSyslogError.h"
#include "SolidSyslogLwipRawDatagramErrors.h"
#include "SolidSyslogPrival.h"

struct udp_pcb;

struct SolidSyslogLwipRawDatagram
{
    struct SolidSyslogDatagram Base;
    struct udp_pcb* Pcb;
};

void LwipRawDatagram_Initialise(struct SolidSyslogDatagram* base);
void LwipRawDatagram_Cleanup(struct SolidSyslogDatagram* base);

static inline void LwipRawDatagram_Report(
    enum SolidSyslogSeverity severity,
    uint16_t category,
    enum SolidSyslogLwipRawDatagramErrors code
)
{
    SolidSyslog_Error(severity, &LwipRawDatagramErrorSource, category, code);
}

#endif /* SOLIDSYSLOGLWIPRAWDATAGRAMPRIVATE_H */
