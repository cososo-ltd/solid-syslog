#ifndef SOLIDSYSLOGPOSIXADDRESS_H
#define SOLIDSYSLOGPOSIXADDRESS_H

#include "ExternC.h"

EXTERN_C_BEGIN

    struct SolidSyslogAddress;

    struct SolidSyslogAddress* SolidSyslogPosixAddress_Create(void);
    void SolidSyslogPosixAddress_Destroy(struct SolidSyslogAddress * base);

EXTERN_C_END

#endif /* SOLIDSYSLOGPOSIXADDRESS_H */
