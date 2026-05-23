#include <stdint.h>

#include "SolidSyslogError.h"
#include "SolidSyslogPosixMutexErrors.h"

static const char* PosixMutexError_AsString(uint8_t code)
{
    static const char* const messages[POSIXMUTEX_ERROR_MAX] = {
        [POSIXMUTEX_ERROR_POOL_EXHAUSTED] = "SolidSyslogPosixMutex_Create pool exhausted; returning fallback mutex",
        [POSIXMUTEX_ERROR_UNKNOWN_DESTROY] =
            "SolidSyslogPosixMutex_Destroy called with a handle not issued by this pool",
    };
    const char* result = "unknown";
    if (code < (uint8_t) POSIXMUTEX_ERROR_MAX)
    {
        enum SolidSyslogPosixMutexErrors typed = (enum SolidSyslogPosixMutexErrors) code;
        result = messages[typed];
    }
    return result;
}

const struct SolidSyslogErrorSource PosixMutexErrorSource = {"PosixMutex", PosixMutexError_AsString};
