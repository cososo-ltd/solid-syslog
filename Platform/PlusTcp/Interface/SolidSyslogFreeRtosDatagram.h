#ifndef SOLIDSYSLOGFREERTOSDATAGRAM_H
#define SOLIDSYSLOGFREERTOSDATAGRAM_H

#include "ExternC.h"

EXTERN_C_BEGIN

    struct SolidSyslogDatagram;

    struct SolidSyslogDatagram* SolidSyslogFreeRtosDatagram_Create(void);
    void SolidSyslogFreeRtosDatagram_Destroy(struct SolidSyslogDatagram * base);

EXTERN_C_END

#endif /* SOLIDSYSLOGFREERTOSDATAGRAM_H */
