#include "TempFile.h"

#include <stdio.h>

FILE* TempFile_Open(void)
{
    FILE* file = NULL;
    if (tmpfile_s(&file) != 0)
    {
        file = NULL;
    }
    return file;
}

void TempFile_Close(FILE* file)
{
    (void) fclose(file);
}
