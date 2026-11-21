#ifndef BDD_TARGET_CMSIS_STORE_H
#define BDD_TARGET_CMSIS_STORE_H

struct BddTargetFreeRtosPipelineConfig;

/* The store this build of the target has. Two files define the function and
 * the build takes one of them: BddTargetCmsisStore_LittleFs.c, for a store
 * kept in files on LittleFS, and BddTargetCmsisStore_NorFlash.c, for one kept
 * directly on the board's NOR flash.
 *
 * Fills the store members of the pipeline's config and leaves the rest. */
void BddTargetCmsisStore_Fill(struct BddTargetFreeRtosPipelineConfig* config);

#endif /* BDD_TARGET_CMSIS_STORE_H */
