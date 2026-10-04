#include "TempFile.h"

#include <stdio.h>

FILE* TempFile_Open(void)
{
    return tmpfile();
}
