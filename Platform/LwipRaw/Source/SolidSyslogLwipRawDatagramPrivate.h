#ifndef SOLIDSYSLOGLWIPRAWDATAGRAMPRIVATE_H
#define SOLIDSYSLOGLWIPRAWDATAGRAMPRIVATE_H

#include "SolidSyslogDatagramDefinition.h"

struct udp_pcb;

struct SolidSyslogLwipRawDatagram
{
    struct SolidSyslogDatagram Base;
    struct udp_pcb* Pcb;
};

void LwipRawDatagram_Initialise(struct SolidSyslogDatagram* base);
void LwipRawDatagram_Cleanup(struct SolidSyslogDatagram* base);

#endif /* SOLIDSYSLOGLWIPRAWDATAGRAMPRIVATE_H */
