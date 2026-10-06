/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogVxWorks64File.h"

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>

#include "vxWorks.h"

#include "ioLib.h"

#include "SolidSyslogError.h"
#include "SolidSyslogFileDefinition.h"
#include "SolidSyslogNullFile.h"
#include "SolidSyslogVxWorks64FileErrors.h"
#include "SolidSyslogVxWorks64FilePrivate.h"

const struct SolidSyslogErrorSource SolidSyslogVxWorks64FileErrorSource = {"VxWorks64File"};

/* rw------- : dosFs ignores the mode, HRFS keeps it. Spelled in hex because
 * MISRA 7.1 rules out the octal the permission bits are usually written in. */
enum
{
    VXWORKS64FILE_OWNER_READ_WRITE = 0x180
};

static bool VxWorks64File_Open(struct SolidSyslogFile* base, const char* path);
static void VxWorks64File_Close(struct SolidSyslogFile* base);
static bool VxWorks64File_IsOpen(struct SolidSyslogFile* base);
static bool VxWorks64File_Read(struct SolidSyslogFile* base, void* buf, size_t count);
static bool VxWorks64File_Write(struct SolidSyslogFile* base, const void* buf, size_t count);
static void VxWorks64File_SeekTo(struct SolidSyslogFile* base, size_t offset);
static size_t VxWorks64File_Size(struct SolidSyslogFile* base);
static void VxWorks64File_Truncate(struct SolidSyslogFile* base);
static bool VxWorks64File_Exists(struct SolidSyslogFile* base, const char* path);
static bool VxWorks64File_Delete(struct SolidSyslogFile* base, const char* path);

static inline struct SolidSyslogVxWorks64File* VxWorks64File_SelfFromBase(struct SolidSyslogFile* base);
static inline bool VxWorks64File_IsWholeTransfer(int transferred, size_t count);
static inline bool VxWorks64File_Commit(int fd);

void SolidSyslogVxWorks64File_Initialise(struct SolidSyslogFile* base)
{
    struct SolidSyslogVxWorks64File* self = VxWorks64File_SelfFromBase(base);
    self->Base.Open = VxWorks64File_Open;
    self->Base.Close = VxWorks64File_Close;
    self->Base.IsOpen = VxWorks64File_IsOpen;
    self->Base.Read = VxWorks64File_Read;
    self->Base.Write = VxWorks64File_Write;
    self->Base.SeekTo = VxWorks64File_SeekTo;
    self->Base.Size = VxWorks64File_Size;
    self->Base.Truncate = VxWorks64File_Truncate;
    self->Base.Exists = VxWorks64File_Exists;
    self->Base.Delete = VxWorks64File_Delete;
    self->Fd = ERROR;
}

void SolidSyslogVxWorks64File_Cleanup(struct SolidSyslogFile* base)
{
    VxWorks64File_Close(base);
    /* Overwrite the abstract base with the shared NullFile vtable so
     * use-after-destroy is a safe no-op rather than a NULL-fn-pointer crash. */
    *base = *SolidSyslogNullFile_Get();
}

static inline struct SolidSyslogVxWorks64File* VxWorks64File_SelfFromBase(struct SolidSyslogFile* base)
{
    return (struct SolidSyslogVxWorks64File*) base;
}

static bool VxWorks64File_Open(struct SolidSyslogFile* base, const char* path)
{
    struct SolidSyslogVxWorks64File* self = VxWorks64File_SelfFromBase(base);
    self->Fd = open(path, O_RDWR | O_CREAT, VXWORKS64FILE_OWNER_READ_WRITE);
    return self->Fd != ERROR;
}

static void VxWorks64File_Close(struct SolidSyslogFile* base)
{
    struct SolidSyslogVxWorks64File* self = VxWorks64File_SelfFromBase(base);
    if (VxWorks64File_IsOpen(base))
    {
        (void) close(self->Fd);
        self->Fd = ERROR;
    }
}

static bool VxWorks64File_IsOpen(struct SolidSyslogFile* base)
{
    struct SolidSyslogVxWorks64File* self = VxWorks64File_SelfFromBase(base);
    return self->Fd != ERROR;
}

static bool VxWorks64File_Read(struct SolidSyslogFile* base, void* buf, size_t count)
{
    struct SolidSyslogVxWorks64File* self = VxWorks64File_SelfFromBase(base);
    return VxWorks64File_IsWholeTransfer(read(self->Fd, (char*) buf, count), count);
}

/* The kernel's read and write answer an int: ERROR, or how many bytes moved. */
static inline bool VxWorks64File_IsWholeTransfer(int transferred, size_t count)
{
    return (transferred >= 0) && ((size_t) transferred == count);
}

static bool VxWorks64File_Write(struct SolidSyslogFile* base, const void* buf, size_t count)
{
    struct SolidSyslogVxWorks64File* self = VxWorks64File_SelfFromBase(base);
    /* The kernel's write takes a char*, though it only reads the buffer. */
    bool committed = VxWorks64File_IsWholeTransfer(write(self->Fd, (char*) buf, count), count);
    if (committed)
    {
        committed = VxWorks64File_Commit(self->Fd);
    }
    return committed;
}

/* FIOSYNC synchronises what the file system holds for this file with the
 * device. FIOCOMMITFS then commits a transactional block device's transaction,
 * which dosFs can sit on; a device with none answers ENOTSUP, as HRFS passes
 * the request down to one, and that counts as committed. */
static inline bool VxWorks64File_Commit(int fd)
{
    bool committed = ioctl(fd, FIOSYNC, 0) != ERROR;
    if (committed)
    {
        int status = ioctl(fd, FIOCOMMITFS, 0);
        /* Read errno straight after the call that set it, with nothing between
         * (MISRA 22.10). */
        int commitErrno = (status == ERROR) ? errno : 0;
        committed = (status != ERROR) || (commitErrno == ENOTSUP);
    }
    return committed;
}

static void VxWorks64File_SeekTo(struct SolidSyslogFile* base, size_t offset)
{
    struct SolidSyslogVxWorks64File* self = VxWorks64File_SelfFromBase(base);
    (void) lseek(self->Fd, (off_t) offset, SEEK_SET);
}

static size_t VxWorks64File_Size(struct SolidSyslogFile* base)
{
    struct SolidSyslogVxWorks64File* self = VxWorks64File_SelfFromBase(base);
    off_t end = lseek(self->Fd, 0, SEEK_END);
    return (end >= 0) ? (size_t) end : 0U;
}

static void VxWorks64File_Truncate(struct SolidSyslogFile* base)
{
    struct SolidSyslogVxWorks64File* self = VxWorks64File_SelfFromBase(base);
    (void) ioctl(self->Fd, FIOTRUNC, 0);
}

/* The kernel offers no access(); a path exists if it opens. */
static bool VxWorks64File_Exists(struct SolidSyslogFile* base, const char* path)
{
    (void) base;
    int probe = open(path, O_RDONLY, 0);
    bool exists = probe != ERROR;
    if (exists)
    {
        (void) close(probe);
    }
    return exists;
}

/* The I/O system names no error for a path that was never there, so an absent
 * path is told apart by looking for it after remove fails. */
static bool VxWorks64File_Delete(struct SolidSyslogFile* base, const char* path)
{
    bool deleted = remove(path) == OK;
    if (!deleted)
    {
        deleted = !VxWorks64File_Exists(base, path);
    }
    return deleted;
}
