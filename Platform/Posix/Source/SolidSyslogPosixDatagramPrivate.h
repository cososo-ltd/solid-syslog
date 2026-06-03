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

void PosixDatagram_Initialise(struct SolidSyslogDatagram* base);
void PosixDatagram_Cleanup(struct SolidSyslogDatagram* base);

static inline void PosixDatagram_Report(
    enum SolidSyslogSeverity severity,
    uint16_t category,
    enum SolidSyslogPosixDatagramErrors code
)
{
    SolidSyslog_Error(severity, &PosixDatagramErrorSource, category, code);
}

#endif /* SOLIDSYSLOGPOSIXDATAGRAMPRIVATE_H */
