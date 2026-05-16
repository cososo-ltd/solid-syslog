#include "SolidSyslogFileBlockDevice.h"

#include <stdbool.h>
#include <stddef.h>

#include "SolidSyslogBlockDeviceDefinition.h"
#include "SolidSyslogFormatter.h"
#include "SolidSyslogMacros.h"
#include "SolidSyslogFile.h"

struct SolidSyslogFile;

enum
{
    MAX_PATH_SIZE = 128U
};

static const char FILE_EXTENSION[] = ".log";

enum
{
    SEQUENCE_DIGITS = 2U,
    FILENAME_SUFFIX = SEQUENCE_DIGITS + sizeof(FILE_EXTENSION) - 1U,
    MAX_PREFIX_LENGTH = (size_t) MAX_PATH_SIZE - (size_t) FILENAME_SUFFIX - 1U,
    /* Two-digit on-disk sequence — indices > 99 cannot be represented
     * uniquely. Without this guard, a wide blockIndex would be narrowed
     * to uint8_t and alias an existing block (256 -> 00). */
    MAX_BLOCK_INDEX = 99U
};

static inline bool FileBlockDevice_IsValidBlockIndex(size_t blockIndex)
{
    return blockIndex <= MAX_BLOCK_INDEX;
}

/* OpenHandle caches the single SolidSyslogFile the device holds. The handle is
 * re-pointed only when the targeted blockIndex changes; same-block runs reuse
 * it. This is the structural enforcement of the S27.01 single-handle-per-path
 * invariant — by construction the device has exactly one underlying file. */
struct OpenHandle
{
    struct SolidSyslogFile* File;
    size_t BlockIndex;
    bool IsOpen;
};

struct SolidSyslogFileBlockDevice
{
    struct SolidSyslogBlockDevice Base;
    struct OpenHandle Handle;
    const char* PathPrefix;
};

SOLIDSYSLOG_STATIC_ASSERT(
    sizeof(struct SolidSyslogFileBlockDevice) <= sizeof(SolidSyslogFileBlockDeviceStorage),
    "SOLIDSYSLOG_FILEBLOCKDEVICE_STORAGE_SIZE is too small for struct SolidSyslogFileBlockDevice"
);

/* vtable — forward-declared because Create wires them before their definitions */
static bool FileBlockDevice_Acquire(struct SolidSyslogBlockDevice* self, size_t blockIndex);
static bool FileBlockDevice_Dispose(struct SolidSyslogBlockDevice* self, size_t blockIndex);
static bool FileBlockDevice_Exists(struct SolidSyslogBlockDevice* self, size_t blockIndex);
// NOLINTBEGIN(bugprone-easily-swappable-parameters) -- vtable signature: blockIndex / offset are positional, distinct semantics
static bool FileBlockDevice_Read(
    struct SolidSyslogBlockDevice* self,
    size_t blockIndex,
    size_t offset,
    void* buf,
    size_t count
);
// NOLINTEND(bugprone-easily-swappable-parameters)
static bool FileBlockDevice_Append(
    struct SolidSyslogBlockDevice* self,
    size_t blockIndex,
    const void* buf,
    size_t count
);
// NOLINTBEGIN(bugprone-easily-swappable-parameters) -- vtable signature: blockIndex / offset are positional, distinct semantics
static bool FileBlockDevice_WriteAt(
    struct SolidSyslogBlockDevice* self,
    size_t blockIndex,
    size_t offset,
    const void* buf,
    size_t count
);
// NOLINTEND(bugprone-easily-swappable-parameters)
static size_t FileBlockDevice_Size(struct SolidSyslogBlockDevice* self, size_t blockIndex);

static inline struct SolidSyslogFileBlockDevice* FileBlockDevice_AsFileBlockDevice(struct SolidSyslogBlockDevice* device
);
static inline void FileBlockDevice_InitialiseVtable(struct SolidSyslogFileBlockDevice* device);

/* ------------------------------------------------------------------
 * Create
 * ----------------------------------------------------------------*/

struct SolidSyslogBlockDevice* SolidSyslogFileBlockDevice_Create(
    SolidSyslogFileBlockDeviceStorage* storage,
    struct SolidSyslogFile* file,
    const char* pathPrefix
)
{
    struct SolidSyslogFileBlockDevice* device = (struct SolidSyslogFileBlockDevice*) storage;

    FileBlockDevice_InitialiseVtable(device);
    device->Handle = (struct OpenHandle) {.File = file, .BlockIndex = 0, .IsOpen = false};
    device->PathPrefix = pathPrefix;

    return &device->Base;
}

static inline struct SolidSyslogFileBlockDevice* FileBlockDevice_AsFileBlockDevice(struct SolidSyslogBlockDevice* device
)
{
    return (struct SolidSyslogFileBlockDevice*) device;
}

static inline void FileBlockDevice_InitialiseVtable(struct SolidSyslogFileBlockDevice* device)
{
    device->Base.Acquire = FileBlockDevice_Acquire;
    device->Base.Dispose = FileBlockDevice_Dispose;
    device->Base.Exists = FileBlockDevice_Exists;
    device->Base.Read = FileBlockDevice_Read;
    device->Base.Append = FileBlockDevice_Append;
    device->Base.WriteAt = FileBlockDevice_WriteAt;
    device->Base.Size = FileBlockDevice_Size;
}

/* ------------------------------------------------------------------
 * Destroy
 * ----------------------------------------------------------------*/

static inline void FileBlockDevice_CloseIfOpen(struct OpenHandle* handle);

void SolidSyslogFileBlockDevice_Destroy(struct SolidSyslogBlockDevice* device)
{
    struct SolidSyslogFileBlockDevice* fileDevice = FileBlockDevice_AsFileBlockDevice(device);
    FileBlockDevice_CloseIfOpen(&fileDevice->Handle);
}

static inline void FileBlockDevice_CloseIfOpen(struct OpenHandle* handle)
{
    if (handle->IsOpen)
    {
        SolidSyslogFile_Close(handle->File);
        handle->IsOpen = false;
    }
}

/* ------------------------------------------------------------------
 * FileBlockDevice_Acquire
 * ----------------------------------------------------------------*/

static bool FileBlockDevice_EnsureHandleOpenOnBlock(
    struct OpenHandle* handle,
    const struct SolidSyslogFileBlockDevice* device,
    size_t blockIndex
);
static bool FileBlockDevice_OpenHandleOnBlock(
    struct OpenHandle* handle,
    const struct SolidSyslogFileBlockDevice* device,
    size_t blockIndex
);
static inline const char* FileBlockDevice_FormatBlockFilename(
    const struct SolidSyslogFileBlockDevice* device,
    SolidSyslogFormatterStorage* storage,
    size_t blockIndex
);

static bool FileBlockDevice_Acquire(struct SolidSyslogBlockDevice* self, size_t blockIndex)
{
    bool ready = false;

    if (FileBlockDevice_IsValidBlockIndex(blockIndex))
    {
        struct SolidSyslogFileBlockDevice* device = FileBlockDevice_AsFileBlockDevice(self);
        ready = FileBlockDevice_EnsureHandleOpenOnBlock(&device->Handle, device, blockIndex);

        if (ready)
        {
            SolidSyslogFile_Truncate(device->Handle.File);
        }
    }

    return ready;
}

static inline bool FileBlockDevice_IsHandleAlreadyOpenOnBlock(
    const struct OpenHandle* handle,
    bool underlyingFileIsOpen,
    size_t blockIndex
)
{
    return handle->IsOpen && underlyingFileIsOpen && (handle->BlockIndex == blockIndex);
}

static bool FileBlockDevice_EnsureHandleOpenOnBlock(
    struct OpenHandle* handle,
    const struct SolidSyslogFileBlockDevice* device,
    size_t blockIndex
)
{
    bool underlyingFileIsOpen = SolidSyslogFile_IsOpen(handle->File);
    bool ready = FileBlockDevice_IsHandleAlreadyOpenOnBlock(handle, underlyingFileIsOpen, blockIndex);

    if (!ready)
    {
        ready = FileBlockDevice_OpenHandleOnBlock(handle, device, blockIndex);
    }

    return ready;
}

static bool FileBlockDevice_OpenHandleOnBlock(
    struct OpenHandle* handle,
    const struct SolidSyslogFileBlockDevice* device,
    size_t blockIndex
)
{
    if (SolidSyslogFile_IsOpen(handle->File))
    {
        SolidSyslogFile_Close(handle->File);
    }
    handle->IsOpen = false;

    SolidSyslogFormatterStorage nameStorage[SOLIDSYSLOG_FORMATTER_STORAGE_SIZE(MAX_PATH_SIZE)];
    const char* name = FileBlockDevice_FormatBlockFilename(device, nameStorage, blockIndex);

    bool opened = SolidSyslogFile_Open(handle->File, name);

    if (opened)
    {
        handle->BlockIndex = blockIndex;
        handle->IsOpen = true;
    }

    return opened;
}

static inline const char* FileBlockDevice_FormatBlockFilename(
    const struct SolidSyslogFileBlockDevice* device,
    SolidSyslogFormatterStorage* storage,
    size_t blockIndex
)
{
    struct SolidSyslogFormatter* formatter = SolidSyslogFormatter_Create(storage, MAX_PATH_SIZE);

    SolidSyslogFormatter_BoundedString(formatter, device->PathPrefix, MAX_PREFIX_LENGTH);
    SolidSyslogFormatter_TwoDigit(formatter, (uint8_t) blockIndex);
    SolidSyslogFormatter_BoundedString(formatter, FILE_EXTENSION, sizeof(FILE_EXTENSION) - 1U);

    return SolidSyslogFormatter_AsFormattedBuffer(formatter);
}

/* ------------------------------------------------------------------
 * FileBlockDevice_Dispose
 * ----------------------------------------------------------------*/

static inline void FileBlockDevice_CloseIfHoldingBlock(struct OpenHandle* handle, size_t blockIndex);

static bool FileBlockDevice_Dispose(struct SolidSyslogBlockDevice* self, size_t blockIndex)
{
    bool disposed = false;

    if (FileBlockDevice_IsValidBlockIndex(blockIndex))
    {
        struct SolidSyslogFileBlockDevice* device = FileBlockDevice_AsFileBlockDevice(self);

        FileBlockDevice_CloseIfHoldingBlock(&device->Handle, blockIndex);

        SolidSyslogFormatterStorage nameStorage[SOLIDSYSLOG_FORMATTER_STORAGE_SIZE(MAX_PATH_SIZE)];
        const char* name = FileBlockDevice_FormatBlockFilename(device, nameStorage, blockIndex);
        disposed = SolidSyslogFile_Delete(device->Handle.File, name);
    }

    return disposed;
}

static inline void FileBlockDevice_CloseIfHoldingBlock(struct OpenHandle* handle, size_t blockIndex)
{
    if (handle->IsOpen && (handle->BlockIndex == blockIndex))
    {
        SolidSyslogFile_Close(handle->File);
        handle->IsOpen = false;
    }
}

/* ------------------------------------------------------------------
 * FileBlockDevice_Exists
 * ----------------------------------------------------------------*/

static bool FileBlockDevice_Exists(struct SolidSyslogBlockDevice* self, size_t blockIndex)
{
    bool exists = false;

    if (FileBlockDevice_IsValidBlockIndex(blockIndex))
    {
        struct SolidSyslogFileBlockDevice* device = FileBlockDevice_AsFileBlockDevice(self);
        SolidSyslogFormatterStorage nameStorage[SOLIDSYSLOG_FORMATTER_STORAGE_SIZE(MAX_PATH_SIZE)];
        const char* name = FileBlockDevice_FormatBlockFilename(device, nameStorage, blockIndex);
        exists = SolidSyslogFile_Exists(device->Handle.File, name);
    }

    return exists;
}

/* ------------------------------------------------------------------
 * FileBlockDevice_Read / FileBlockDevice_Append / FileBlockDevice_WriteAt / FileBlockDevice_Size
 * ----------------------------------------------------------------*/

// NOLINTBEGIN(bugprone-easily-swappable-parameters) -- vtable signature: blockIndex / offset are positional, distinct semantics
static bool FileBlockDevice_Read(
    struct SolidSyslogBlockDevice* self,
    size_t blockIndex,
    size_t offset,
    void* buf,
    size_t count
)
{
    bool read = false;

    if (FileBlockDevice_IsValidBlockIndex(blockIndex))
    {
        struct SolidSyslogFileBlockDevice* device = FileBlockDevice_AsFileBlockDevice(self);
        if (FileBlockDevice_EnsureHandleOpenOnBlock(&device->Handle, device, blockIndex))
        {
            SolidSyslogFile_SeekTo(device->Handle.File, offset);
            read = SolidSyslogFile_Read(device->Handle.File, buf, count);
        }
    }

    return read;
}

// NOLINTEND(bugprone-easily-swappable-parameters)

static bool FileBlockDevice_Append(
    struct SolidSyslogBlockDevice* self,
    size_t blockIndex,
    const void* buf,
    size_t count
)
{
    bool written = false;

    if (FileBlockDevice_IsValidBlockIndex(blockIndex))
    {
        struct SolidSyslogFileBlockDevice* device = FileBlockDevice_AsFileBlockDevice(self);
        if (FileBlockDevice_EnsureHandleOpenOnBlock(&device->Handle, device, blockIndex))
        {
            SolidSyslogFile_SeekTo(device->Handle.File, SolidSyslogFile_Size(device->Handle.File));
            written = SolidSyslogFile_Write(device->Handle.File, buf, count);
        }
    }

    return written;
}

// NOLINTBEGIN(bugprone-easily-swappable-parameters) -- vtable signature: blockIndex / offset are positional, distinct semantics
static bool FileBlockDevice_WriteAt(
    struct SolidSyslogBlockDevice* self,
    size_t blockIndex,
    size_t offset,
    const void* buf,
    size_t count
)
{
    bool written = false;

    if (FileBlockDevice_IsValidBlockIndex(blockIndex))
    {
        struct SolidSyslogFileBlockDevice* device = FileBlockDevice_AsFileBlockDevice(self);
        if (FileBlockDevice_EnsureHandleOpenOnBlock(&device->Handle, device, blockIndex))
        {
            SolidSyslogFile_SeekTo(device->Handle.File, offset);
            written = SolidSyslogFile_Write(device->Handle.File, buf, count);
        }
    }

    return written;
}

// NOLINTEND(bugprone-easily-swappable-parameters)

static size_t FileBlockDevice_Size(struct SolidSyslogBlockDevice* self, size_t blockIndex)
{
    size_t size = 0;

    if (FileBlockDevice_IsValidBlockIndex(blockIndex))
    {
        struct SolidSyslogFileBlockDevice* device = FileBlockDevice_AsFileBlockDevice(self);
        if (FileBlockDevice_EnsureHandleOpenOnBlock(&device->Handle, device, blockIndex))
        {
            size = SolidSyslogFile_Size(device->Handle.File);
        }
    }

    return size;
}
