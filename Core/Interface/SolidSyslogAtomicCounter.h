#ifndef SOLIDSYSLOGATOMICCOUNTER_H
#define SOLIDSYSLOGATOMICCOUNTER_H

#include "ExternC.h"

#include <stdint.h>

EXTERN_C_BEGIN

    enum
    {
        SOLIDSYSLOG_SEQUENCE_ID_MAX =
            2147483647U /* RFC 5424 §7.3.1: values in [1, 2^31 - 1], wraps to 1 on overflow. */
    };

    struct SolidSyslogAtomicCounter;

    uint32_t SolidSyslogAtomicCounter_Increment(struct SolidSyslogAtomicCounter * base);

EXTERN_C_END

#endif /* SOLIDSYSLOGATOMICCOUNTER_H */
