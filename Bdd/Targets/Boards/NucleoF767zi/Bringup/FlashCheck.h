#ifndef FLASH_CHECK_H
#define FLASH_CHECK_H

/* Erases, programs and reads back the last sector of the W25Q128, through
 * the flash driver the BDD targets use (BddTargetSpiNorFlash.h) and the
 * board's bus under it, and says on the console what it found. It tells a
 * fault in the driver, the bus or the part from one in a filesystem built on
 * them.
 *
 * The last sector is beyond what any target's filesystem is given, so a
 * store on the part is not disturbed. It is left erased. */
void FlashCheck_Run(void);

#endif
