/* The target's store kept in files on LittleFS - see BddTargetCmsisStore.h. */

#include "BddTargetCmsisStore.h"

#include "BddTargetCountingNorFlash.h"
#include "BddTargetFreeRtosPipeline.h"
#include "BddTargetLittleFsMount.h"

void BddTargetCmsisStore_Fill(struct BddTargetFreeRtosPipelineConfig* config)
{
    config->MountStore = BddTargetLittleFsMount_Mount;
    config->UnmountStore = BddTargetLittleFsMount_Unmount;
    config->CreateStoreFile = BddTargetLittleFsMount_CreateFile;
    config->DestroyStoreFile = BddTargetLittleFsMount_DestroyFile;
    config->WipeStore = BddTargetLittleFsMount_Wipe;
    /* There are counts where the board puts LittleFS on a flash, and none
     * where it puts it on a sector disk. */
    config->DescribeStoreMedium = BddTargetCountingNorFlash_Describe;
}
