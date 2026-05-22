#ifndef SOLIDSYSLOGWINSOCKADDRESS_H
#define SOLIDSYSLOGWINSOCKADDRESS_H

#include "ExternC.h"

EXTERN_C_BEGIN

    struct SolidSyslogAddress;

    struct SolidSyslogAddress* SolidSyslogWinsockAddress_Create(void);
    void SolidSyslogWinsockAddress_Destroy(struct SolidSyslogAddress * base);

EXTERN_C_END

#endif /* SOLIDSYSLOGWINSOCKADDRESS_H */
