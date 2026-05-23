#include <stdint.h>

#include "SolidSyslogError.h"
#include "SolidSyslogPosixTcpStreamErrors.h"

static const char* PosixTcpStreamError_AsString(uint8_t code)
{
    static const char* const messages[POSIXTCPSTREAM_ERROR_MAX] = {
        [POSIXTCPSTREAM_ERROR_POOL_EXHAUSTED] =
            "SolidSyslogPosixTcpStream_Create pool exhausted; returning fallback stream",
        [POSIXTCPSTREAM_ERROR_UNKNOWN_DESTROY] =
            "SolidSyslogPosixTcpStream_Destroy called with a handle not issued by this pool",
    };
    const char* result = "unknown";
    if (code < (uint8_t) POSIXTCPSTREAM_ERROR_MAX)
    {
        enum SolidSyslogPosixTcpStreamErrors typed = (enum SolidSyslogPosixTcpStreamErrors) code;
        result = messages[typed];
    }
    return result;
}

const struct SolidSyslogErrorSource PosixTcpStreamErrorSource = {"PosixTcpStream", PosixTcpStreamError_AsString};
