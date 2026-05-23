#ifndef SOLIDSYSLOGTLSSTREAMERRORS_H
#define SOLIDSYSLOGTLSSTREAMERRORS_H

#include "ExternC.h"
#include "SolidSyslogError.h"

EXTERN_C_BEGIN

    enum SolidSyslogTlsStreamErrors
    {
        TLSSTREAM_ERROR_POOL_EXHAUSTED,
        TLSSTREAM_ERROR_UNKNOWN_DESTROY,
        TLSSTREAM_ERROR_MAX
    };

    extern const struct SolidSyslogErrorSource TlsStreamErrorSource;

EXTERN_C_END

#endif /* SOLIDSYSLOGTLSSTREAMERRORS_H */
