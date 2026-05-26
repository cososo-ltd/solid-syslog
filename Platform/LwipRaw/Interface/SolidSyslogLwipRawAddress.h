#ifndef SOLIDSYSLOGLWIPRAWADDRESS_H
#define SOLIDSYSLOGLWIPRAWADDRESS_H

#include "ExternC.h"

EXTERN_C_BEGIN

    struct SolidSyslogAddress;

    struct SolidSyslogAddress* SolidSyslogLwipRawAddress_Create(void);
    void SolidSyslogLwipRawAddress_Destroy(struct SolidSyslogAddress * base);

EXTERN_C_END

#endif /* SOLIDSYSLOGLWIPRAWADDRESS_H */
