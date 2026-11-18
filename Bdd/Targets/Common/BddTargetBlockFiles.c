#include "BddTargetBlockFiles.h"

#include <stddef.h>
#include <stdio.h>

#include "SolidSyslogBlockDevice.h"

void BddTargetBlockFiles_Print(FILE* out, struct SolidSyslogBlockDevice* device)
{
    size_t blockCount = SolidSyslogBlockDevice_GetBlockCount(device);

    (void) fprintf(out, "BLOCKS:");
    for (size_t index = 0U; index < blockCount; index++)
    {
        if (SolidSyslogBlockDevice_Exists(device, index))
        {
            (void) fprintf(out, " %02u", (unsigned) index);
        }
    }
    (void) fprintf(out, "\n");
}
