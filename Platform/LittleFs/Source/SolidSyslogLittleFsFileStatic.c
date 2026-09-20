/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogLittleFsFile.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "SolidSyslogError.h"
#include "SolidSyslogErrorCategory.h"
#include "SolidSyslogLittleFsFileErrors.h"
#include "SolidSyslogLittleFsFilePrivate.h"
#include "SolidSyslogNullFile.h"
#include "SolidSyslogPoolAllocator.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogTunables.h"
#include "lfs.h"

struct SolidSyslogFile;

static inline size_t LittleFsFile_IndexFromHandle(const struct SolidSyslogFile* base);
static inline void LittleFsFile_CleanupAtIndex(size_t index, void* context);

static bool LittleFsFile_InUse[SOLIDSYSLOG_FILE_POOL_SIZE];
static struct SolidSyslogLittleFsFile LittleFsFile_Pool[SOLIDSYSLOG_FILE_POOL_SIZE];
static struct SolidSyslogPoolAllocator LittleFsFile_Allocator = {LittleFsFile_InUse, SOLIDSYSLOG_FILE_POOL_SIZE};

struct SolidSyslogFile* SolidSyslogLittleFsFile_Create(lfs_t* filesystem, void* fileBuffer, uint32_t fileBufferBytes)
{
    size_t index = SolidSyslogPoolAllocator_AcquireFirstFree(&LittleFsFile_Allocator);
    struct SolidSyslogFile* handle = SolidSyslogNullFile_Get();
    if (SolidSyslogPoolAllocator_IndexIsValid(&LittleFsFile_Allocator, index) == true)
    {
        SolidSyslogLittleFsFile_Initialise(&LittleFsFile_Pool[index].Base, filesystem, fileBuffer, fileBufferBytes);
        handle = &LittleFsFile_Pool[index].Base;
    }
    else
    {
        LittleFsFile_Report(
            SOLIDSYSLOG_POOL_EXHAUSTED_SEVERITY,
            SOLIDSYSLOG_CAT_POOL_EXHAUSTED,
            SOLIDSYSLOG_FILE_ERROR_POOL_EXHAUSTED
        );
    }
    return handle;
}

void SolidSyslogLittleFsFile_Destroy(struct SolidSyslogFile* base)
{
    size_t index = LittleFsFile_IndexFromHandle(base);
    bool released = SolidSyslogPoolAllocator_IndexIsValid(&LittleFsFile_Allocator, index) &&
                    SolidSyslogPoolAllocator_FreeIfInUse(&LittleFsFile_Allocator, index, LittleFsFile_CleanupAtIndex, NULL);
    if (!released)
    {
        LittleFsFile_Report(
            SOLIDSYSLOG_UNKNOWN_DESTROY_SEVERITY,
            SOLIDSYSLOG_CAT_UNKNOWN_DESTROY,
            SOLIDSYSLOG_FILE_ERROR_UNKNOWN_DESTROY
        );
    }
}

static inline size_t LittleFsFile_IndexFromHandle(const struct SolidSyslogFile* base)
{
    size_t result = SOLIDSYSLOG_FILE_POOL_SIZE;
    for (size_t poolIndex = 0; poolIndex < SOLIDSYSLOG_FILE_POOL_SIZE; poolIndex++)
    {
        if (base == &LittleFsFile_Pool[poolIndex].Base)
        {
            result = poolIndex;
            break;
        }
    }
    return result;
}

static inline void LittleFsFile_CleanupAtIndex(size_t index, void* context)
{
    (void) context;
    SolidSyslogLittleFsFile_Cleanup(&LittleFsFile_Pool[index].Base);
}
