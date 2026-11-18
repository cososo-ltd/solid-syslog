/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogNorFlashBlockDevice.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "SolidSyslogError.h"
#include "SolidSyslogErrorCategory.h"
#include "SolidSyslogNorFlash.h"
#include "SolidSyslogNorFlashBlockDeviceErrors.h"
#include "SolidSyslogNorFlashBlockDevicePrivate.h"
#include "SolidSyslogNullBlockDevice.h"
#include "SolidSyslogNullNorFlash.h"
#include "SolidSyslogPoolAllocator.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogTunables.h"

struct SolidSyslogBlockDevice;

static bool NorFlashBlockDevice_IsWantedConfig(const struct SolidSyslogNorFlashBlockDeviceConfig* config);
static bool NorFlashBlockDevice_ResolveGeometry(
    const struct SolidSyslogNorFlashBlockDeviceConfig* config,
    struct SolidSyslogNorFlashBlockDeviceGeometry* geometry
);
static struct SolidSyslogBlockDevice* NorFlashBlockDevice_CreateInSlot(
    const struct SolidSyslogNorFlashBlockDeviceGeometry* geometry
);
static inline size_t NorFlashBlockDevice_IndexFromHandle(const struct SolidSyslogBlockDevice* base);
static inline void NorFlashBlockDevice_CleanupAtIndex(size_t index, void* context);

static bool NorFlashBlockDevice_InUse[SOLIDSYSLOG_NOR_FLASH_BLOCK_DEVICE_POOL_SIZE];
static struct SolidSyslogNorFlashBlockDevice NorFlashBlockDevice_Pool[SOLIDSYSLOG_NOR_FLASH_BLOCK_DEVICE_POOL_SIZE];
static struct SolidSyslogPoolAllocator NorFlashBlockDevice_Allocator = {
    NorFlashBlockDevice_InUse,
    SOLIDSYSLOG_NOR_FLASH_BLOCK_DEVICE_POOL_SIZE
};

struct SolidSyslogBlockDevice* SolidSyslogNorFlashBlockDevice_Create(
    const struct SolidSyslogNorFlashBlockDeviceConfig* config
)
{
    struct SolidSyslogBlockDevice* result = SolidSyslogNullBlockDevice_Get();
    struct SolidSyslogNorFlashBlockDeviceGeometry geometry = {NULL, 0U, 0U, 0U};

    if (NorFlashBlockDevice_IsWantedConfig(config) && NorFlashBlockDevice_ResolveGeometry(config, &geometry))
    {
        result = NorFlashBlockDevice_CreateInSlot(&geometry);
    }

    return result;
}

/* False for a config with no flash to work on: a NULL, which is reported, or
 * the Null flash, which asks for no device and is not. */
static bool NorFlashBlockDevice_IsWantedConfig(const struct SolidSyslogNorFlashBlockDeviceConfig* config)
{
    bool wanted = false;

    if (config == NULL)
    {
        NorFlashBlockDevice_Report(
            SOLIDSYSLOG_BAD_CONFIG_FATAL_SEVERITY,
            SOLIDSYSLOG_CAT_BAD_CONFIG,
            SOLIDSYSLOG_NOR_FLASH_BLOCK_DEVICE_ERROR_NULL_CONFIG
        );
    }
    else if (config->Flash == NULL)
    {
        NorFlashBlockDevice_Report(
            SOLIDSYSLOG_BAD_CONFIG_FATAL_SEVERITY,
            SOLIDSYSLOG_CAT_BAD_CONFIG,
            SOLIDSYSLOG_NOR_FLASH_BLOCK_DEVICE_ERROR_NULL_FLASH
        );
    }
    else
    {
        wanted = (config->Flash != SolidSyslogNullNorFlash_Get());
    }

    return wanted;
}

/* The subtraction keeps `first + count` from wrapping past the comparison. */
// NOLINTBEGIN(bugprone-easily-swappable-parameters) -- a first block, a count and a bound; distinct semantics
static inline bool NorFlashBlockDevice_BlocksAreWithinFlash(size_t firstBlock, size_t blockCount, size_t flashBlocks)
{
    return (blockCount <= flashBlocks) && (firstBlock <= (flashBlocks - blockCount));
}

// NOLINTEND(bugprone-easily-swappable-parameters)

/* What a flash block holds once the block's state has its place. */
static inline size_t NorFlashBlockDevice_DataBytesIn(size_t flashBlockSize)
{
    return (flashBlockSize > (size_t) SOLIDSYSLOG_NOR_FLASH_BLOCK_DEVICE_STATE_SIZE)
               ? (flashBlockSize - (size_t) SOLIDSYSLOG_NOR_FLASH_BLOCK_DEVICE_STATE_SIZE)
               : 0U;
}

static bool NorFlashBlockDevice_ResolveGeometry(
    const struct SolidSyslogNorFlashBlockDeviceConfig* config,
    struct SolidSyslogNorFlashBlockDeviceGeometry* geometry
)
{
    size_t available = NorFlashBlockDevice_DataBytesIn(SolidSyslogNorFlash_GetBlockSize(config->Flash));
    size_t blockSize = (config->BlockSize == 0U) ? available : config->BlockSize;
    bool resolved = false;

    if (!NorFlashBlockDevice_BlocksAreWithinFlash(
            config->FirstBlock,
            config->BlockCount,
            SolidSyslogNorFlash_GetBlockCount(config->Flash)
        ))
    {
        NorFlashBlockDevice_Report(
            SOLIDSYSLOG_BAD_CONFIG_FATAL_SEVERITY,
            SOLIDSYSLOG_CAT_BAD_CONFIG,
            SOLIDSYSLOG_NOR_FLASH_BLOCK_DEVICE_ERROR_BLOCKS_OUTSIDE_FLASH
        );
    }
    else if ((blockSize == 0U) || (blockSize > available))
    {
        NorFlashBlockDevice_Report(
            SOLIDSYSLOG_BAD_CONFIG_FATAL_SEVERITY,
            SOLIDSYSLOG_CAT_BAD_CONFIG,
            SOLIDSYSLOG_NOR_FLASH_BLOCK_DEVICE_ERROR_BLOCK_SIZE_UNAVAILABLE
        );
    }
    else
    {
        geometry->Flash = config->Flash;
        geometry->FirstBlock = config->FirstBlock;
        geometry->BlockCount = config->BlockCount;
        geometry->BlockSize = blockSize;
        resolved = true;
    }

    return resolved;
}

static struct SolidSyslogBlockDevice* NorFlashBlockDevice_CreateInSlot(
    const struct SolidSyslogNorFlashBlockDeviceGeometry* geometry
)
{
    struct SolidSyslogBlockDevice* result = SolidSyslogNullBlockDevice_Get();
    size_t index = SolidSyslogPoolAllocator_AcquireFirstFree(&NorFlashBlockDevice_Allocator);

    if (SolidSyslogPoolAllocator_IndexIsValid(&NorFlashBlockDevice_Allocator, index))
    {
        SolidSyslogNorFlashBlockDevice_Initialise(&NorFlashBlockDevice_Pool[index].Base, geometry);
        result = &NorFlashBlockDevice_Pool[index].Base;
    }
    else
    {
        NorFlashBlockDevice_Report(
            SOLIDSYSLOG_POOL_EXHAUSTED_SEVERITY,
            SOLIDSYSLOG_CAT_POOL_EXHAUSTED,
            SOLIDSYSLOG_NOR_FLASH_BLOCK_DEVICE_ERROR_POOL_EXHAUSTED
        );
    }

    return result;
}

void SolidSyslogNorFlashBlockDevice_Destroy(struct SolidSyslogBlockDevice* base)
{
    size_t index = NorFlashBlockDevice_IndexFromHandle(base);
    bool released = SolidSyslogPoolAllocator_IndexIsValid(&NorFlashBlockDevice_Allocator, index) &&
                    SolidSyslogPoolAllocator_FreeIfInUse(
                        &NorFlashBlockDevice_Allocator,
                        index,
                        NorFlashBlockDevice_CleanupAtIndex,
                        NULL
                    );
    if (!released)
    {
        NorFlashBlockDevice_Report(
            SOLIDSYSLOG_UNKNOWN_DESTROY_SEVERITY,
            SOLIDSYSLOG_CAT_UNKNOWN_DESTROY,
            SOLIDSYSLOG_NOR_FLASH_BLOCK_DEVICE_ERROR_UNKNOWN_DESTROY
        );
    }
}

static inline size_t NorFlashBlockDevice_IndexFromHandle(const struct SolidSyslogBlockDevice* base)
{
    size_t result = SOLIDSYSLOG_NOR_FLASH_BLOCK_DEVICE_POOL_SIZE;
    for (size_t poolIndex = 0U; poolIndex < SOLIDSYSLOG_NOR_FLASH_BLOCK_DEVICE_POOL_SIZE; poolIndex++)
    {
        if (base == &NorFlashBlockDevice_Pool[poolIndex].Base)
        {
            result = poolIndex;
            break;
        }
    }
    return result;
}

static inline void NorFlashBlockDevice_CleanupAtIndex(size_t index, void* context)
{
    (void) context;
    SolidSyslogNorFlashBlockDevice_Cleanup(&NorFlashBlockDevice_Pool[index].Base);
}
