#include <stdint.h>

#include "SolidSyslogError.h"
#include "SolidSyslogStdAtomicCounterErrors.h"

static const char* StdAtomicCounterError_AsString(uint8_t code)
{
    static const char* const messages[STDATOMICCOUNTER_ERROR_MAX] = {
        [STDATOMICCOUNTER_ERROR_POOL_EXHAUSTED] =
            "SolidSyslogStdAtomicCounter_Create pool exhausted; returning fallback counter",
        [STDATOMICCOUNTER_ERROR_UNKNOWN_DESTROY] =
            "SolidSyslogStdAtomicCounter_Destroy called with a handle not issued by this pool",
    };
    const char* result = "unknown";
    if (code < (uint8_t) STDATOMICCOUNTER_ERROR_MAX)
    {
        enum SolidSyslogStdAtomicCounterErrors typed = (enum SolidSyslogStdAtomicCounterErrors) code;
        result = messages[typed];
    }
    return result;
}

const struct SolidSyslogErrorSource StdAtomicCounterErrorSource = {"StdAtomicCounter", StdAtomicCounterError_AsString};
