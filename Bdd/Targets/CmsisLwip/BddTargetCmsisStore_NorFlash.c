/* The target's store kept directly on the board's NOR flash - see
 * BddTargetCmsisStore.h. There are no files, so the two file members stay as
 * they were given. */

#include "BddTargetCmsisStore.h"

#include "BddTargetFreeRtosPipeline.h"
#include "BddTargetNorFlashStore.h"

void BddTargetCmsisStore_Fill(struct BddTargetFreeRtosPipelineConfig* config)
{
    config->MountStore = BddTargetNorFlashStore_Mount;
    config->UnmountStore = BddTargetNorFlashStore_Unmount;
    config->WipeStore = BddTargetNorFlashStore_Wipe;
    config->CreateStoreBlockDevice = BddTargetNorFlashStore_CreateBlockDevice;
    config->DestroyStoreBlockDevice = BddTargetNorFlashStore_DestroyBlockDevice;
}
