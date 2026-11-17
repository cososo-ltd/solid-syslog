#ifndef FLASH_CHECK_H
#define FLASH_CHECK_H

/* Erases, programs and reads back the last sector of the W25Q128, through
 * the flash driver the BDD targets use (BddTargetSpiNorFlash.h) and the
 * board's bus under it, and reports the result on the console. It separates
 * a fault in the driver, the bus or the part from one in a filesystem built
 * on them.
 *
 * The board's part is 16 MiB, so its last sector is beyond the 8 MiB a
 * target's filesystem is given, and a store on the part is not disturbed. On
 * an 8 MiB part the check would write in the store. The sector is erased
 * afterwards, whatever the result. */
void FlashCheck_Run(void);

#endif
