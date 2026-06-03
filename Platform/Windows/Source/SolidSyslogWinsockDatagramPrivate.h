#ifndef SOLIDSYSLOGWINSOCKDATAGRAMPRIVATE_H
#define SOLIDSYSLOGWINSOCKDATAGRAMPRIVATE_H

#include <stdint.h>

#include <stdbool.h>
#include <winsock2.h>

#include "SolidSyslogDatagramDefinition.h"
#include "SolidSyslogError.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogWinsockDatagramErrors.h"

struct SolidSyslogWinsockDatagram
{
    struct SolidSyslogDatagram Base;
    SOCKET Fd;
    bool Connected;
};

void WinsockDatagram_Initialise(struct SolidSyslogDatagram* base);
void WinsockDatagram_Cleanup(struct SolidSyslogDatagram* base);

static inline void WinsockDatagram_Report(
    enum SolidSyslogSeverity severity,
    uint16_t category,
    enum SolidSyslogWinsockDatagramErrors code
)
{
    SolidSyslog_Error(severity, &WinsockDatagramErrorSource, category, code);
}

#endif /* SOLIDSYSLOGWINSOCKDATAGRAMPRIVATE_H */
