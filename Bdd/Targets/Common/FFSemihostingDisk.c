/* FreeRTOS-Plus-FAT FF_Disk_t media driver over the shared semihosting flat
 * disk — see FFSemihostingDisk.h. Block read/write delegate to SemihostingDisk
 * (the same host image + BKPT 0xAB trap the ChaN-FatFs diskio.c uses). The IO
 * manager / partition / format / mount sequence mirrors Plus-FAT's reference
 * portable/common/ff_ramdisk.c, with format made first-use-only so a persistent
 * image survives a power cycle (power_cycle_replay). SolidSyslog S29.05. */

#include "FFSemihostingDisk.h"

#include "SemihostingDisk.h"

#include "FreeRTOS.h"
#include "semphr.h"

#include "ff_headers.h"
#include "ff_sys.h"

#include <stdio.h>
#include <string.h>

enum
{
    /* Only a single primary partition, partition 0, is used. */
    DISK_PARTITION_NUMBER = 0,
    DISK_PRIMARY_PARTITIONS = 1,
    /* Keep the conventional first track free, matching ff_ramdisk.c. */
    DISK_HIDDEN_SECTORS = 8,
    /* IO manager sector cache, in 512 B sectors. Multiple of the sector size and
     * at least 2 sectors (FF_CreateIOManager asserts both). Eight sectors (4 KB)
     * gives the FAT / directory traversal comfortable headroom while staying a
     * small fraction of the 96 KB FreeRTOS heap this target also shares with the
     * Plus-TCP buffers and the mbedTLS handshake. */
    DISK_CACHE_SECTORS = 8,
};

/* Magic stamped into the FF_Disk_t so the block callbacks can validate they were
 * handed our disk before touching it. "SYSL". */
#define DISK_SIGNATURE 0x5359534CUL

/* The single FF_Disk_t volume + its mounted flag live in .bss; only the Plus-FAT
 * IO manager cache (allocated inside FF_CreateIOManager) hits the heap. */
static FF_Disk_t g_disk;
static bool g_mounted = false;
static StaticSemaphore_t g_mutexStorage;

static int32_t ReadBlocks(uint8_t* buffer, uint32_t sector, uint32_t count, FF_Disk_t* disk);
static int32_t WriteBlocks(uint8_t* buffer, uint32_t sector, uint32_t count, FF_Disk_t* disk);
static FF_Error_t PartitionAndFormat(void);

bool FFSemihostingDisk_Mount(void)
{
    if (g_mounted)
    {
        return true;
    }
    if (!SemihostingDisk_EnsureReady())
    {
        return false;
    }

    memset(&g_disk, 0, sizeof(g_disk));
    g_disk.ulSignature = DISK_SIGNATURE;
    g_disk.ulNumberOfSectors = (uint32_t) SEMIHOSTING_DISK_SECTOR_COUNT;

    FF_CreationParameters_t parameters;
    memset(&parameters, 0, sizeof(parameters));
    parameters.pucCacheMemory = NULL; /* Plus-FAT mallocs the cache from heap_4. */
    parameters.ulMemorySize = (uint32_t) (DISK_CACHE_SECTORS * SEMIHOSTING_DISK_SECTOR_SIZE);
    parameters.ulSectorSize = SEMIHOSTING_DISK_SECTOR_SIZE;
    parameters.fnWriteBlocks = WriteBlocks;
    parameters.fnReadBlocks = ReadBlocks;
    parameters.pxDisk = &g_disk;
    parameters.pvSemaphore = (void*) xSemaphoreCreateRecursiveMutexStatic(&g_mutexStorage);
    parameters.xBlockDeviceIsReentrant = pdFALSE;

    FF_Error_t error = FF_ERR_NONE;
    g_disk.pxIOManager = FF_CreateIOManager(&parameters, &error);
    if (g_disk.pxIOManager == NULL)
    {
        return false;
    }
    g_disk.xStatus.bIsInitialised = pdTRUE;
    g_disk.xStatus.bPartitionNumber = DISK_PARTITION_NUMBER;

    /* Mount-or-format: try the existing FAT first so a power cycle keeps its
     * data; only a fresh / zero-filled image (no mountable partition) falls
     * through to partition + format + remount. */
    error = FF_Mount(&g_disk, DISK_PARTITION_NUMBER);
    if (FF_isERR(error) != pdFALSE)
    {
        error = PartitionAndFormat();
        if (FF_isERR(error) == pdFALSE)
        {
            error = FF_Mount(&g_disk, DISK_PARTITION_NUMBER);
        }
    }
    if (FF_isERR(error) != pdFALSE)
    {
        (void) printf("[solidsyslog] plusfat mount failed: 0x%08X\n", (unsigned int) error);
        FF_DeleteIOManager(g_disk.pxIOManager);
        g_disk.pxIOManager = NULL;
        return false;
    }

    g_disk.xStatus.bIsMounted = pdTRUE;
    FF_FS_Add("/", &g_disk);
    g_mounted = true;
    return true;
}

void FFSemihostingDisk_Unmount(void)
{
    if (g_mounted)
    {
        FF_FS_Remove("/");
        (void) FF_Unmount(&g_disk);
        FF_DeleteIOManager(g_disk.pxIOManager);
        g_disk.pxIOManager = NULL;
        g_disk.xStatus.bIsMounted = pdFALSE;
        g_mounted = false;
    }
}

/* Lay down a single primary partition spanning the disk and format it FAT16
 * (xPreferFAT16 = pdTRUE). The 8 MiB geometry sits clear of the ~4085-cluster
 * FAT12/16 boundary, so the formatter lands FAT16 as intended. Runs only on a
 * fresh image. */
static FF_Error_t PartitionAndFormat(void)
{
    FF_PartitionParameters_t partition;
    memset(&partition, 0, sizeof(partition));
    partition.ulSectorCount = g_disk.ulNumberOfSectors;
    partition.ulHiddenSectors = DISK_HIDDEN_SECTORS;
    partition.xPrimaryCount = DISK_PRIMARY_PARTITIONS;
    partition.eSizeType = eSizeIsQuota;

    FF_Error_t error = FF_Partition(&g_disk, &partition);
    if (FF_isERR(error) == pdFALSE)
    {
        error = FF_Format(&g_disk, DISK_PARTITION_NUMBER, pdTRUE, pdFALSE);
    }
    return error;
}

static int32_t ReadBlocks(uint8_t* buffer, uint32_t sector, uint32_t count, FF_Disk_t* disk)
{
    int32_t result;
    if ((disk == NULL) || (disk->ulSignature != DISK_SIGNATURE) || (disk->xStatus.bIsInitialised == pdFALSE))
    {
        result = FF_ERR_IOMAN_DRIVER_FATAL_ERROR | FF_ERRFLAG;
    }
    else
    {
        switch (SemihostingDisk_Read(buffer, sector, count))
        {
            case SEMIHOSTING_DISK_OK:
                result = FF_ERR_NONE;
                break;
            case SEMIHOSTING_DISK_OUT_OF_RANGE:
                result = (int32_t) (FF_ERR_IOMAN_OUT_OF_BOUNDS_READ | FF_ERRFLAG);
                break;
            default:
                result = FF_ERR_IOMAN_DRIVER_FATAL_ERROR | FF_ERRFLAG;
                break;
        }
    }
    return result;
}

static int32_t WriteBlocks(uint8_t* buffer, uint32_t sector, uint32_t count, FF_Disk_t* disk)
{
    int32_t result;
    if ((disk == NULL) || (disk->ulSignature != DISK_SIGNATURE) || (disk->xStatus.bIsInitialised == pdFALSE))
    {
        result = FF_ERR_IOMAN_DRIVER_FATAL_ERROR | FF_ERRFLAG;
    }
    else
    {
        switch (SemihostingDisk_Write(buffer, sector, count))
        {
            case SEMIHOSTING_DISK_OK:
                result = FF_ERR_NONE;
                break;
            case SEMIHOSTING_DISK_OUT_OF_RANGE:
                result = (int32_t) (FF_ERR_IOMAN_OUT_OF_BOUNDS_WRITE | FF_ERRFLAG);
                break;
            default:
                result = FF_ERR_IOMAN_DRIVER_FATAL_ERROR | FF_ERRFLAG;
                break;
        }
    }
    return result;
}
