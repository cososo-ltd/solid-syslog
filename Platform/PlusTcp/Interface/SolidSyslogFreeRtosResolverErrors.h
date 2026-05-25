#ifndef SOLIDSYSLOGFREERTOSRESOLVERERRORS_H
#define SOLIDSYSLOGFREERTOSRESOLVERERRORS_H

#include "ExternC.h"

EXTERN_C_BEGIN

    struct SolidSyslogErrorSource;

    enum SolidSyslogFreeRtosResolverErrors
    {
        FREERTOSRESOLVER_ERROR_POOL_EXHAUSTED,
        FREERTOSRESOLVER_ERROR_UNKNOWN_DESTROY,
        FREERTOSRESOLVER_ERROR_MAX
    };

    extern const struct SolidSyslogErrorSource FreeRtosResolverErrorSource;

EXTERN_C_END

#endif /* SOLIDSYSLOGFREERTOSRESOLVERERRORS_H */
