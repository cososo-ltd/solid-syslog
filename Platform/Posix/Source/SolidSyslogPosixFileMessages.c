#include <stdint.h>

#include "SolidSyslogError.h"
#include "SolidSyslogPosixFileErrors.h"

static const char* PosixFileError_AsString(uint8_t code)
{
    static const char* const messages[POSIXFILE_ERROR_MAX] = {
        [POSIXFILE_ERROR_POOL_EXHAUSTED] = "SolidSyslogPosixFile_Create pool exhausted; returning fallback file",
        [POSIXFILE_ERROR_UNKNOWN_DESTROY] = "SolidSyslogPosixFile_Destroy called with a handle not issued by this pool",
    };
    const char* result = "unknown";
    if (code < (uint8_t) POSIXFILE_ERROR_MAX)
    {
        enum SolidSyslogPosixFileErrors typed = (enum SolidSyslogPosixFileErrors) code;
        result = messages[typed];
    }
    return result;
}

const struct SolidSyslogErrorSource PosixFileErrorSource = {"PosixFile", PosixFileError_AsString};
