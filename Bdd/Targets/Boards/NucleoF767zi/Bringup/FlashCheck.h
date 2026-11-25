#ifndef FLASH_CHECK_H
#define FLASH_CHECK_H

/* Erases, programs and reads back the last sector of the W25Q128, through
 * this image's own flash driver (BddTargetSpiNorFlash.h) and the board's bus
 * under it, and reports the result on the console. It separates a fault in
 * the bus or the part from one in the library's flash pack or a filesystem
 * built on it.
 *
 * The board's part is 16 MiB, and its last sector is outside what any target
 * gives its store, with a filesystem or without, so a store on the part is
 * not disturbed. On a part small enough for a store to reach its last sector,
 * the check would write in the store. The sector is erased afterwards,
 * whatever the result. */
void FlashCheck_Run(void);

#endif
