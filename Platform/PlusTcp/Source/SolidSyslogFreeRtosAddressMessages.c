#include <stdint.h>

#include "SolidSyslogError.h"
#include "SolidSyslogFreeRtosAddressErrors.h"

static const char* FreeRtosAddressError_AsString(uint8_t code)
{
    static const char* const messages[FREERTOSADDRESS_ERROR_MAX] = {
        [FREERTOSADDRESS_ERROR_POOL_EXHAUSTED] =
            "SolidSyslogFreeRtosAddress_Create pool exhausted; returning fallback address",
        [FREERTOSADDRESS_ERROR_UNKNOWN_DESTROY] =
            "SolidSyslogFreeRtosAddress_Destroy called with a handle not issued by this pool",
    };
    const char* result = "unknown";
    if (code < (uint8_t) FREERTOSADDRESS_ERROR_MAX)
    {
        enum SolidSyslogFreeRtosAddressErrors typed = (enum SolidSyslogFreeRtosAddressErrors) code;
        result = messages[typed];
    }
    return result;
}

const struct SolidSyslogErrorSource FreeRtosAddressErrorSource = {"FreeRtosAddress", FreeRtosAddressError_AsString};
