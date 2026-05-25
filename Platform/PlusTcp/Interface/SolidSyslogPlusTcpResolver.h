#ifndef SOLIDSYSLOGPLUSTCPRESOLVER_H
#define SOLIDSYSLOGPLUSTCPRESOLVER_H

#include "ExternC.h"

EXTERN_C_BEGIN

    struct SolidSyslogResolver;

    struct SolidSyslogResolver* SolidSyslogPlusTcpResolver_Create(void);
    void SolidSyslogPlusTcpResolver_Destroy(struct SolidSyslogResolver * base);

EXTERN_C_END

#endif /* SOLIDSYSLOGPLUSTCPRESOLVER_H */
