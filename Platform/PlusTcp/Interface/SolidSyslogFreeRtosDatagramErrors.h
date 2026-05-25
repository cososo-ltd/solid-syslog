#ifndef SOLIDSYSLOGFREERTOSDATAGRAMERRORS_H
#define SOLIDSYSLOGFREERTOSDATAGRAMERRORS_H

#include "ExternC.h"

EXTERN_C_BEGIN

    struct SolidSyslogErrorSource;

    enum SolidSyslogFreeRtosDatagramErrors
    {
        FREERTOSDATAGRAM_ERROR_POOL_EXHAUSTED,
        FREERTOSDATAGRAM_ERROR_UNKNOWN_DESTROY,
        FREERTOSDATAGRAM_ERROR_MAX
    };

    extern const struct SolidSyslogErrorSource FreeRtosDatagramErrorSource;

EXTERN_C_END

#endif /* SOLIDSYSLOGFREERTOSDATAGRAMERRORS_H */
