#include <stdint.h>

#include "SolidSyslogError.h"
#include "SolidSyslogLwipRawAddressErrors.h"

static const char* LwipRawAddressError_AsString(uint8_t code)
{
    static const char* const messages[LWIPRAWADDRESS_ERROR_MAX] = {
        [LWIPRAWADDRESS_ERROR_POOL_EXHAUSTED] =
            "SolidSyslogLwipRawAddress_Create pool exhausted; returning fallback address",
        [LWIPRAWADDRESS_ERROR_UNKNOWN_DESTROY] =
            "SolidSyslogLwipRawAddress_Destroy called with a handle not issued by this pool",
    };
    const char* result = "unknown";
    if (code < (uint8_t) LWIPRAWADDRESS_ERROR_MAX)
    {
        enum SolidSyslogLwipRawAddressErrors typed = (enum SolidSyslogLwipRawAddressErrors) code;
        result = messages[typed];
    }
    return result;
}

const struct SolidSyslogErrorSource LwipRawAddressErrorSource = {"LwipRawAddress", LwipRawAddressError_AsString};
