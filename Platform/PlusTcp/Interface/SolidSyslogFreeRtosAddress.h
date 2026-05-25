#ifndef SOLIDSYSLOGFREERTOSADDRESS_H
#define SOLIDSYSLOGFREERTOSADDRESS_H

#include "ExternC.h"

EXTERN_C_BEGIN

    struct SolidSyslogAddress;

    struct SolidSyslogAddress* SolidSyslogFreeRtosAddress_Create(void);
    void SolidSyslogFreeRtosAddress_Destroy(struct SolidSyslogAddress * base);

EXTERN_C_END

#endif /* SOLIDSYSLOGFREERTOSADDRESS_H */
