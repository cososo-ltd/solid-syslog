#include "TempFile.h"

#include <stdio.h>

FILE* TempFile_Open(void)
{
    return tmpfile();
}

void TempFile_Close(FILE* file)
{
    (void) fclose(file);
}
