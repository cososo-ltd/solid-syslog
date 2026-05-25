#include <stdint.h>

#include "SolidSyslogError.h"
#include "SolidSyslogPlusTcpResolverErrors.h"

static const char* PlusTcpResolverError_AsString(uint8_t code)
{
    static const char* const messages[PLUSTCPRESOLVER_ERROR_MAX] = {
        [PLUSTCPRESOLVER_ERROR_POOL_EXHAUSTED] =
            "SolidSyslogPlusTcpResolver_Create pool exhausted; returning fallback resolver",
        [PLUSTCPRESOLVER_ERROR_UNKNOWN_DESTROY] =
            "SolidSyslogPlusTcpResolver_Destroy called with a handle not issued by this pool",
    };
    const char* result = "unknown";
    if (code < (uint8_t) PLUSTCPRESOLVER_ERROR_MAX)
    {
        enum SolidSyslogPlusTcpResolverErrors typed = (enum SolidSyslogPlusTcpResolverErrors) code;
        result = messages[typed];
    }
    return result;
}

const struct SolidSyslogErrorSource PlusTcpResolverErrorSource = {"PlusTcpResolver", PlusTcpResolverError_AsString};
