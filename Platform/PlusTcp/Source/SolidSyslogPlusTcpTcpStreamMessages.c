#include <stdint.h>

#include "SolidSyslogError.h"
#include "SolidSyslogPlusTcpTcpStreamErrors.h"

static const char* PlusTcpTcpStreamError_AsString(uint8_t code)
{
    static const char* const messages[PLUSTCPTCPSTREAM_ERROR_MAX] = {
        [PLUSTCPTCPSTREAM_ERROR_POOL_EXHAUSTED] =
            "SolidSyslogPlusTcpTcpStream_Create pool exhausted; returning fallback stream",
        [PLUSTCPTCPSTREAM_ERROR_UNKNOWN_DESTROY] =
            "SolidSyslogPlusTcpTcpStream_Destroy called with a handle not issued by this pool",
    };
    const char* result = "unknown";
    if (code < (uint8_t) PLUSTCPTCPSTREAM_ERROR_MAX)
    {
        enum SolidSyslogPlusTcpTcpStreamErrors typed = (enum SolidSyslogPlusTcpTcpStreamErrors) code;
        result = messages[typed];
    }
    return result;
}

const struct SolidSyslogErrorSource PlusTcpTcpStreamErrorSource = {"PlusTcpTcpStream", PlusTcpTcpStreamError_AsString};
