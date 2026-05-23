#ifndef SOLIDSYSLOGPOSIXDATAGRAMERRORS_H
#define SOLIDSYSLOGPOSIXDATAGRAMERRORS_H

#include "ExternC.h"
#include "SolidSyslogError.h"

EXTERN_C_BEGIN

    enum SolidSyslogPosixDatagramErrors
    {
        POSIXDATAGRAM_ERROR_POOL_EXHAUSTED,
        POSIXDATAGRAM_ERROR_UNKNOWN_DESTROY,
        POSIXDATAGRAM_ERROR_MAX
    };

    extern const struct SolidSyslogErrorSource PosixDatagramErrorSource;

EXTERN_C_END

#endif /* SOLIDSYSLOGPOSIXDATAGRAMERRORS_H */
