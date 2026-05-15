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
    MAX_PATH_SIZE = 128
};

static const char FILE_EXTENSION[] = ".log";

enum
{
    SEQUENCE_DIGITS = 2,
    FILENAME_SUFFIX = SEQUENCE_DIGITS + sizeof(FILE_EXTENSION) - 1,
    MAX_PREFIX_LENGTH = MAX_PATH_SIZE - FILENAME_SUFFIX - 1,
    /* Two-digit on-disk sequence — indices > 99 cannot be represented
     * uniquely. Without this guard, a wide blockIndex would be narrowed
     * to uint8_t and alias an existing block (256 -> 00). */
    MAX_BLOCK_INDEX = 99
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
    struct SolidSyslogFile* file;
    size_t blockIndex;
    bool isOpen;
};

struct SolidSyslogFileBlockDevice
{
    struct SolidSyslogBlockDevice base;
    struct OpenHandle handle;
    const char* pathPrefix;
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
    device->handle = (struct OpenHandle) {.file = file, .blockIndex = 0, .isOpen = false};
    device->pathPrefix = pathPrefix;

    return &device->base;
}

static inline struct SolidSyslogFileBlockDevice* FileBlockDevice_AsFileBlockDevice(struct SolidSyslogBlockDevice* device
)
{
    return (struct SolidSyslogFileBlockDevice*) device;
}

static inline void FileBlockDevice_InitialiseVtable(struct SolidSyslogFileBlockDevice* device)
{
    device->base.Acquire = FileBlockDevice_Acquire;
    device->base.Dispose = FileBlockDevice_Dispose;
    device->base.Exists = FileBlockDevice_Exists;
    device->base.Read = FileBlockDevice_Read;
    device->base.Append = FileBlockDevice_Append;
    device->base.WriteAt = FileBlockDevice_WriteAt;
    device->base.Size = FileBlockDevice_Size;
}

/* ------------------------------------------------------------------
 * Destroy
 * ----------------------------------------------------------------*/

static inline void FileBlockDevice_CloseIfOpen(struct OpenHandle* handle);

void SolidSyslogFileBlockDevice_Destroy(struct SolidSyslogBlockDevice* device)
{
    struct SolidSyslogFileBlockDevice* fileDevice = FileBlockDevice_AsFileBlockDevice(device);
    FileBlockDevice_CloseIfOpen(&fileDevice->handle);
}

static inline void FileBlockDevice_CloseIfOpen(struct OpenHandle* handle)
{
    if (handle->isOpen)
    {
        SolidSyslogFile_Close(handle->file);
        handle->isOpen = false;
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
        ready = FileBlockDevice_EnsureHandleOpenOnBlock(&device->handle, device, blockIndex);

        if (ready)
        {
            SolidSyslogFile_Truncate(device->handle.file);
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
    return handle->isOpen && underlyingFileIsOpen && (handle->blockIndex == blockIndex);
}

static bool FileBlockDevice_EnsureHandleOpenOnBlock(
    struct OpenHandle* handle,
    const struct SolidSyslogFileBlockDevice* device,
    size_t blockIndex
)
{
    bool underlyingFileIsOpen = SolidSyslogFile_IsOpen(handle->file);
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
    if (SolidSyslogFile_IsOpen(handle->file))
    {
        SolidSyslogFile_Close(handle->file);
    }
    handle->isOpen = false;

    SolidSyslogFormatterStorage nameStorage[SOLIDSYSLOG_FORMATTER_STORAGE_SIZE(MAX_PATH_SIZE)];
    const char* name = FileBlockDevice_FormatBlockFilename(device, nameStorage, blockIndex);

    bool opened = SolidSyslogFile_Open(handle->file, name);

    if (opened)
    {
        handle->blockIndex = blockIndex;
        handle->isOpen = true;
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

    SolidSyslogFormatter_BoundedString(formatter, device->pathPrefix, MAX_PREFIX_LENGTH);
    SolidSyslogFormatter_TwoDigit(formatter, (uint8_t) blockIndex);
    SolidSyslogFormatter_BoundedString(formatter, FILE_EXTENSION, sizeof(FILE_EXTENSION) - 1);

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

        FileBlockDevice_CloseIfHoldingBlock(&device->handle, blockIndex);

        SolidSyslogFormatterStorage nameStorage[SOLIDSYSLOG_FORMATTER_STORAGE_SIZE(MAX_PATH_SIZE)];
        const char* name = FileBlockDevice_FormatBlockFilename(device, nameStorage, blockIndex);
        disposed = SolidSyslogFile_Delete(device->handle.file, name);
    }

    return disposed;
}

static inline void FileBlockDevice_CloseIfHoldingBlock(struct OpenHandle* handle, size_t blockIndex)
{
    if (handle->isOpen && (handle->blockIndex == blockIndex))
    {
        SolidSyslogFile_Close(handle->file);
        handle->isOpen = false;
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
        exists = SolidSyslogFile_Exists(device->handle.file, name);
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
        if (FileBlockDevice_EnsureHandleOpenOnBlock(&device->handle, device, blockIndex))
        {
            SolidSyslogFile_SeekTo(device->handle.file, offset);
            read = SolidSyslogFile_Read(device->handle.file, buf, count);
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
        if (FileBlockDevice_EnsureHandleOpenOnBlock(&device->handle, device, blockIndex))
        {
            SolidSyslogFile_SeekTo(device->handle.file, SolidSyslogFile_Size(device->handle.file));
            written = SolidSyslogFile_Write(device->handle.file, buf, count);
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
        if (FileBlockDevice_EnsureHandleOpenOnBlock(&device->handle, device, blockIndex))
        {
            SolidSyslogFile_SeekTo(device->handle.file, offset);
            written = SolidSyslogFile_Write(device->handle.file, buf, count);
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
        if (FileBlockDevice_EnsureHandleOpenOnBlock(&device->handle, device, blockIndex))
        {
            size = SolidSyslogFile_Size(device->handle.file);
        }
    }

    return size;
}
