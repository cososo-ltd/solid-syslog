#ifndef LITTLEFSDISK_H
#define LITTLEFSDISK_H

#include "SolidSyslogExternC.h"
#include "lfs.h"

#include <stdbool.h>
#include <stdint.h>

SOLIDSYSLOG_EXTERN_C_BEGIN

    /* An emulated flash device for the LittleFS integration suite.
 *
 * The storage is an array rather than a host file. It is standing in for flash,
 * and a host filesystem underneath would contribute its own buffering to a test
 * whose whole subject is durability.
 *
 * What it adds over a plain memory device is a power cut. A cut is armed for the
 * Nth program or erase; when it fires, that operation either does nothing (a
 * clean cut) or writes half its buffer and fails (a torn one), and every
 * operation after it fails too, because the power is off. LittleFsDisk_PowerOn
 * ends the outage, leaving the storage exactly as the cut left it. */

    enum LittleFsDiskGeometry
    {
        LITTLEFS_DISK_READ_SIZE = 16,
        LITTLEFS_DISK_PROG_SIZE = 16,
        LITTLEFS_DISK_BLOCK_SIZE = 512,
        LITTLEFS_DISK_BLOCK_COUNT = 256,
        LITTLEFS_DISK_CACHE_SIZE = 64,
        LITTLEFS_DISK_LOOKAHEAD_SIZE = 16,
        LITTLEFS_DISK_BYTES = LITTLEFS_DISK_BLOCK_SIZE * LITTLEFS_DISK_BLOCK_COUNT
    };

    enum LittleFsDiskCut
    {
        /* The operation does not happen at all. */
        LITTLEFS_DISK_CUT_CLEAN,
        /* The operation writes the first half of its buffer, then fails. */
        LITTLEFS_DISK_CUT_TORN
    };

    struct LittleFsDisk
    {
        uint8_t Storage[LITTLEFS_DISK_BYTES];
        uint8_t ReadBuffer[LITTLEFS_DISK_CACHE_SIZE];
        uint8_t ProgBuffer[LITTLEFS_DISK_CACHE_SIZE];
        uint8_t LookaheadBuffer[LITTLEFS_DISK_LOOKAHEAD_SIZE];
        unsigned WriteCount; /* programs + erases attempted since power on */
        unsigned CutAfter; /* 0 = no cut armed */
        enum LittleFsDiskCut CutKind;
        bool PowerLost;
    };

    /* Erase the whole device to 0xFF, as a blank flash part reads, and clear any
 * armed cut. */
    void LittleFsDisk_Init(struct LittleFsDisk * disk);

    /* Fill in geometry, callbacks and the static buffers. The config points at the
 * disk, so both must outlive the mount. */
    void LittleFsDisk_Configure(struct LittleFsDisk * disk, struct lfs_config * config);

    /* Cut the power during the Nth program or erase from now, counting from 1. */
    void LittleFsDisk_CutAfter(struct LittleFsDisk * disk, unsigned writes, enum LittleFsDiskCut kind);

    /* True once the armed cut has fired. */
    bool LittleFsDisk_PowerLost(const struct LittleFsDisk* disk);

    /* Restore power, leaving the storage as the cut left it. */
    void LittleFsDisk_PowerOn(struct LittleFsDisk * disk);

    unsigned LittleFsDisk_WriteCount(const struct LittleFsDisk* disk);

SOLIDSYSLOG_EXTERN_C_END

#endif /* LITTLEFSDISK_H */
