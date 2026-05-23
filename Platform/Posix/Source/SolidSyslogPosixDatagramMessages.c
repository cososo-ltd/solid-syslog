#include <stdint.h>

#include "SolidSyslogError.h"
#include "SolidSyslogPosixDatagramErrors.h"

static const char* PosixDatagramError_AsString(uint8_t code)
{
    static const char* const messages[POSIXDATAGRAM_ERROR_MAX] = {
        [POSIXDATAGRAM_ERROR_POOL_EXHAUSTED] =
            "SolidSyslogPosixDatagram_Create pool exhausted; returning fallback datagram",
        [POSIXDATAGRAM_ERROR_UNKNOWN_DESTROY] =
            "SolidSyslogPosixDatagram_Destroy called with a handle not issued by this pool",
    };
    const char* result = "unknown";
    if (code < (uint8_t) POSIXDATAGRAM_ERROR_MAX)
    {
        enum SolidSyslogPosixDatagramErrors typed = (enum SolidSyslogPosixDatagramErrors) code;
        result = messages[typed];
    }
    return result;
}

const struct SolidSyslogErrorSource PosixDatagramErrorSource = {"PosixDatagram", PosixDatagramError_AsString};
