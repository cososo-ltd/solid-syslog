#include "SolidSyslogNullBlockDevice.h"

#include <stdbool.h>
#include <stddef.h>

#include "SolidSyslogBlockDeviceDefinition.h"

static bool NullBlockDevice_Acquire(struct SolidSyslogBlockDevice* base, size_t blockIndex);
static bool NullBlockDevice_Dispose(struct SolidSyslogBlockDevice* base, size_t blockIndex);
static bool NullBlockDevice_Exists(struct SolidSyslogBlockDevice* base, size_t blockIndex);
// NOLINTBEGIN(bugprone-easily-swappable-parameters) -- vtable signature mirrored from BlockDevice contract
static bool NullBlockDevice_Read(
    struct SolidSyslogBlockDevice* base,
    size_t blockIndex,
    size_t offset,
    void* buf,
    size_t count
);
static bool NullBlockDevice_WriteAt(
    struct SolidSyslogBlockDevice* base,
    size_t blockIndex,
    size_t offset,
    const void* buf,
    size_t count
);
// NOLINTEND(bugprone-easily-swappable-parameters)
static bool NullBlockDevice_Append(
    struct SolidSyslogBlockDevice* base,
    size_t blockIndex,
    const void* buf,
    size_t count
);
static size_t NullBlockDevice_Size(struct SolidSyslogBlockDevice* base, size_t blockIndex);

struct SolidSyslogBlockDevice* SolidSyslogNullBlockDevice_Get(void)
{
    static struct SolidSyslogBlockDevice instance = {
        .Acquire = NullBlockDevice_Acquire,
        .Dispose = NullBlockDevice_Dispose,
        .Exists = NullBlockDevice_Exists,
        .Read = NullBlockDevice_Read,
        .Append = NullBlockDevice_Append,
        .WriteAt = NullBlockDevice_WriteAt,
        .Size = NullBlockDevice_Size,
    };
    return &instance;
}

static bool NullBlockDevice_Acquire(struct SolidSyslogBlockDevice* base, size_t blockIndex)
{
    (void) base;
    (void) blockIndex;
    return false;
}

static bool NullBlockDevice_Dispose(struct SolidSyslogBlockDevice* base, size_t blockIndex)
{
    (void) base;
    (void) blockIndex;
    return false;
}

static bool NullBlockDevice_Exists(struct SolidSyslogBlockDevice* base, size_t blockIndex)
{
    (void) base;
    (void) blockIndex;
    return false;
}

// NOLINTBEGIN(bugprone-easily-swappable-parameters) -- vtable signature mirrored from BlockDevice contract
static bool NullBlockDevice_Read(
    struct SolidSyslogBlockDevice* base,
    size_t blockIndex,
    size_t offset,
    void* buf,
    size_t count
)
{
    (void) base;
    (void) blockIndex;
    (void) offset;
    (void) buf;
    (void) count;
    return false;
}

// NOLINTEND(bugprone-easily-swappable-parameters)

static bool NullBlockDevice_Append(
    struct SolidSyslogBlockDevice* base,
    size_t blockIndex,
    const void* buf,
    size_t count
)
{
    (void) base;
    (void) blockIndex;
    (void) buf;
    (void) count;
    return false;
}

// NOLINTBEGIN(bugprone-easily-swappable-parameters) -- vtable signature mirrored from BlockDevice contract
static bool NullBlockDevice_WriteAt(
    struct SolidSyslogBlockDevice* base,
    size_t blockIndex,
    size_t offset,
    const void* buf,
    size_t count
)
{
    (void) base;
    (void) blockIndex;
    (void) offset;
    (void) buf;
    (void) count;
    return false;
}

// NOLINTEND(bugprone-easily-swappable-parameters)

static size_t NullBlockDevice_Size(struct SolidSyslogBlockDevice* base, size_t blockIndex)
{
    (void) base;
    (void) blockIndex;
    return 0;
}
