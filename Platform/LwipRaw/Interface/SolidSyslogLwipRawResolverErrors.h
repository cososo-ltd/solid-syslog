#ifndef SOLIDSYSLOGLWIPRAWRESOLVERERRORS_H
#define SOLIDSYSLOGLWIPRAWRESOLVERERRORS_H

#include "ExternC.h"

EXTERN_C_BEGIN

    struct SolidSyslogErrorSource;

    enum SolidSyslogLwipRawResolverErrors
    {
        LWIPRAWRESOLVER_ERROR_POOL_EXHAUSTED,
        LWIPRAWRESOLVER_ERROR_UNKNOWN_DESTROY,
        LWIPRAWRESOLVER_ERROR_MAX
    };

    extern const struct SolidSyslogErrorSource LwipRawResolverErrorSource;

EXTERN_C_END

#endif /* SOLIDSYSLOGLWIPRAWRESOLVERERRORS_H */
