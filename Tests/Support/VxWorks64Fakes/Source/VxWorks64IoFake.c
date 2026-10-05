#include "vxWorks.h"

#include "VxWorks64IoFake.h"

#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include "ioLib.h"

/* What open hands back when it succeeds: any descriptor other than zero, so an
 * adapter that assumed a fixed one would be caught. */
enum
{
    VXWORKS64IOFAKE_FD = 7,
    VXWORKS64IOFAKE_ALL_GIVEN = -2,
    VXWORKS64IOFAKE_IOCTL_LOG_SIZE = 8,
    VXWORKS64IOFAKE_NONE = -1
};

struct VxWorks64IoFake_IoctlCall
{
    int Fd;
    int Function;
    int Arg;
};

static bool VxWorks64IoFake_OpensFail = false;
static unsigned VxWorks64IoFake_Opens = 0U;
static const char* VxWorks64IoFake_OpenName = NULL;
static int VxWorks64IoFake_OpenFlags = -1;
static int VxWorks64IoFake_OpenMode = -1;
static const char* VxWorks64IoFake_ReadData = NULL;
static int VxWorks64IoFake_ReadDataLength = 0;
static bool VxWorks64IoFake_ReadsFail = false;
static unsigned VxWorks64IoFake_Reads = 0U;
static int VxWorks64IoFake_ReadFd = -1;
static int VxWorks64IoFake_ReadMaxBytes = -1;
static int VxWorks64IoFake_WriteResult = VXWORKS64IOFAKE_ALL_GIVEN;
static unsigned VxWorks64IoFake_Writes = 0U;
static int VxWorks64IoFake_WriteFd = -1;
static const char* VxWorks64IoFake_WriteBuffer = NULL;
static int VxWorks64IoFake_WriteNBytes = -1;
static long VxWorks64IoFake_FileSize = 0;
static bool VxWorks64IoFake_LseeksFail = false;
static unsigned VxWorks64IoFake_Lseeks = 0U;
static int VxWorks64IoFake_LseekFd = -1;
static long VxWorks64IoFake_LseekOffset = -1;
static int VxWorks64IoFake_LseekWhence = -1;
static int VxWorks64IoFake_FailingIoctl = VXWORKS64IOFAKE_NONE;
static unsigned VxWorks64IoFake_Ioctls = 0U;
static struct VxWorks64IoFake_IoctlCall VxWorks64IoFake_IoctlLog[VXWORKS64IOFAKE_IOCTL_LOG_SIZE];
static bool VxWorks64IoFake_RemovesFail = false;
static unsigned VxWorks64IoFake_Removes = 0U;
static const char* VxWorks64IoFake_RemoveName = NULL;
static unsigned VxWorks64IoFake_Closes = 0U;
static int VxWorks64IoFake_ClosedFd = -1;

void VxWorks64IoFake_Reset(void)
{
    VxWorks64IoFake_OpensFail = false;
    VxWorks64IoFake_Opens = 0U;
    VxWorks64IoFake_OpenName = NULL;
    VxWorks64IoFake_OpenFlags = -1;
    VxWorks64IoFake_OpenMode = -1;
    VxWorks64IoFake_ReadData = NULL;
    VxWorks64IoFake_ReadDataLength = 0;
    VxWorks64IoFake_ReadsFail = false;
    VxWorks64IoFake_Reads = 0U;
    VxWorks64IoFake_ReadFd = -1;
    VxWorks64IoFake_ReadMaxBytes = -1;
    VxWorks64IoFake_WriteResult = VXWORKS64IOFAKE_ALL_GIVEN;
    VxWorks64IoFake_Writes = 0U;
    VxWorks64IoFake_WriteFd = -1;
    VxWorks64IoFake_WriteBuffer = NULL;
    VxWorks64IoFake_WriteNBytes = -1;
    VxWorks64IoFake_FileSize = 0;
    VxWorks64IoFake_LseeksFail = false;
    VxWorks64IoFake_Lseeks = 0U;
    VxWorks64IoFake_LseekFd = -1;
    VxWorks64IoFake_LseekOffset = -1;
    VxWorks64IoFake_LseekWhence = -1;
    VxWorks64IoFake_FailingIoctl = VXWORKS64IOFAKE_NONE;
    VxWorks64IoFake_Ioctls = 0U;
    VxWorks64IoFake_RemovesFail = false;
    VxWorks64IoFake_Removes = 0U;
    VxWorks64IoFake_RemoveName = NULL;
    VxWorks64IoFake_Closes = 0U;
    VxWorks64IoFake_ClosedFd = -1;
}

int VxWorks64IoFake_Fd(void)
{
    return VXWORKS64IOFAKE_FD;
}

void VxWorks64IoFake_FailOpens(void)
{
    VxWorks64IoFake_OpensFail = true;
}

unsigned VxWorks64IoFake_OpenCallCount(void)
{
    return VxWorks64IoFake_Opens;
}

const char* VxWorks64IoFake_LastOpenName(void)
{
    return VxWorks64IoFake_OpenName;
}

int VxWorks64IoFake_LastOpenFlags(void)
{
    return VxWorks64IoFake_OpenFlags;
}

int VxWorks64IoFake_LastOpenMode(void)
{
    return VxWorks64IoFake_OpenMode;
}

void VxWorks64IoFake_ReadDelivers(const char* data, int length)
{
    VxWorks64IoFake_ReadData = data;
    VxWorks64IoFake_ReadDataLength = length;
}

void VxWorks64IoFake_FailReads(void)
{
    VxWorks64IoFake_ReadsFail = true;
}

unsigned VxWorks64IoFake_ReadCallCount(void)
{
    return VxWorks64IoFake_Reads;
}

int VxWorks64IoFake_LastReadFd(void)
{
    return VxWorks64IoFake_ReadFd;
}

int VxWorks64IoFake_LastReadMaxBytes(void)
{
    return VxWorks64IoFake_ReadMaxBytes;
}

void VxWorks64IoFake_WriteAccepts(int written)
{
    VxWorks64IoFake_WriteResult = written;
}

unsigned VxWorks64IoFake_WriteCallCount(void)
{
    return VxWorks64IoFake_Writes;
}

int VxWorks64IoFake_LastWriteFd(void)
{
    return VxWorks64IoFake_WriteFd;
}

const char* VxWorks64IoFake_LastWriteBuffer(void)
{
    return VxWorks64IoFake_WriteBuffer;
}

int VxWorks64IoFake_LastWriteNBytes(void)
{
    return VxWorks64IoFake_WriteNBytes;
}

void VxWorks64IoFake_SetFileSize(long size)
{
    VxWorks64IoFake_FileSize = size;
}

void VxWorks64IoFake_FailLseeks(void)
{
    VxWorks64IoFake_LseeksFail = true;
}

unsigned VxWorks64IoFake_LseekCallCount(void)
{
    return VxWorks64IoFake_Lseeks;
}

int VxWorks64IoFake_LastLseekFd(void)
{
    return VxWorks64IoFake_LseekFd;
}

long VxWorks64IoFake_LastLseekOffset(void)
{
    return VxWorks64IoFake_LseekOffset;
}

int VxWorks64IoFake_LastLseekWhence(void)
{
    return VxWorks64IoFake_LseekWhence;
}

void VxWorks64IoFake_FailIoctl(int function)
{
    VxWorks64IoFake_FailingIoctl = function;
}

unsigned VxWorks64IoFake_IoctlCallCount(void)
{
    return VxWorks64IoFake_Ioctls;
}

int VxWorks64IoFake_IoctlFd(unsigned call)
{
    return (call < VxWorks64IoFake_Ioctls) ? VxWorks64IoFake_IoctlLog[call].Fd : VXWORKS64IOFAKE_NONE;
}

int VxWorks64IoFake_IoctlFunction(unsigned call)
{
    return (call < VxWorks64IoFake_Ioctls) ? VxWorks64IoFake_IoctlLog[call].Function : VXWORKS64IOFAKE_NONE;
}

int VxWorks64IoFake_IoctlArg(unsigned call)
{
    return (call < VxWorks64IoFake_Ioctls) ? VxWorks64IoFake_IoctlLog[call].Arg : VXWORKS64IOFAKE_NONE;
}

void VxWorks64IoFake_FailRemoves(void)
{
    VxWorks64IoFake_RemovesFail = true;
}

unsigned VxWorks64IoFake_RemoveCallCount(void)
{
    return VxWorks64IoFake_Removes;
}

const char* VxWorks64IoFake_LastRemoveName(void)
{
    return VxWorks64IoFake_RemoveName;
}

unsigned VxWorks64IoFake_CloseCallCount(void)
{
    return VxWorks64IoFake_Closes;
}

int VxWorks64IoFake_LastClosedFd(void)
{
    return VxWorks64IoFake_ClosedFd;
}

int open(const char* name, int flags, int mode)
{
    VxWorks64IoFake_Opens++;
    VxWorks64IoFake_OpenMode = mode;
    VxWorks64IoFake_OpenName = name;
    VxWorks64IoFake_OpenFlags = flags;
    return VxWorks64IoFake_OpensFail ? ERROR : VXWORKS64IOFAKE_FD;
}

int read(int fd, char* buffer, size_t maxbytes)
{
    int result = ERROR;
    VxWorks64IoFake_Reads++;
    VxWorks64IoFake_ReadFd = fd;
    VxWorks64IoFake_ReadMaxBytes = (int) maxbytes;
    if (!VxWorks64IoFake_ReadsFail)
    {
        result = (VxWorks64IoFake_ReadDataLength < (int) maxbytes) ? VxWorks64IoFake_ReadDataLength : (int) maxbytes;
        if (result > 0)
        {
            (void) memcpy(buffer, VxWorks64IoFake_ReadData, (size_t) result);
        }
    }
    return result;
}

int write(int fd, char* buffer, size_t nbytes)
{
    VxWorks64IoFake_Writes++;
    VxWorks64IoFake_WriteFd = fd;
    VxWorks64IoFake_WriteBuffer = buffer;
    VxWorks64IoFake_WriteNBytes = (int) nbytes;
    return (VxWorks64IoFake_WriteResult == VXWORKS64IOFAKE_ALL_GIVEN) ? (int) nbytes : VxWorks64IoFake_WriteResult;
}

off_t lseek(int fd, off_t offset, int whence)
{
    off_t result = (whence == SEEK_END) ? (VxWorks64IoFake_FileSize + offset) : offset;
    VxWorks64IoFake_Lseeks++;
    VxWorks64IoFake_LseekFd = fd;
    VxWorks64IoFake_LseekOffset = offset;
    VxWorks64IoFake_LseekWhence = whence;
    return VxWorks64IoFake_LseeksFail ? ERROR : result;
}

int ioctl(int fd, int function, int arg)
{
    if (VxWorks64IoFake_Ioctls < (unsigned) VXWORKS64IOFAKE_IOCTL_LOG_SIZE)
    {
        VxWorks64IoFake_IoctlLog[VxWorks64IoFake_Ioctls].Fd = fd;
        VxWorks64IoFake_IoctlLog[VxWorks64IoFake_Ioctls].Function = function;
        VxWorks64IoFake_IoctlLog[VxWorks64IoFake_Ioctls].Arg = arg;
    }
    VxWorks64IoFake_Ioctls++;
    return (function == VxWorks64IoFake_FailingIoctl) ? ERROR : OK;
}

STATUS remove(const char* name)
{
    VxWorks64IoFake_Removes++;
    VxWorks64IoFake_RemoveName = name;
    return VxWorks64IoFake_RemovesFail ? ERROR : OK;
}

STATUS close(int fd)
{
    VxWorks64IoFake_Closes++;
    VxWorks64IoFake_ClosedFd = fd;
    return OK;
}
