#include <stdint.h>

#include "SolidSyslogError.h"
#include "SolidSyslogLwipRawDatagramErrors.h"

static const char* LwipRawDatagramError_AsString(uint8_t code)
{
    static const char* const messages[LWIPRAWDATAGRAM_ERROR_MAX] = {
        [LWIPRAWDATAGRAM_ERROR_POOL_EXHAUSTED] =
            "SolidSyslogLwipRawDatagram_Create pool exhausted; returning fallback NullDatagram",
        [LWIPRAWDATAGRAM_ERROR_UNKNOWN_DESTROY] =
            "SolidSyslogLwipRawDatagram_Destroy called with a handle not issued by this pool",
    };
    const char* result = "unknown";
    if (code < (uint8_t) LWIPRAWDATAGRAM_ERROR_MAX)
    {
        enum SolidSyslogLwipRawDatagramErrors typed = (enum SolidSyslogLwipRawDatagramErrors) code;
        result = messages[typed];
    }
    return result;
}

const struct SolidSyslogErrorSource LwipRawDatagramErrorSource = {"LwipRawDatagram", LwipRawDatagramError_AsString};
