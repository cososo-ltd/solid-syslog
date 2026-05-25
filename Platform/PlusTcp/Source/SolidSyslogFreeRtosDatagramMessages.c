#include <stdint.h>

#include "SolidSyslogError.h"
#include "SolidSyslogFreeRtosDatagramErrors.h"

static const char* FreeRtosDatagramError_AsString(uint8_t code)
{
    static const char* const messages[FREERTOSDATAGRAM_ERROR_MAX] = {
        [FREERTOSDATAGRAM_ERROR_POOL_EXHAUSTED] =
            "SolidSyslogFreeRtosDatagram_Create pool exhausted; returning fallback datagram",
        [FREERTOSDATAGRAM_ERROR_UNKNOWN_DESTROY] =
            "SolidSyslogFreeRtosDatagram_Destroy called with a handle not issued by this pool",
    };
    const char* result = "unknown";
    if (code < (uint8_t) FREERTOSDATAGRAM_ERROR_MAX)
    {
        enum SolidSyslogFreeRtosDatagramErrors typed = (enum SolidSyslogFreeRtosDatagramErrors) code;
        result = messages[typed];
    }
    return result;
}

const struct SolidSyslogErrorSource FreeRtosDatagramErrorSource = {"FreeRtosDatagram", FreeRtosDatagramError_AsString};
