#ifndef SOLIDSYSLOGPOSIXMUTEXERRORS_H
#define SOLIDSYSLOGPOSIXMUTEXERRORS_H

#include "ExternC.h"
#include "SolidSyslogError.h"

EXTERN_C_BEGIN

    enum SolidSyslogPosixMutexErrors
    {
        POSIXMUTEX_ERROR_POOL_EXHAUSTED,
        POSIXMUTEX_ERROR_UNKNOWN_DESTROY,
        POSIXMUTEX_ERROR_MAX
    };

    extern const struct SolidSyslogErrorSource PosixMutexErrorSource;

EXTERN_C_END

#endif /* SOLIDSYSLOGPOSIXMUTEXERRORS_H */
