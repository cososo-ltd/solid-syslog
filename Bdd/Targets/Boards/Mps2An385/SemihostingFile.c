/* A file on the QEMU host - see SemihostingFile.h. */

#include "SemihostingFile.h"

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

static int SemihostingFile_OpenExisting(const char* path, int bytes);
static int SemihostingFile_Open(const char* path, int mode);
static int Semihosting(int op, const void* args);
static int SemihostingFile_Length(int handle);
static int SemihostingFile_Close(int handle);
static int SemihostingFile_Create(const char* path, int bytes);
static int SemihostingFile_Seek(int handle, int position);
static int SemihostingFile_Read(int handle, void* buffer, int count);
static int SemihostingFile_Write(int handle, const void* buffer, int count);

int SemihostingFile_OpenImage(const char* path, int bytes)
{
    int handle = SemihostingFile_OpenExisting(path, bytes);
    if (handle < 0)
    {
        handle = SemihostingFile_Create(path, bytes);
    }
    return handle;
}

/* A file shorter than it should be is one an earlier run was stopped while
 * creating, and is not opened. */
static int SemihostingFile_OpenExisting(const char* path, int bytes)
{
    int handle = SemihostingFile_Open(path, SEMIHOSTING_MODE_READ_PLUS_BINARY);
    if ((handle >= 0) && (SemihostingFile_Length(handle) < bytes))
    {
        (void) SemihostingFile_Close(handle);
        handle = -1;
    }
    return handle;
}

static int SemihostingFile_Open(const char* path, int mode)
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
     * the compiler does not reorder around buffers passed by pointer. */
    register int result __asm("r0") = op;
    register const void* request __asm("r1") = args;
    __asm volatile("bkpt 0xAB" : "+r"(result) : "r"(request) : "memory");
    return result;
}

static int SemihostingFile_Length(int handle)
{
    /* SYS_FLEN returns the file length in bytes, -1 on error. */
    const struct
    {
        int handle;
    } args = {handle};

    return Semihosting(SEMIHOSTING_SYS_FLEN, &args);
}

static int SemihostingFile_Close(int handle)
{
    const struct
    {
        int handle;
    } args = {handle};

    return Semihosting(SEMIHOSTING_SYS_CLOSE, &args);
}

/* Creates or truncates, then extends by writing one zero as the last byte. A
 * POSIX host leaves the rest a hole, which reads as zeros. */
static int SemihostingFile_Create(const char* path, int bytes)
{
    static const uint8_t ZERO_BYTE = 0U;
    int handle = SemihostingFile_Open(path, SEMIHOSTING_MODE_WRITE_PLUS_BINARY);
    if ((handle >= 0) && (SemihostingFile_WriteAt(handle, bytes - 1, &ZERO_BYTE, 1) != 0))
    {
        (void) SemihostingFile_Close(handle);
        handle = -1;
    }
    return handle;
}

int SemihostingFile_ReadAt(int handle, int position, void* buffer, int count)
{
    int result = SemihostingFile_Seek(handle, position);
    if (result == 0)
    {
        result = SemihostingFile_Read(handle, buffer, count);
    }
    return result;
}

static int SemihostingFile_Seek(int handle, int position)
{
    /* SYS_SEEK returns 0 on success, a negative value on error. */
    const struct
    {
        int handle;
        int position;
    } args = {handle, position};

    return Semihosting(SEMIHOSTING_SYS_SEEK, &args);
}

static int SemihostingFile_Read(int handle, void* buffer, int count)
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

int SemihostingFile_WriteAt(int handle, int position, const void* buffer, int count)
{
    int result = SemihostingFile_Seek(handle, position);
    if (result == 0)
    {
        result = SemihostingFile_Write(handle, buffer, count);
    }
    return result;
}

static int SemihostingFile_Write(int handle, const void* buffer, int count)
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
