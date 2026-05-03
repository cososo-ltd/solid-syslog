#include "SolidSyslogFileBlockDevice.h"
#include "SolidSyslogBlockDeviceDefinition.h"
#include "SolidSyslogFormatter.h"
#include "SolidSyslogMacros.h"

enum
{
    MAX_PATH_SIZE = 128
};

static const char FILE_EXTENSION[] = ".log";

enum
{
    SEQUENCE_DIGITS   = 2,
    FILENAME_SUFFIX   = SEQUENCE_DIGITS + sizeof(FILE_EXTENSION) - 1,
    MAX_PREFIX_LENGTH = MAX_PATH_SIZE - FILENAME_SUFFIX - 1
};

struct OpenHandle
{
    struct SolidSyslogFile* file;
    size_t                  blockIndex;
    bool                    isOpen;
};

struct SolidSyslogFileBlockDevice
{
    struct SolidSyslogBlockDevice base;
    struct OpenHandle             readHandle;
    struct OpenHandle             writeHandle;
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

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) -- explicit two-handle wiring; integrator distinguishes readFile / writeFile by name
struct SolidSyslogBlockDevice* SolidSyslogFileBlockDevice_Create(SolidSyslogFileBlockDeviceStorage* storage, struct SolidSyslogFile* readFile,
                                                                 struct SolidSyslogFile* writeFile, const char* pathPrefix)
{
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast) -- C header; integrator-supplied storage blob recast to impl
    struct SolidSyslogFileBlockDevice* device = (struct SolidSyslogFileBlockDevice*) storage;

    InitialiseVtable(device);
    device->readHandle  = (struct OpenHandle) {.file = readFile, .blockIndex = 0, .isOpen = false};
    device->writeHandle = (struct OpenHandle) {.file = writeFile, .blockIndex = 0, .isOpen = false};
    device->pathPrefix  = pathPrefix;

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
    CloseIfOpen(&fileDevice->readHandle);
    CloseIfOpen(&fileDevice->writeHandle);
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
static inline const char* FormatBlockFilename(const struct SolidSyslogFileBlockDevice* device, SolidSyslogFormatterStorage* storage, size_t blockIndex);

static bool Acquire(struct SolidSyslogBlockDevice* self, size_t blockIndex)
{
    struct SolidSyslogFileBlockDevice* device = AsFileBlockDevice(self);
    bool                               ready  = EnsureHandleOpenOnBlock(&device->writeHandle, device, blockIndex);

    if (ready)
    {
        SolidSyslogFile_Truncate(device->writeHandle.file);
    }

    return ready;
}

static bool EnsureHandleOpenOnBlock(struct OpenHandle* handle, const struct SolidSyslogFileBlockDevice* device, size_t blockIndex)
{
    bool underlyingFileIsOpen = SolidSyslogFile_IsOpen(handle->file);
    bool ready                = handle->isOpen && underlyingFileIsOpen && (handle->blockIndex == blockIndex);

    if (!ready)
    {
        if (underlyingFileIsOpen)
        {
            SolidSyslogFile_Close(handle->file);
        }
        handle->isOpen = false;

        SolidSyslogFormatterStorage nameStorage[SOLIDSYSLOG_FORMATTER_STORAGE_SIZE(MAX_PATH_SIZE)];
        const char*                 name = FormatBlockFilename(device, nameStorage, blockIndex);

        if (SolidSyslogFile_Open(handle->file, name))
        {
            handle->blockIndex = blockIndex;
            handle->isOpen     = true;
            ready              = true;
        }
    }

    return ready;
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
    struct SolidSyslogFileBlockDevice* device = AsFileBlockDevice(self);

    CloseIfHoldingBlock(&device->readHandle, blockIndex);
    CloseIfHoldingBlock(&device->writeHandle, blockIndex);

    SolidSyslogFormatterStorage nameStorage[SOLIDSYSLOG_FORMATTER_STORAGE_SIZE(MAX_PATH_SIZE)];
    const char*                 name = FormatBlockFilename(device, nameStorage, blockIndex);
    return SolidSyslogFile_Delete(device->writeHandle.file, name);
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
    struct SolidSyslogFileBlockDevice* device = AsFileBlockDevice(self);
    SolidSyslogFormatterStorage        nameStorage[SOLIDSYSLOG_FORMATTER_STORAGE_SIZE(MAX_PATH_SIZE)];
    const char*                        name = FormatBlockFilename(device, nameStorage, blockIndex);
    return SolidSyslogFile_Exists(device->writeHandle.file, name);
}

/* ------------------------------------------------------------------
 * Read / Append / WriteAt / Size
 * ----------------------------------------------------------------*/

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) -- vtable signature: blockIndex / offset are positional, distinct semantics
static bool Read(struct SolidSyslogBlockDevice* self, size_t blockIndex, size_t offset, void* buf, size_t count)
{
    struct SolidSyslogFileBlockDevice* device = AsFileBlockDevice(self);
    bool                               read   = false;

    if (EnsureHandleOpenOnBlock(&device->readHandle, device, blockIndex))
    {
        SolidSyslogFile_SeekTo(device->readHandle.file, offset);
        read = SolidSyslogFile_Read(device->readHandle.file, buf, count);
    }

    return read;
}

static bool Append(struct SolidSyslogBlockDevice* self, size_t blockIndex, const void* buf, size_t count)
{
    struct SolidSyslogFileBlockDevice* device  = AsFileBlockDevice(self);
    bool                               written = false;

    if (EnsureHandleOpenOnBlock(&device->writeHandle, device, blockIndex))
    {
        SolidSyslogFile_SeekTo(device->writeHandle.file, SolidSyslogFile_Size(device->writeHandle.file));
        written = SolidSyslogFile_Write(device->writeHandle.file, buf, count);
    }

    return written;
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) -- vtable signature: blockIndex / offset are positional, distinct semantics
static bool WriteAt(struct SolidSyslogBlockDevice* self, size_t blockIndex, size_t offset, const void* buf, size_t count)
{
    struct SolidSyslogFileBlockDevice* device  = AsFileBlockDevice(self);
    bool                               written = false;

    if (EnsureHandleOpenOnBlock(&device->writeHandle, device, blockIndex))
    {
        SolidSyslogFile_SeekTo(device->writeHandle.file, offset);
        written = SolidSyslogFile_Write(device->writeHandle.file, buf, count);
    }

    return written;
}

static size_t Size(struct SolidSyslogBlockDevice* self, size_t blockIndex)
{
    struct SolidSyslogFileBlockDevice* device = AsFileBlockDevice(self);
    size_t                             size   = 0;

    if (EnsureHandleOpenOnBlock(&device->writeHandle, device, blockIndex))
    {
        size = SolidSyslogFile_Size(device->writeHandle.file);
    }

    return size;
}
