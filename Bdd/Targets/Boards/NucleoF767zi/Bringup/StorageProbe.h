#ifndef STORAGE_PROBE_H
#define STORAGE_PROBE_H

/* Checks that the two storage breakouts on SPI4 answer, and reports on the
 * console what each returned. Its purpose is to separate a wiring fault from
 * a driver fault, so it uses no driver: it bit-bangs the bus slowly on the
 * pins, and writes nothing to either device.
 *
 * README.md in the directory above holds the pin plan. */

/* Takes the SPI4 pins and leaves both chip selects high. */
void StorageProbe_Init(void);

/* Takes the pins again, in case the flash check has had them since, and
 * probes the MicroSD card, then the W25Q128, one console line each. */
void StorageProbe_Run(void);

#endif
