#include "SafeString.h"

#include <string.h>

void SafeString_Copy(char* dest, size_t destSize, const char* src)
{
    if (destSize == 0)
    {
        return;
    }
    strncpy(dest, src, destSize - 1);
    dest[destSize - 1] = '\0';
}
