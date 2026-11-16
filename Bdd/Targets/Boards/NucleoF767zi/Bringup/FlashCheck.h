#ifndef FLASH_CHECK_H
#define FLASH_CHECK_H

/* Erases, programs and reads back the last sector of the W25Q128, through
 * the flash driver the BDD targets use (BddTargetSpiNorFlash.h) and the
 * board's bus under it, and says on the console what it found. It tells a
 * fault in the driver, the bus or the part from one in a filesystem built on
 * them.
 *
 * On the part the board has, of 16 MiB, the last sector is beyond the 8 MiB
 * a target's filesystem is given, so a store on the part is not disturbed. A
 * part of 8 MiB has no such sector, and the check would write in its store.
 * The sector is erased last, whatever was found. */
void FlashCheck_Run(void);

#endif
