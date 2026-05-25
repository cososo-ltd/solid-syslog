#ifndef SOLIDSYSLOGFREERTOSTCPSTREAMERRORS_H
#define SOLIDSYSLOGFREERTOSTCPSTREAMERRORS_H

#include "ExternC.h"

EXTERN_C_BEGIN

    struct SolidSyslogErrorSource;

    enum SolidSyslogFreeRtosTcpStreamErrors
    {
        FREERTOSTCPSTREAM_ERROR_POOL_EXHAUSTED,
        FREERTOSTCPSTREAM_ERROR_UNKNOWN_DESTROY,
        FREERTOSTCPSTREAM_ERROR_MAX
    };

    extern const struct SolidSyslogErrorSource FreeRtosTcpStreamErrorSource;

EXTERN_C_END

#endif /* SOLIDSYSLOGFREERTOSTCPSTREAMERRORS_H */
