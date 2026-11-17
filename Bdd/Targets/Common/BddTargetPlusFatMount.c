/* FreeRTOS-Plus-FAT implementation of the shared pipeline's FS-mount seam - see
 * BddTargetPlusFatMount.h. */

#include "BddTargetPlusFatMount.h"

#include "BddTargetBoardDisk.h"
#include "BddTargetPlusFatDisk.h"
#include "SolidSyslogPlusFatFile.h"

bool BddTargetPlusFatMount_Mount(void)
{
    return BddTargetPlusFatDisk_Mount();
}

void BddTargetPlusFatMount_Unmount(void)
{
    BddTargetPlusFatDisk_Unmount();
}

bool BddTargetPlusFatMount_Wipe(void)
{
    return BddTargetBoardDisk_Wipe();
}

struct SolidSyslogFile* BddTargetPlusFatMount_CreateFile(void)
{
    return SolidSyslogPlusFatFile_Create();
}

void BddTargetPlusFatMount_DestroyFile(struct SolidSyslogFile* file)
{
    /* PlusFatFile_Destroy -> Close -> ff_fclose flushes the file's dir entry. */
    SolidSyslogPlusFatFile_Destroy(file);
}
