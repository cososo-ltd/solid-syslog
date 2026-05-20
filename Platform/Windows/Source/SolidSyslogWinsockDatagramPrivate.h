#ifndef SOLIDSYSLOGWINSOCKDATAGRAMPRIVATE_H
#define SOLIDSYSLOGWINSOCKDATAGRAMPRIVATE_H

#include <stdbool.h>
#include <winsock2.h>

#include "SolidSyslogDatagramDefinition.h"

struct SolidSyslogWinsockDatagram
{
    struct SolidSyslogDatagram Base;
    SOCKET Fd;
    bool Connected;
};

void WinsockDatagram_Initialise(struct SolidSyslogDatagram* base);
void WinsockDatagram_Cleanup(struct SolidSyslogDatagram* base);

#endif /* SOLIDSYSLOGWINSOCKDATAGRAMPRIVATE_H */
