#ifndef SOLIDSYSLOGLWIPRAWDATAGRAM_H
#define SOLIDSYSLOGLWIPRAWDATAGRAM_H

#include "ExternC.h"

EXTERN_C_BEGIN

    struct SolidSyslogDatagram;

    struct SolidSyslogDatagram* SolidSyslogLwipRawDatagram_Create(void);
    void SolidSyslogLwipRawDatagram_Destroy(struct SolidSyslogDatagram * base);

EXTERN_C_END

#endif /* SOLIDSYSLOGLWIPRAWDATAGRAM_H */
