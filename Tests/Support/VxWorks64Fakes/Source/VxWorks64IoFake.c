#include "vxWorks.h"

#include "VxWorks64IoFake.h"

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include "ioLib.h"

/* The first descriptor open hands out: any other than zero, so an adapter that
 * assumed a fixed one would be caught. */
enum
{
    VXWORKS64IOFAKE_FIRST_FD = 7,
    VXWORKS64IOFAKE_ALL_GIVEN = -2,
    VXWORKS64IOFAKE_IOCTL_LOG_SIZE = 8,
    VXWORKS64IOFAKE_NONE = -1,
    VXWORKS64IOFAKE_NAME_SIZE = 128,
    VXWORKS64IOFAKE_WRITTEN_SIZE = 8192,
    VXWORKS64IOFAKE_FILES = 8,
    VXWORKS64IOFAKE_FILE_SIZE = 8192,
    VXWORKS64IOFAKE_DESCRIPTORS = 8
};

struct VxWorks64IoFake_IoctlCall
{
    int Fd;
    int Function;
    int Arg;
};

/* A file on the in-memory disk. */
struct VxWorks64IoFake_File
{
    bool InUse;
    char Name[VXWORKS64IOFAKE_NAME_SIZE];
    char Data[VXWORKS64IOFAKE_FILE_SIZE];
    size_t Size;
};

/* An open descriptor: the file it names and where the next read or write goes. */
struct VxWorks64IoFake_Descriptor
{
    bool InUse;
    int File;
    size_t Position;
};

static struct VxWorks64IoFake_File VxWorks64IoFake_Files[VXWORKS64IOFAKE_FILES];
static struct VxWorks64IoFake_Descriptor VxWorks64IoFake_Descriptors[VXWORKS64IOFAKE_DESCRIPTORS];
static int VxWorks64IoFake_LastFd = VXWORKS64IOFAKE_FIRST_FD;

static bool VxWorks64IoFake_OpensFail = false;
static unsigned VxWorks64IoFake_Opens = 0U;
/* Names and data are copied, since a caller may build them in a buffer that is
 * gone by the time a test asks. */
static char VxWorks64IoFake_OpenName[VXWORKS64IOFAKE_NAME_SIZE];
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
static bool VxWorks64IoFake_FileSizeSet = false;
static long VxWorks64IoFake_FileSize = 0;
static bool VxWorks64IoFake_LseeksFail = false;
static unsigned VxWorks64IoFake_Lseeks = 0U;
static int VxWorks64IoFake_LseekFd = -1;
static long VxWorks64IoFake_LseekOffset = -1;
static int VxWorks64IoFake_LseekWhence = -1;
static int VxWorks64IoFake_FailingIoctl = VXWORKS64IOFAKE_NONE;
static int VxWorks64IoFake_FailingIoctlErrno = EIO;
static int VxWorks64IoFake_FailureErrno = EIO;
static unsigned VxWorks64IoFake_Ioctls = 0U;
static struct VxWorks64IoFake_IoctlCall VxWorks64IoFake_IoctlLog[VXWORKS64IOFAKE_IOCTL_LOG_SIZE];
static bool VxWorks64IoFake_RemovesFail = false;
static unsigned VxWorks64IoFake_Removes = 0U;
static char VxWorks64IoFake_RemoveName[VXWORKS64IOFAKE_NAME_SIZE];
static char VxWorks64IoFake_WrittenBytes[VXWORKS64IOFAKE_WRITTEN_SIZE];
static size_t VxWorks64IoFake_WrittenCount = 0U;
static unsigned VxWorks64IoFake_Closes = 0U;
static int VxWorks64IoFake_ClosedFd = -1;

static void VxWorks64IoFake_CopyName(char* copy, const char* name)
{
    size_t length = strlen(name);
    if (length >= (size_t) VXWORKS64IOFAKE_NAME_SIZE)
    {
        length = (size_t) VXWORKS64IOFAKE_NAME_SIZE - 1U;
    }
    (void) memcpy(copy, name, length);
    copy[length] = '\0';
}

static void VxWorks64IoFake_KeepWritten(const char* buffer, size_t nbytes)
{
    size_t room = (size_t) VXWORKS64IOFAKE_WRITTEN_SIZE - VxWorks64IoFake_WrittenCount;
    size_t kept = (nbytes < room) ? nbytes : room;
    (void) memcpy(&VxWorks64IoFake_WrittenBytes[VxWorks64IoFake_WrittenCount], buffer, kept);
    VxWorks64IoFake_WrittenCount += kept;
}

static int VxWorks64IoFake_FindFile(const char* name)
{
    int found = VXWORKS64IOFAKE_NONE;
    for (int index = 0; (index < VXWORKS64IOFAKE_FILES) && (found == VXWORKS64IOFAKE_NONE); index++)
    {
        if (VxWorks64IoFake_Files[index].InUse && (strcmp(VxWorks64IoFake_Files[index].Name, name) == 0))
        {
            found = index;
        }
    }
    return found;
}

static int VxWorks64IoFake_CreateFile(const char* name)
{
    int created = VXWORKS64IOFAKE_NONE;
    for (int index = 0; (index < VXWORKS64IOFAKE_FILES) && (created == VXWORKS64IOFAKE_NONE); index++)
    {
        if (!VxWorks64IoFake_Files[index].InUse)
        {
            VxWorks64IoFake_Files[index].InUse = true;
            VxWorks64IoFake_CopyName(VxWorks64IoFake_Files[index].Name, name);
            VxWorks64IoFake_Files[index].Size = 0U;
            created = index;
        }
    }
    return created;
}

static int VxWorks64IoFake_OpenDescriptor(int file)
{
    int fd = ERROR;
    for (int index = 0; (index < VXWORKS64IOFAKE_DESCRIPTORS) && (fd == ERROR); index++)
    {
        if (!VxWorks64IoFake_Descriptors[index].InUse)
        {
            VxWorks64IoFake_Descriptors[index].InUse = true;
            VxWorks64IoFake_Descriptors[index].File = file;
            VxWorks64IoFake_Descriptors[index].Position = 0U;
            fd = VXWORKS64IOFAKE_FIRST_FD + index;
        }
    }
    return fd;
}

/* The descriptor's entry, or NULL for one this fake did not open - a socket's,
 * say, closed through the same call. */
static struct VxWorks64IoFake_Descriptor* VxWorks64IoFake_Descriptor(int fd)
{
    int index = fd - VXWORKS64IOFAKE_FIRST_FD;
    bool known = (index >= 0) && (index < VXWORKS64IOFAKE_DESCRIPTORS) && VxWorks64IoFake_Descriptors[index].InUse;
    return known ? &VxWorks64IoFake_Descriptors[index] : NULL;
}

void VxWorks64IoFake_Reset(void)
{
    (void) memset(VxWorks64IoFake_Files, 0, sizeof(VxWorks64IoFake_Files));
    (void) memset(VxWorks64IoFake_Descriptors, 0, sizeof(VxWorks64IoFake_Descriptors));
    VxWorks64IoFake_LastFd = VXWORKS64IOFAKE_FIRST_FD;
    VxWorks64IoFake_OpensFail = false;
    VxWorks64IoFake_Opens = 0U;
    VxWorks64IoFake_OpenName[0] = '\0';
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
    VxWorks64IoFake_FileSizeSet = false;
    VxWorks64IoFake_FileSize = 0;
    VxWorks64IoFake_LseeksFail = false;
    VxWorks64IoFake_Lseeks = 0U;
    VxWorks64IoFake_LseekFd = -1;
    VxWorks64IoFake_LseekOffset = -1;
    VxWorks64IoFake_LseekWhence = -1;
    VxWorks64IoFake_FailingIoctl = VXWORKS64IOFAKE_NONE;
    VxWorks64IoFake_FailingIoctlErrno = EIO;
    VxWorks64IoFake_FailureErrno = EIO;
    VxWorks64IoFake_Ioctls = 0U;
    VxWorks64IoFake_RemovesFail = false;
    VxWorks64IoFake_Removes = 0U;
    VxWorks64IoFake_RemoveName[0] = '\0';
    VxWorks64IoFake_WrittenCount = 0U;
    VxWorks64IoFake_Closes = 0U;
    VxWorks64IoFake_ClosedFd = -1;
}

void VxWorks64IoFake_PutFile(const char* name)
{
    if (VxWorks64IoFake_FindFile(name) == VXWORKS64IOFAKE_NONE)
    {
        (void) VxWorks64IoFake_CreateFile(name);
    }
}

int VxWorks64IoFake_Fd(void)
{
    return VxWorks64IoFake_LastFd;
}

void VxWorks64IoFake_FailWithErrno(int errnoValue)
{
    VxWorks64IoFake_FailureErrno = errnoValue;
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

const char* VxWorks64IoFake_Written(void)
{
    return VxWorks64IoFake_WrittenBytes;
}

size_t VxWorks64IoFake_WrittenLength(void)
{
    return VxWorks64IoFake_WrittenCount;
}

void VxWorks64IoFake_SetFileSize(long size)
{
    VxWorks64IoFake_FileSizeSet = true;
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
    VxWorks64IoFake_FailIoctlWithErrno(function, EIO);
}

void VxWorks64IoFake_FailIoctlWithErrno(int function, int errnoValue)
{
    VxWorks64IoFake_FailingIoctl = function;
    VxWorks64IoFake_FailingIoctlErrno = errnoValue;
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

/* O_CREAT makes a file that is not there; without it, open fails on one. */
int open(const char* name, int flags, int mode)
{
    int fd = ERROR;
    VxWorks64IoFake_Opens++;
    VxWorks64IoFake_OpenMode = mode;
    VxWorks64IoFake_CopyName(VxWorks64IoFake_OpenName, name);
    VxWorks64IoFake_OpenFlags = flags;
    if (!VxWorks64IoFake_OpensFail)
    {
        int file = VxWorks64IoFake_FindFile(name);
        if ((file == VXWORKS64IOFAKE_NONE) && ((flags & O_CREAT) != 0))
        {
            file = VxWorks64IoFake_CreateFile(name);
        }
        if (file != VXWORKS64IOFAKE_NONE)
        {
            fd = VxWorks64IoFake_OpenDescriptor(file);
        }
    }
    if (fd != ERROR)
    {
        VxWorks64IoFake_LastFd = fd;
    }
    else
    {
        errno = VxWorks64IoFake_FailureErrno;
    }
    return fd;
}

/* Scripted bytes, when a test gives some, stand in for the file's own. */
int read(int fd, char* buffer, size_t maxbytes)
{
    int result = ERROR;
    struct VxWorks64IoFake_Descriptor* descriptor = VxWorks64IoFake_Descriptor(fd);
    VxWorks64IoFake_Reads++;
    VxWorks64IoFake_ReadFd = fd;
    VxWorks64IoFake_ReadMaxBytes = (int) maxbytes;
    if (VxWorks64IoFake_ReadsFail)
    {
        errno = VxWorks64IoFake_FailureErrno;
        result = ERROR;
    }
    else if ((VxWorks64IoFake_ReadData != NULL) || (descriptor == NULL))
    {
        result = (VxWorks64IoFake_ReadDataLength < (int) maxbytes) ? VxWorks64IoFake_ReadDataLength : (int) maxbytes;
        if ((result > 0) && (VxWorks64IoFake_ReadData != NULL))
        {
            (void) memcpy(buffer, VxWorks64IoFake_ReadData, (size_t) result);
        }
    }
    else
    {
        const struct VxWorks64IoFake_File* file = &VxWorks64IoFake_Files[descriptor->File];
        size_t available = (descriptor->Position < file->Size) ? (file->Size - descriptor->Position) : 0U;
        size_t count = (available < maxbytes) ? available : maxbytes;
        (void) memcpy(buffer, &file->Data[descriptor->Position], count);
        descriptor->Position += count;
        result = (int) count;
    }
    return result;
}

/* A scripted answer, when a test gives one, leaves the file as it was. */
int write(int fd, char* buffer, size_t nbytes)
{
    int result = (int) nbytes;
    struct VxWorks64IoFake_Descriptor* descriptor = VxWorks64IoFake_Descriptor(fd);
    VxWorks64IoFake_Writes++;
    VxWorks64IoFake_WriteFd = fd;
    VxWorks64IoFake_WriteBuffer = buffer;
    VxWorks64IoFake_KeepWritten(buffer, nbytes);
    VxWorks64IoFake_WriteNBytes = (int) nbytes;
    if (VxWorks64IoFake_WriteResult != VXWORKS64IOFAKE_ALL_GIVEN)
    {
        result = VxWorks64IoFake_WriteResult;
    }
    else if (descriptor != NULL)
    {
        struct VxWorks64IoFake_File* file = &VxWorks64IoFake_Files[descriptor->File];
        size_t room = (descriptor->Position < (size_t) VXWORKS64IOFAKE_FILE_SIZE)
                          ? ((size_t) VXWORKS64IOFAKE_FILE_SIZE - descriptor->Position)
                          : 0U;
        size_t count = (nbytes < room) ? nbytes : room;
        (void) memcpy(&file->Data[descriptor->Position], buffer, count);
        descriptor->Position += count;
        if (descriptor->Position > file->Size)
        {
            file->Size = descriptor->Position;
        }
        result = (int) count;
    }
    if (result == ERROR)
    {
        errno = VxWorks64IoFake_FailureErrno;
    }
    return result;
}

/* A size a test sets stands in for the file's own at SEEK_END. */
off_t lseek(int fd, off_t offset, int whence)
{
    struct VxWorks64IoFake_Descriptor* descriptor = VxWorks64IoFake_Descriptor(fd);
    long end = VxWorks64IoFake_FileSize;
    if (!VxWorks64IoFake_FileSizeSet && (descriptor != NULL))
    {
        end = (long) VxWorks64IoFake_Files[descriptor->File].Size;
    }
    off_t result = (whence == SEEK_END) ? (end + offset) : offset;
    VxWorks64IoFake_Lseeks++;
    VxWorks64IoFake_LseekFd = fd;
    VxWorks64IoFake_LseekOffset = offset;
    VxWorks64IoFake_LseekWhence = whence;
    if (VxWorks64IoFake_LseeksFail)
    {
        errno = VxWorks64IoFake_FailureErrno;
        result = ERROR;
    }
    else if ((descriptor != NULL) && (result >= 0))
    {
        descriptor->Position = (size_t) result;
    }
    return result;
}

int ioctl(int fd, int function, int arg)
{
    struct VxWorks64IoFake_Descriptor* descriptor = VxWorks64IoFake_Descriptor(fd);
    bool fails = function == VxWorks64IoFake_FailingIoctl;
    if (VxWorks64IoFake_Ioctls < (unsigned) VXWORKS64IOFAKE_IOCTL_LOG_SIZE)
    {
        VxWorks64IoFake_IoctlLog[VxWorks64IoFake_Ioctls].Fd = fd;
        VxWorks64IoFake_IoctlLog[VxWorks64IoFake_Ioctls].Function = function;
        VxWorks64IoFake_IoctlLog[VxWorks64IoFake_Ioctls].Arg = arg;
    }
    VxWorks64IoFake_Ioctls++;
    if (!fails && (function == FIOTRUNC) && (descriptor != NULL) && (arg >= 0))
    {
        struct VxWorks64IoFake_File* file = &VxWorks64IoFake_Files[descriptor->File];
        if ((size_t) arg < file->Size)
        {
            file->Size = (size_t) arg;
        }
    }
    if (fails)
    {
        errno = VxWorks64IoFake_FailingIoctlErrno;
    }
    return fails ? ERROR : OK;
}

/* Fails for a path that is not there, as the kernel's does. */
STATUS remove(const char* name)
{
    int file = VxWorks64IoFake_FindFile(name);
    bool removed = !VxWorks64IoFake_RemovesFail && (file != VXWORKS64IOFAKE_NONE);
    VxWorks64IoFake_Removes++;
    VxWorks64IoFake_CopyName(VxWorks64IoFake_RemoveName, name);
    if (removed)
    {
        VxWorks64IoFake_Files[file].InUse = false;
    }
    return removed ? OK : ERROR;
}

STATUS close(int fd)
{
    struct VxWorks64IoFake_Descriptor* descriptor = VxWorks64IoFake_Descriptor(fd);
    VxWorks64IoFake_Closes++;
    VxWorks64IoFake_ClosedFd = fd;
    if (descriptor != NULL)
    {
        descriptor->InUse = false;
    }
    return OK;
}
