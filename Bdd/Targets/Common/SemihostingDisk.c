/* Host-backed flat-disk media access over ARM semihosting — see
 * SemihostingDisk.h. The BKPT 0xAB trap and the open / read / write / seek /
 * length / close primitives were extracted from diskio.c in SolidSyslog S29.05
 * so the FreeRTOS-Plus-FAT FF_Disk_t driver (FFSemihostingDisk.c) shares the
 * exact same host image, geometry, and trap as the ChaN-FatFs disk_* glue. */

#include "SemihostingDisk.h"

#include <stddef.h>
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
    DISK_TOTAL_BYTES = SEMIHOSTING_DISK_SECTOR_COUNT * SEMIHOSTING_DISK_SECTOR_SIZE,
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
static bool SemihostingDisk_IsRangeValid(uint32_t sector, uint32_t count);

bool SemihostingDisk_EnsureReady(void)
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
     * back-fill the hole with zeros on read, which is what the filesystem
     * needs to see no FAT and fall through to format-on-first-use. */
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

bool SemihostingDisk_IsReady(void)
{
    return g_diskHandle >= 0;
}

enum SemihostingDiskResult SemihostingDisk_Read(void* buffer, uint32_t sector, uint32_t count)
{
    enum SemihostingDiskResult result = SEMIHOSTING_DISK_OK;
    if (!SemihostingDisk_IsRangeValid(sector, count))
    {
        result = SEMIHOSTING_DISK_OUT_OF_RANGE;
    }
    else
    {
        int position = (int) sector * (int) SEMIHOSTING_DISK_SECTOR_SIZE;
        int bytes = (int) count * (int) SEMIHOSTING_DISK_SECTOR_SIZE;
        if (SemihostingSeek(g_diskHandle, position) != 0)
        {
            result = SEMIHOSTING_DISK_IO_ERROR;
        }
        else if (SemihostingRead(g_diskHandle, buffer, bytes) != 0)
        {
            result = SEMIHOSTING_DISK_IO_ERROR;
        }
    }
    return result;
}

/* Range-checks an LBA span without overflow: the subtraction form keeps the sum
 * `sector + count` from wrapping in uint32 (an arithmetic add could wrap a huge
 * sector past the comparison and admit an out-of-bounds seek). */
static bool SemihostingDisk_IsRangeValid(uint32_t sector, uint32_t count)
{
    return (count <= (uint32_t) SEMIHOSTING_DISK_SECTOR_COUNT) &&
           (sector <= ((uint32_t) SEMIHOSTING_DISK_SECTOR_COUNT - count));
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

enum SemihostingDiskResult SemihostingDisk_Write(const void* buffer, uint32_t sector, uint32_t count)
{
    enum SemihostingDiskResult result = SEMIHOSTING_DISK_OK;
    if (!SemihostingDisk_IsRangeValid(sector, count))
    {
        result = SEMIHOSTING_DISK_OUT_OF_RANGE;
    }
    else
    {
        int position = (int) sector * (int) SEMIHOSTING_DISK_SECTOR_SIZE;
        int bytes = (int) count * (int) SEMIHOSTING_DISK_SECTOR_SIZE;
        if (SemihostingSeek(g_diskHandle, position) != 0)
        {
            result = SEMIHOSTING_DISK_IO_ERROR;
        }
        else if (SemihostingWrite(g_diskHandle, buffer, bytes) != 0)
        {
            result = SEMIHOSTING_DISK_IO_ERROR;
        }
    }
    return result;
}
