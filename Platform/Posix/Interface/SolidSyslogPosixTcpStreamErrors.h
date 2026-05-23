#ifndef SOLIDSYSLOGPOSIXTCPSTREAMERRORS_H
#define SOLIDSYSLOGPOSIXTCPSTREAMERRORS_H

#include "ExternC.h"
#include "SolidSyslogError.h"

EXTERN_C_BEGIN

    enum SolidSyslogPosixTcpStreamErrors
    {
        POSIXTCPSTREAM_ERROR_POOL_EXHAUSTED,
        POSIXTCPSTREAM_ERROR_UNKNOWN_DESTROY,
        POSIXTCPSTREAM_ERROR_MAX
    };

    extern const struct SolidSyslogErrorSource PosixTcpStreamErrorSource;

EXTERN_C_END

#endif /* SOLIDSYSLOGPOSIXTCPSTREAMERRORS_H */
