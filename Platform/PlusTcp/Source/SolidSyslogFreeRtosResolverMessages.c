#include <stdint.h>

#include "SolidSyslogError.h"
#include "SolidSyslogFreeRtosResolverErrors.h"

static const char* FreeRtosResolverError_AsString(uint8_t code)
{
    static const char* const messages[FREERTOSRESOLVER_ERROR_MAX] = {
        [FREERTOSRESOLVER_ERROR_POOL_EXHAUSTED] =
            "SolidSyslogFreeRtosResolver_Create pool exhausted; returning fallback resolver",
        [FREERTOSRESOLVER_ERROR_UNKNOWN_DESTROY] =
            "SolidSyslogFreeRtosResolver_Destroy called with a handle not issued by this pool",
    };
    const char* result = "unknown";
    if (code < (uint8_t) FREERTOSRESOLVER_ERROR_MAX)
    {
        enum SolidSyslogFreeRtosResolverErrors typed = (enum SolidSyslogFreeRtosResolverErrors) code;
        result = messages[typed];
    }
    return result;
}

const struct SolidSyslogErrorSource FreeRtosResolverErrorSource = {"FreeRtosResolver", FreeRtosResolverError_AsString};
