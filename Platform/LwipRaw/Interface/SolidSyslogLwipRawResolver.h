#ifndef SOLIDSYSLOGLWIPRAWRESOLVER_H
#define SOLIDSYSLOGLWIPRAWRESOLVER_H

#include "ExternC.h"

EXTERN_C_BEGIN

    struct SolidSyslogResolver;

    struct SolidSyslogResolver* SolidSyslogLwipRawResolver_Create(void);
    void SolidSyslogLwipRawResolver_Destroy(struct SolidSyslogResolver * base);

EXTERN_C_END

#endif /* SOLIDSYSLOGLWIPRAWRESOLVER_H */
