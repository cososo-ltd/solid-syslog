#include <stdint.h>

#include "SolidSyslogError.h"
#include "SolidSyslogLwipRawResolverErrors.h"

static const char* LwipRawResolverError_AsString(uint8_t code)
{
    static const char* const messages[LWIPRAWRESOLVER_ERROR_MAX] = {
        [LWIPRAWRESOLVER_ERROR_POOL_EXHAUSTED] =
            "SolidSyslogLwipRawResolver_Create pool exhausted; returning fallback NullResolver",
        [LWIPRAWRESOLVER_ERROR_UNKNOWN_DESTROY] =
            "SolidSyslogLwipRawResolver_Destroy called with a handle not issued by this pool",
    };
    const char* result = "unknown";
    if (code < (uint8_t) LWIPRAWRESOLVER_ERROR_MAX)
    {
        enum SolidSyslogLwipRawResolverErrors typed = (enum SolidSyslogLwipRawResolverErrors) code;
        result = messages[typed];
    }
    return result;
}

const struct SolidSyslogErrorSource LwipRawResolverErrorSource = {"LwipRawResolver", LwipRawResolverError_AsString};
