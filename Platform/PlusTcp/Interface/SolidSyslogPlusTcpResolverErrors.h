#ifndef SOLIDSYSLOGPLUSTCPRESOLVERERRORS_H
#define SOLIDSYSLOGPLUSTCPRESOLVERERRORS_H

#include "ExternC.h"

EXTERN_C_BEGIN

    struct SolidSyslogErrorSource;

    enum SolidSyslogPlusTcpResolverErrors
    {
        PLUSTCPRESOLVER_ERROR_POOL_EXHAUSTED,
        PLUSTCPRESOLVER_ERROR_UNKNOWN_DESTROY,
        PLUSTCPRESOLVER_ERROR_MAX
    };

    extern const struct SolidSyslogErrorSource PlusTcpResolverErrorSource;

EXTERN_C_END

#endif /* SOLIDSYSLOGPLUSTCPRESOLVERERRORS_H */
