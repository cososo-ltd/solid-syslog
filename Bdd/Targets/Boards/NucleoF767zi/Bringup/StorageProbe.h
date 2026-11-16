#ifndef STORAGE_PROBE_H
#define STORAGE_PROBE_H

/* Asks the two storage breakouts on SPI4 whether they are there, and says on
 * the console what each answered. It tells a fault in the wiring from one in
 * a driver built on it, so it uses no driver: the bus is driven a bit at a
 * time on the pins, slowly, and nothing is written to either device.
 *
 * README.md in the directory above holds the pin plan. */

/* Takes the SPI4 pins, which the flash check may have had, with both chip
 * selects high. Then probes the MicroSD card and the W25Q128, one line of
 * console output each. */
void StorageProbe_Run(void);

#endif
