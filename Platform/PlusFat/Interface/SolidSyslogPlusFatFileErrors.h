#ifndef SOLIDSYSLOGPLUSFATFILEERRORS_H
#define SOLIDSYSLOGPLUSFATFILEERRORS_H

#include "ExternC.h"

EXTERN_C_BEGIN

    struct SolidSyslogErrorSource;

    enum SolidSyslogPlusFatFileErrors
    {
        PLUSFATFILE_ERROR_POOL_EXHAUSTED,
        PLUSFATFILE_ERROR_UNKNOWN_DESTROY,
        PLUSFATFILE_ERROR_MAX
    };

    extern const struct SolidSyslogErrorSource PlusFatFileErrorSource;

EXTERN_C_END

#endif /* SOLIDSYSLOGPLUSFATFILEERRORS_H */
