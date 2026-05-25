#include <stdint.h>

#include "SolidSyslogError.h"
#include "SolidSyslogFreeRtosTcpStreamErrors.h"

static const char* FreeRtosTcpStreamError_AsString(uint8_t code)
{
    static const char* const messages[FREERTOSTCPSTREAM_ERROR_MAX] = {
        [FREERTOSTCPSTREAM_ERROR_POOL_EXHAUSTED] =
            "SolidSyslogFreeRtosTcpStream_Create pool exhausted; returning fallback stream",
        [FREERTOSTCPSTREAM_ERROR_UNKNOWN_DESTROY] =
            "SolidSyslogFreeRtosTcpStream_Destroy called with a handle not issued by this pool",
    };
    const char* result = "unknown";
    if (code < (uint8_t) FREERTOSTCPSTREAM_ERROR_MAX)
    {
        enum SolidSyslogFreeRtosTcpStreamErrors typed = (enum SolidSyslogFreeRtosTcpStreamErrors) code;
        result = messages[typed];
    }
    return result;
}

const struct SolidSyslogErrorSource FreeRtosTcpStreamErrorSource = {
    "FreeRtosTcpStream",
    FreeRtosTcpStreamError_AsString
};
