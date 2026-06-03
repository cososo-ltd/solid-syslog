/* Semihosting disk_* glue between FatFs and a host-backed flat disk
 * image. Runs on QEMU mps2-an385 with -semihosting-config enable=on so
 * BKPT 0xAB invocations are trapped by QEMU and forwarded to host
 * file I/O against solidsyslog-disk.img in QEMU's working directory.
 *
 * Behave's after_scenario removes the image so each scenario starts
 * with no filesystem; disk_initialize creates a fresh 8 MiB sparse
 * file when it sees the open fail or the file is too small,
 * f_mount returns FR_NO_FILESYSTEM on the zero-filled image, and the
 * integrator falls through to f_mkfs to lay down a FAT.
 *
 * Shared by the FreeRTOS-Plus-TCP and lwIP BDD targets (Bdd/Targets/Common)
 * so both exercise the same semihosting media geometry.
 *
 * Single logical drive (pdrv 0); no exFAT, no LFN. 16384 sectors x
 * 512 B = 8 MiB — large enough for the store-and-forward / capacity /
 * power-cycle scenarios that exercise multi-block files, and clear of
 * the ~4085-cluster FAT12/16 boundary so f_mkfs lays down FAT16 (the
 * geometry the later FreeRTOS-Plus-FAT formatter needs). */

/* ff.h before diskio.h: diskio.h declares disk_* in terms of BYTE / UINT /
 * LBA_t / DWORD / WORD which are typedef'd in ff.h's integer headers. */
#include "ff.h"
#include "diskio.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

enum
{
    SEMIHOSTING_SYS_OPEN = 0x01,
    SEMIHOSTING_SYS_CLOSE = 0x02,
    SEMIHOSTING_SYS_WRITE = 0x05,
    SEMIHOSTING_SYS_READ = 0x06,
    SEMIHOSTING_SYS_SEEK = 0x0A,
    SEMIHOSTING_SYS_FLEN = 0x0C,
};

/* Semihosting file open modes per the ARM Semihosting spec table 5-3.
 * "r+b" opens an existing file for read/write without truncation;
 * "w+b" creates or truncates and opens for read/write. */
enum
{
    SEMIHOSTING_MODE_READ_PLUS_BINARY = 3,
    SEMIHOSTING_MODE_WRITE_PLUS_BINARY = 7,
};

enum
{
    DISK_SECTOR_SIZE = 512,
    DISK_SECTOR_COUNT = 16384,
    DISK_TOTAL_BYTES = DISK_SECTOR_COUNT * DISK_SECTOR_SIZE,
};

static const char DISK_IMAGE_PATH[] = "solidsyslog-disk.img";

static int g_diskHandle = -1;

static int Semihosting(int op, const void* args);
static int SemihostingOpen(const char* path, int mode);
static int SemihostingRead(int handle, void* buffer, int count);
static int SemihostingWrite(int handle, const void* buffer, int count);
static int SemihostingSeek(int handle, int position);
static int SemihostingFlen(int handle);
static int SemihostingClose(int handle);

static bool DiskImageIsReady(void);

DSTATUS disk_initialize(BYTE pdrv)
{
    if (pdrv != 0)
    {
        return STA_NOINIT;
    }
    return DiskImageIsReady() ? 0 : STA_NOINIT;
}

static bool DiskImageIsReady(void)
{
    if (g_diskHandle >= 0)
    {
        return true;
    }
    g_diskHandle = SemihostingOpen(DISK_IMAGE_PATH, SEMIHOSTING_MODE_READ_PLUS_BINARY);
    if (g_diskHandle >= 0)
    {
        int length = SemihostingFlen(g_diskHandle);
        if (length >= (int) DISK_TOTAL_BYTES)
        {
            return true;
        }
        (void) SemihostingClose(g_diskHandle);
        g_diskHandle = -1;
    }
    /* Create or truncate a fresh image. Sparse-extend by seeking to the
     * last byte and writing one zero — POSIX hosts under semihosting
     * back-fill the hole with zeros on read, which is what FatFs needs
     * to see FR_NO_FILESYSTEM and fall through to f_mkfs. */
    g_diskHandle = SemihostingOpen(DISK_IMAGE_PATH, SEMIHOSTING_MODE_WRITE_PLUS_BINARY);
    if (g_diskHandle < 0)
    {
        return false;
    }
    static const uint8_t ZERO_BYTE = 0U;
    if (SemihostingSeek(g_diskHandle, (int) DISK_TOTAL_BYTES - 1) != 0)
    {
        (void) SemihostingClose(g_diskHandle);
        g_diskHandle = -1;
        return false;
    }
    if (SemihostingWrite(g_diskHandle, &ZERO_BYTE, 1) != 0)
    {
        (void) SemihostingClose(g_diskHandle);
        g_diskHandle = -1;
        return false;
    }
    return true;
}

static int SemihostingOpen(const char* path, int mode)
{
    const struct
    {
        const char* name;
        int mode;
        int length;
    } args = {path, mode, (int) strlen(path)};

    return Semihosting(SEMIHOSTING_SYS_OPEN, &args);
}

static int Semihosting(int op, const void* args)
{
    /* BKPT 0xAB is the Cortex-M Thumb semihosting trap. r0 is the
     * operation number on entry and the return value on exit; r1
     * is a pointer to a per-op parameter block. memory clobber so
     * the compiler doesn't reorder around buffers passed by pointer. */
    register int result __asm("r0") = op;
    register const void* request __asm("r1") = args;
    __asm volatile("bkpt 0xAB" : "+r"(result) : "r"(request) : "memory");
    return result;
}

static int SemihostingFlen(int handle)
{
    /* SYS_FLEN returns the file length in bytes, -1 on error. */
    const struct
    {
        int handle;
    } args = {handle};

    return Semihosting(SEMIHOSTING_SYS_FLEN, &args);
}

static int SemihostingClose(int handle)
{
    const struct
    {
        int handle;
    } args = {handle};

    return Semihosting(SEMIHOSTING_SYS_CLOSE, &args);
}

static int SemihostingSeek(int handle, int position)
{
    /* SYS_SEEK returns 0 on success, a negative value on error. */
    const struct
    {
        int handle;
        int position;
    } args = {handle, position};

    return Semihosting(SEMIHOSTING_SYS_SEEK, &args);
}

static int SemihostingWrite(int handle, const void* buffer, int count)
{
    /* SYS_WRITE returns the number of bytes NOT written (0 == full write). */
    const struct
    {
        int handle;
        const void* buffer;
        int count;
    } args = {handle, buffer, count};

    return Semihosting(SEMIHOSTING_SYS_WRITE, &args);
}

DSTATUS disk_status(BYTE pdrv)
{
    if (pdrv != 0)
    {
        return STA_NOINIT;
    }
    return (g_diskHandle >= 0) ? 0 : STA_NOINIT;
}

DRESULT disk_read(BYTE pdrv, BYTE* buff, LBA_t sector, UINT count)
{
    if ((pdrv != 0) || (g_diskHandle < 0))
    {
        return RES_NOTRDY;
    }
    if ((sector + count) > (LBA_t) DISK_SECTOR_COUNT)
    {
        return RES_PARERR;
    }
    int position = (int) sector * (int) DISK_SECTOR_SIZE;
    if (SemihostingSeek(g_diskHandle, position) != 0)
    {
        return RES_ERROR;
    }
    int bytes = (int) count * (int) DISK_SECTOR_SIZE;
    if (SemihostingRead(g_diskHandle, buff, bytes) != 0)
    {
        return RES_ERROR;
    }
    return RES_OK;
}

static int SemihostingRead(int handle, void* buffer, int count)
{
    /* SYS_READ returns the number of bytes NOT read (0 == full read). */
    const struct
    {
        int handle;
        void* buffer;
        int count;
    } args = {handle, buffer, count};

    return Semihosting(SEMIHOSTING_SYS_READ, &args);
}

DRESULT disk_write(BYTE pdrv, const BYTE* buff, LBA_t sector, UINT count)
{
    if ((pdrv != 0) || (g_diskHandle < 0))
    {
        return RES_NOTRDY;
    }
    if ((sector + count) > (LBA_t) DISK_SECTOR_COUNT)
    {
        return RES_PARERR;
    }
    int position = (int) sector * (int) DISK_SECTOR_SIZE;
    if (SemihostingSeek(g_diskHandle, position) != 0)
    {
        return RES_ERROR;
    }
    int bytes = (int) count * (int) DISK_SECTOR_SIZE;
    if (SemihostingWrite(g_diskHandle, buff, bytes) != 0)
    {
        return RES_ERROR;
    }
    return RES_OK;
}

DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void* buff)
{
    if (pdrv != 0)
    {
        return RES_PARERR;
    }
    switch (cmd)
    {
        case CTRL_SYNC:
            /* QEMU semihosting writes are synchronous against the host
             * file — no kernel-level dirty pages we need to flush. */
            return RES_OK;
        case GET_SECTOR_COUNT:
            *(LBA_t*) buff = (LBA_t) DISK_SECTOR_COUNT;
            return RES_OK;
        case GET_SECTOR_SIZE:
            *(WORD*) buff = (WORD) DISK_SECTOR_SIZE;
            return RES_OK;
        case GET_BLOCK_SIZE:
            /* Erase-block size in sectors. The semihosting flat file
             * has no native erase granularity; 1 is the safe minimum. */
            *(DWORD*) buff = 1U;
            return RES_OK;
        default:
            return RES_PARERR;
    }
}
