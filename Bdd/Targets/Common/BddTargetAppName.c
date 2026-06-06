#include "BddTargetAppName.h"
#include "SolidSyslogHeaderField.h"

#include <ctype.h>
#include <stdbool.h>
#include <string.h>

/* Length of ".exe" — used to recognise and strip the Windows executable
   extension so the syslog app name omits it regardless of build platform. */
enum
{
    EXE_SUFFIX_LENGTH = 4
};

static const char* appName;
static size_t appNameLength;

static bool EndsWithDotExe(const char* name, size_t length)
{
    if (length < EXE_SUFFIX_LENGTH)
    {
        return false;
    }

    const char* suffix = name + length - EXE_SUFFIX_LENGTH;
    return (suffix[0] == '.') && (tolower((unsigned char) suffix[1]) == 'e') &&
           (tolower((unsigned char) suffix[2]) == 'x') && (tolower((unsigned char) suffix[3]) == 'e');
}

void BddTargetAppName_Set(const char* argv0)
{
    /* Avoid relational compare on a NULL pointer (UB in ISO C) by handling
       NULL separators explicitly before picking the rightmost. */
    const char* forwardSlash = strrchr(argv0, '/');
    const char* backSlash = strrchr(argv0, '\\');
    const char* separator = NULL;

    if (forwardSlash == NULL)
    {
        separator = backSlash;
    }
    else if (backSlash == NULL)
    {
        separator = forwardSlash;
    }
    else
    {
        separator = (forwardSlash > backSlash) ? forwardSlash : backSlash;
    }

    appName = (separator != NULL) ? separator + 1 : argv0;
    appNameLength = strlen(appName);

    if (EndsWithDotExe(appName, appNameLength))
    {
        appNameLength -= EXE_SUFFIX_LENGTH;
    }
}

void BddTargetAppName_Get(struct SolidSyslogHeaderField* field, void* context)
{
    (void) context;
    SolidSyslogHeaderField_PrintUsAscii(field, appName, appNameLength);
}
