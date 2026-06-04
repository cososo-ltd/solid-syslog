/* FreeRTOS-Plus-FAT implementation of the shared pipeline's FS-mount seam — see
 * BddTargetPlusFatMount.h. The Plus-FAT sibling of BddTargetFatFsMount: the
 * volume mount/unmount lives in the FF_Disk_t media driver (FFSemihostingDisk);
 * the SolidSyslogFile adapter is SolidSyslogPlusFatFile. SolidSyslog S29.05. */

#include "BddTargetPlusFatMount.h"

#include "FFSemihostingDisk.h"
#include "SolidSyslogPlusFatFile.h"

bool BddTargetPlusFatMount_Mount(void)
{
    return FFSemihostingDisk_Mount();
}

void BddTargetPlusFatMount_Unmount(void)
{
    FFSemihostingDisk_Unmount();
}

struct SolidSyslogFile* BddTargetPlusFatMount_CreateFile(void)
{
    return SolidSyslogPlusFatFile_Create();
}

void BddTargetPlusFatMount_DestroyFile(struct SolidSyslogFile* file)
{
    /* PlusFatFile_Destroy → Close → ff_fclose flushes the file's dir entry. */
    SolidSyslogPlusFatFile_Destroy(file);
}
