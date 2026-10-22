#include "BddTargetBlockFiles.h"

#include <stddef.h>
#include <stdio.h>

void BddTargetBlockFiles_Print(FILE* out, struct SolidSyslogFile* file, const char* pathPrefix, size_t maxBlocks)
{
    (void) file;
    (void) pathPrefix;
    (void) maxBlocks;
    (void) fprintf(out, "BLOCKS:\n");
}
