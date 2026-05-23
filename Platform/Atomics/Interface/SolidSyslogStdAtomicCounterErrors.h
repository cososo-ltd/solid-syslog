#ifndef SOLIDSYSLOGSTDATOMICCOUNTERERRORS_H
#define SOLIDSYSLOGSTDATOMICCOUNTERERRORS_H

#include "ExternC.h"
#include "SolidSyslogError.h"

EXTERN_C_BEGIN

    enum SolidSyslogStdAtomicCounterErrors
    {
        STDATOMICCOUNTER_ERROR_POOL_EXHAUSTED,
        STDATOMICCOUNTER_ERROR_UNKNOWN_DESTROY,
        STDATOMICCOUNTER_ERROR_MAX
    };

    extern const struct SolidSyslogErrorSource StdAtomicCounterErrorSource;

EXTERN_C_END

#endif /* SOLIDSYSLOGSTDATOMICCOUNTERERRORS_H */
