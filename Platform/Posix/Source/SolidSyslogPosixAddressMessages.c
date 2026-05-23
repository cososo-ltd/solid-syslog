#include <stdint.h>

#include "SolidSyslogError.h"
#include "SolidSyslogPosixAddressErrors.h"

static const char* PosixAddressError_AsString(uint8_t code)
{
    static const char* const messages[POSIXADDRESS_ERROR_MAX] = {
        [POSIXADDRESS_ERROR_POOL_EXHAUSTED] =
            "SolidSyslogPosixAddress_Create pool exhausted; returning fallback address",
        [POSIXADDRESS_ERROR_UNKNOWN_DESTROY] =
            "SolidSyslogPosixAddress_Destroy called with a handle not issued by this pool",
    };
    const char* result = "unknown";
    if (code < (uint8_t) POSIXADDRESS_ERROR_MAX)
    {
        enum SolidSyslogPosixAddressErrors typed = (enum SolidSyslogPosixAddressErrors) code;
        result = messages[typed];
    }
    return result;
}

const struct SolidSyslogErrorSource PosixAddressErrorSource = {"PosixAddress", PosixAddressError_AsString};
