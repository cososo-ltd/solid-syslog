#include <stdbool.h>
#include <stddef.h>

#include "SolidSyslogBlockDeviceDefinition.h"
#include "SolidSyslogBlockDevice.h"

bool SolidSyslogBlockDevice_Acquire(struct SolidSyslogBlockDevice* device, size_t blockIndex)
{
    return device->Acquire(device, blockIndex);
}

bool SolidSyslogBlockDevice_Dispose(struct SolidSyslogBlockDevice* device, size_t blockIndex)
{
    return device->Dispose(device, blockIndex);
}

bool SolidSyslogBlockDevice_Exists(struct SolidSyslogBlockDevice* device, size_t blockIndex)
{
    return device->Exists(device, blockIndex);
}

bool SolidSyslogBlockDevice_Read(
    struct SolidSyslogBlockDevice* device,
    size_t blockIndex,
    size_t offset,
    void* buf,
    size_t count
)
{
    return device->Read(device, blockIndex, offset, buf, count);
}

bool SolidSyslogBlockDevice_Append(
    struct SolidSyslogBlockDevice* device,
    size_t blockIndex,
    const void* buf,
    size_t count
)
{
    return device->Append(device, blockIndex, buf, count);
}

bool SolidSyslogBlockDevice_WriteAt(
    struct SolidSyslogBlockDevice* device,
    size_t blockIndex,
    size_t offset,
    const void* buf,
    size_t count
)
{
    return device->WriteAt(device, blockIndex, offset, buf, count);
}

size_t SolidSyslogBlockDevice_Size(struct SolidSyslogBlockDevice* device, size_t blockIndex)
{
    return device->Size(device, blockIndex);
}

size_t SolidSyslogBlockDevice_GetBlockSize(struct SolidSyslogBlockDevice* device)
{
    return device->GetBlockSize(device);
}
