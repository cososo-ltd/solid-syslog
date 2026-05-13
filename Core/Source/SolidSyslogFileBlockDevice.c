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
    SEQUENCE_DIGITS   = 2,
    FILENAME_SUFFIX   = SEQUENCE_DIGITS + sizeof(FILE_EXTENSION) - 1,
    MAX_PREFIX_LENGTH = MAX_PATH_SIZE - FILENAME_SUFFIX - 1,
    /* Two-digit on-disk sequence — indices > 99 cannot be represented
     * uniquely. Without this guard, a wide blockIndex would be narrowed
     * to uint8_t and alias an existing block (256 -> 00). */
    MAX_BLOCK_INDEX = 99
};

static inline bool IsValidBlockIndex(size_t blockIndex)
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
    size_t                  blockIndex;
    bool                    isOpen;
};

struct SolidSyslogFileBlockDevice
{
    struct SolidSyslogBlockDevice base;
    struct OpenHandle             handle;
    const char*                   pathPrefix;
};

SOLIDSYSLOG_STATIC_ASSERT(sizeof(struct SolidSyslogFileBlockDevice) <= sizeof(SolidSyslogFileBlockDeviceStorage),
                          "SOLIDSYSLOG_FILEBLOCKDEVICE_STORAGE_SIZE is too small for struct SolidSyslogFileBlockDevice");

/* vtable — forward-declared because Create wires them before their definitions */
static bool Acquire(struct SolidSyslogBlockDevice* self, size_t blockIndex);
static bool Dispose(struct SolidSyslogBlockDevice* self, size_t blockIndex);
static bool Exists(struct SolidSyslogBlockDevice* self, size_t blockIndex);
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) -- vtable signature: blockIndex / offset are positional, distinct semantics
static bool Read(struct SolidSyslogBlockDevice* self, size_t blockIndex, size_t offset, void* buf, size_t count);
static bool Append(struct SolidSyslogBlockDevice* self, size_t blockIndex, const void* buf, size_t count);
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) -- vtable signature: blockIndex / offset are positional, distinct semantics
static bool   WriteAt(struct SolidSyslogBlockDevice* self, size_t blockIndex, size_t offset, const void* buf, size_t count);
static size_t Size(struct SolidSyslogBlockDevice* self, size_t blockIndex);

static inline struct SolidSyslogFileBlockDevice* AsFileBlockDevice(struct SolidSyslogBlockDevice* device);
static inline void                               InitialiseVtable(struct SolidSyslogFileBlockDevice* device);

/* ------------------------------------------------------------------
 * Create
 * ----------------------------------------------------------------*/

struct SolidSyslogBlockDevice* SolidSyslogFileBlockDevice_Create(SolidSyslogFileBlockDeviceStorage* storage, struct SolidSyslogFile* file,
                                                                 const char* pathPrefix)
{
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast) -- C header; integrator-supplied storage blob recast to impl
    struct SolidSyslogFileBlockDevice* device = (struct SolidSyslogFileBlockDevice*) storage;

    InitialiseVtable(device);
    device->handle     = (struct OpenHandle) {.file = file, .blockIndex = 0, .isOpen = false};
    device->pathPrefix = pathPrefix;

    return &device->base;
}

static inline struct SolidSyslogFileBlockDevice* AsFileBlockDevice(struct SolidSyslogBlockDevice* device)
{
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast) -- C; base is the first member of struct SolidSyslogFileBlockDevice
    return (struct SolidSyslogFileBlockDevice*) device;
}

static inline void InitialiseVtable(struct SolidSyslogFileBlockDevice* device)
{
    device->base.Acquire = Acquire;
    device->base.Dispose = Dispose;
    device->base.Exists  = Exists;
    device->base.Read    = Read;
    device->base.Append  = Append;
    device->base.WriteAt = WriteAt;
    device->base.Size    = Size;
}

/* ------------------------------------------------------------------
 * Destroy
 * ----------------------------------------------------------------*/

static inline void CloseIfOpen(struct OpenHandle* handle);

void SolidSyslogFileBlockDevice_Destroy(struct SolidSyslogBlockDevice* device)
{
    struct SolidSyslogFileBlockDevice* fileDevice = AsFileBlockDevice(device);
    CloseIfOpen(&fileDevice->handle);
}

static inline void CloseIfOpen(struct OpenHandle* handle)
{
    if (handle->isOpen)
    {
        SolidSyslogFile_Close(handle->file);
        handle->isOpen = false;
    }
}

/* ------------------------------------------------------------------
 * Acquire
 * ----------------------------------------------------------------*/

static bool               EnsureHandleOpenOnBlock(struct OpenHandle* handle, const struct SolidSyslogFileBlockDevice* device, size_t blockIndex);
static bool               OpenHandleOnBlock(struct OpenHandle* handle, const struct SolidSyslogFileBlockDevice* device, size_t blockIndex);
static inline const char* FormatBlockFilename(const struct SolidSyslogFileBlockDevice* device, SolidSyslogFormatterStorage* storage, size_t blockIndex);

static bool Acquire(struct SolidSyslogBlockDevice* self, size_t blockIndex)
{
    bool ready = false;

    if (IsValidBlockIndex(blockIndex))
    {
        struct SolidSyslogFileBlockDevice* device = AsFileBlockDevice(self);
        ready                                     = EnsureHandleOpenOnBlock(&device->handle, device, blockIndex);

        if (ready)
        {
            SolidSyslogFile_Truncate(device->handle.file);
        }
    }

    return ready;
}

static inline bool IsHandleAlreadyOpenOnBlock(const struct OpenHandle* handle, bool underlyingFileIsOpen, size_t blockIndex)
{
    return handle->isOpen && underlyingFileIsOpen && (handle->blockIndex == blockIndex);
}

static bool EnsureHandleOpenOnBlock(struct OpenHandle* handle, const struct SolidSyslogFileBlockDevice* device, size_t blockIndex)
{
    bool underlyingFileIsOpen = SolidSyslogFile_IsOpen(handle->file);
    bool ready                = IsHandleAlreadyOpenOnBlock(handle, underlyingFileIsOpen, blockIndex);

    if (!ready)
    {
        ready = OpenHandleOnBlock(handle, device, blockIndex);
    }

    return ready;
}

static bool OpenHandleOnBlock(struct OpenHandle* handle, const struct SolidSyslogFileBlockDevice* device, size_t blockIndex)
{
    if (SolidSyslogFile_IsOpen(handle->file))
    {
        SolidSyslogFile_Close(handle->file);
    }
    handle->isOpen = false;

    SolidSyslogFormatterStorage nameStorage[SOLIDSYSLOG_FORMATTER_STORAGE_SIZE(MAX_PATH_SIZE)];
    const char*                 name = FormatBlockFilename(device, nameStorage, blockIndex);

    bool opened = SolidSyslogFile_Open(handle->file, name);

    if (opened)
    {
        handle->blockIndex = blockIndex;
        handle->isOpen     = true;
    }

    return opened;
}

static inline const char* FormatBlockFilename(const struct SolidSyslogFileBlockDevice* device, SolidSyslogFormatterStorage* storage, size_t blockIndex)
{
    struct SolidSyslogFormatter* formatter = SolidSyslogFormatter_Create(storage, MAX_PATH_SIZE);

    SolidSyslogFormatter_BoundedString(formatter, device->pathPrefix, MAX_PREFIX_LENGTH);
    SolidSyslogFormatter_TwoDigit(formatter, (uint8_t) blockIndex);
    SolidSyslogFormatter_BoundedString(formatter, FILE_EXTENSION, sizeof(FILE_EXTENSION) - 1);

    return SolidSyslogFormatter_AsFormattedBuffer(formatter);
}

/* ------------------------------------------------------------------
 * Dispose
 * ----------------------------------------------------------------*/

static inline void CloseIfHoldingBlock(struct OpenHandle* handle, size_t blockIndex);

static bool Dispose(struct SolidSyslogBlockDevice* self, size_t blockIndex)
{
    bool disposed = false;

    if (IsValidBlockIndex(blockIndex))
    {
        struct SolidSyslogFileBlockDevice* device = AsFileBlockDevice(self);

        CloseIfHoldingBlock(&device->handle, blockIndex);

        SolidSyslogFormatterStorage nameStorage[SOLIDSYSLOG_FORMATTER_STORAGE_SIZE(MAX_PATH_SIZE)];
        const char*                 name = FormatBlockFilename(device, nameStorage, blockIndex);
        disposed                         = SolidSyslogFile_Delete(device->handle.file, name);
    }

    return disposed;
}

static inline void CloseIfHoldingBlock(struct OpenHandle* handle, size_t blockIndex)
{
    if (handle->isOpen && (handle->blockIndex == blockIndex))
    {
        SolidSyslogFile_Close(handle->file);
        handle->isOpen = false;
    }
}

/* ------------------------------------------------------------------
 * Exists
 * ----------------------------------------------------------------*/

static bool Exists(struct SolidSyslogBlockDevice* self, size_t blockIndex)
{
    bool exists = false;

    if (IsValidBlockIndex(blockIndex))
    {
        struct SolidSyslogFileBlockDevice* device = AsFileBlockDevice(self);
        SolidSyslogFormatterStorage        nameStorage[SOLIDSYSLOG_FORMATTER_STORAGE_SIZE(MAX_PATH_SIZE)];
        const char*                        name = FormatBlockFilename(device, nameStorage, blockIndex);
        exists                                  = SolidSyslogFile_Exists(device->handle.file, name);
    }

    return exists;
}

/* ------------------------------------------------------------------
 * Read / Append / WriteAt / Size
 * ----------------------------------------------------------------*/

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) -- vtable signature: blockIndex / offset are positional, distinct semantics
static bool Read(struct SolidSyslogBlockDevice* self, size_t blockIndex, size_t offset, void* buf, size_t count)
{
    bool read = false;

    if (IsValidBlockIndex(blockIndex))
    {
        struct SolidSyslogFileBlockDevice* device = AsFileBlockDevice(self);
        if (EnsureHandleOpenOnBlock(&device->handle, device, blockIndex))
        {
            SolidSyslogFile_SeekTo(device->handle.file, offset);
            read = SolidSyslogFile_Read(device->handle.file, buf, count);
        }
    }

    return read;
}

static bool Append(struct SolidSyslogBlockDevice* self, size_t blockIndex, const void* buf, size_t count)
{
    bool written = false;

    if (IsValidBlockIndex(blockIndex))
    {
        struct SolidSyslogFileBlockDevice* device = AsFileBlockDevice(self);
        if (EnsureHandleOpenOnBlock(&device->handle, device, blockIndex))
        {
            SolidSyslogFile_SeekTo(device->handle.file, SolidSyslogFile_Size(device->handle.file));
            written = SolidSyslogFile_Write(device->handle.file, buf, count);
        }
    }

    return written;
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) -- vtable signature: blockIndex / offset are positional, distinct semantics
static bool WriteAt(struct SolidSyslogBlockDevice* self, size_t blockIndex, size_t offset, const void* buf, size_t count)
{
    bool written = false;

    if (IsValidBlockIndex(blockIndex))
    {
        struct SolidSyslogFileBlockDevice* device = AsFileBlockDevice(self);
        if (EnsureHandleOpenOnBlock(&device->handle, device, blockIndex))
        {
            SolidSyslogFile_SeekTo(device->handle.file, offset);
            written = SolidSyslogFile_Write(device->handle.file, buf, count);
        }
    }

    return written;
}

static size_t Size(struct SolidSyslogBlockDevice* self, size_t blockIndex)
{
    size_t size = 0;

    if (IsValidBlockIndex(blockIndex))
    {
        struct SolidSyslogFileBlockDevice* device = AsFileBlockDevice(self);
        if (EnsureHandleOpenOnBlock(&device->handle, device, blockIndex))
        {
            size = SolidSyslogFile_Size(device->handle.file);
        }
    }

    return size;
}
