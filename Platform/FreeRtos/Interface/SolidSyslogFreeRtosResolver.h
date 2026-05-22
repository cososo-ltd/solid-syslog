#ifndef SOLIDSYSLOGFREERTOSRESOLVER_H
#define SOLIDSYSLOGFREERTOSRESOLVER_H

#include "ExternC.h"

EXTERN_C_BEGIN

    struct SolidSyslogResolver;

    struct SolidSyslogResolver* SolidSyslogFreeRtosResolver_Create(void);
    void SolidSyslogFreeRtosResolver_Destroy(struct SolidSyslogResolver * base);

EXTERN_C_END

#endif /* SOLIDSYSLOGFREERTOSRESOLVER_H */
