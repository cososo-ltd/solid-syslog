#ifndef SOLIDSYSLOGLWIPRAWDATAGRAMERRORS_H
#define SOLIDSYSLOGLWIPRAWDATAGRAMERRORS_H

#include "ExternC.h"

EXTERN_C_BEGIN

    struct SolidSyslogErrorSource;

    enum SolidSyslogLwipRawDatagramErrors
    {
        LWIPRAWDATAGRAM_ERROR_POOL_EXHAUSTED,
        LWIPRAWDATAGRAM_ERROR_UNKNOWN_DESTROY,
        LWIPRAWDATAGRAM_ERROR_MAX
    };

    extern const struct SolidSyslogErrorSource LwipRawDatagramErrorSource;

EXTERN_C_END

#endif /* SOLIDSYSLOGLWIPRAWDATAGRAMERRORS_H */
