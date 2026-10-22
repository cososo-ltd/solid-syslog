#include "BddTargetBlockFiles.h"

#include <stddef.h>
#include <stdio.h>

#include "SolidSyslogFile.h"
#include "SolidSyslogTunables.h"

void BddTargetBlockFiles_Print(FILE* out, struct SolidSyslogFile* file, const char* pathPrefix, size_t maxBlocks)
{
    (void) fprintf(out, "BLOCKS:");
    for (size_t index = 0U; index < maxBlocks; index++)
    {
        char path[SOLIDSYSLOG_MAX_PATH_SIZE];
        (void) snprintf(path, sizeof(path), "%s%02u.log", pathPrefix, (unsigned) index);
        if (SolidSyslogFile_Exists(file, path))
        {
            (void) fprintf(out, " %02u", (unsigned) index);
        }
    }
    (void) fprintf(out, "\n");
}
