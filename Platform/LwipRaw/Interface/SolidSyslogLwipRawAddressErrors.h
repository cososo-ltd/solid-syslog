#ifndef SOLIDSYSLOGLWIPRAWADDRESSERRORS_H
#define SOLIDSYSLOGLWIPRAWADDRESSERRORS_H

#include "ExternC.h"

EXTERN_C_BEGIN

    struct SolidSyslogErrorSource;

    enum SolidSyslogLwipRawAddressErrors
    {
        LWIPRAWADDRESS_ERROR_POOL_EXHAUSTED,
        LWIPRAWADDRESS_ERROR_UNKNOWN_DESTROY,
        LWIPRAWADDRESS_ERROR_MAX
    };

    extern const struct SolidSyslogErrorSource LwipRawAddressErrorSource;

EXTERN_C_END

#endif /* SOLIDSYSLOGLWIPRAWADDRESSERRORS_H */
