#ifndef SOLIDSYSLOGFREERTOSDATAGRAMPRIVATE_H
#define SOLIDSYSLOGFREERTOSDATAGRAMPRIVATE_H

#include "FreeRTOS.h"
#include "FreeRTOS_Sockets.h"

#include "SolidSyslogDatagramDefinition.h"

struct SolidSyslogFreeRtosDatagram
{
    struct SolidSyslogDatagram Base;
    Socket_t Socket;
};

void FreeRtosDatagram_Initialise(struct SolidSyslogDatagram* base);
void FreeRtosDatagram_Cleanup(struct SolidSyslogDatagram* base);

#endif /* SOLIDSYSLOGFREERTOSDATAGRAMPRIVATE_H */
