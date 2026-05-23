#include <stdint.h>

#include "SolidSyslogError.h"
#include "SolidSyslogGetAddrInfoResolverErrors.h"

static const char* GetAddrInfoResolverError_AsString(uint8_t code)
{
    static const char* const messages[GETADDRINFORESOLVER_ERROR_MAX] = {
        [GETADDRINFORESOLVER_ERROR_POOL_EXHAUSTED] =
            "SolidSyslogGetAddrInfoResolver_Create pool exhausted; returning fallback resolver",
        [GETADDRINFORESOLVER_ERROR_UNKNOWN_DESTROY] =
            "SolidSyslogGetAddrInfoResolver_Destroy called with a handle not issued by this pool",
    };
    const char* result = "unknown";
    if (code < (uint8_t) GETADDRINFORESOLVER_ERROR_MAX)
    {
        enum SolidSyslogGetAddrInfoResolverErrors typed = (enum SolidSyslogGetAddrInfoResolverErrors) code;
        result = messages[typed];
    }
    return result;
}

const struct SolidSyslogErrorSource GetAddrInfoResolverErrorSource = {
    "GetAddrInfoResolver",
    GetAddrInfoResolverError_AsString
};
