/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogPlusFatFile.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "SolidSyslogError.h"
#include "SolidSyslogErrorCategory.h"
#include "SolidSyslogNullFile.h"
#include "SolidSyslogPlusFatFileErrors.h"
#include "SolidSyslogPlusFatFilePrivate.h"
#include "SolidSyslogPoolAllocator.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogTunables.h"

struct SolidSyslogFile;

static inline size_t PlusFatFile_IndexFromHandle(const struct SolidSyslogFile* base);
static inline void PlusFatFile_CleanupAtIndex(size_t index, void* context);

static bool PlusFatFile_InUse[SOLIDSYSLOG_FILE_POOL_SIZE];
static struct SolidSyslogPlusFatFile PlusFatFile_Pool[SOLIDSYSLOG_FILE_POOL_SIZE];
static struct SolidSyslogPoolAllocator PlusFatFile_Allocator = {PlusFatFile_InUse, SOLIDSYSLOG_FILE_POOL_SIZE};

struct SolidSyslogFile* SolidSyslogPlusFatFile_Create(void)
{
    size_t index = SolidSyslogPoolAllocator_AcquireFirstFree(&PlusFatFile_Allocator);
    struct SolidSyslogFile* handle = SolidSyslogNullFile_Get();
    if (SolidSyslogPoolAllocator_IndexIsValid(&PlusFatFile_Allocator, index) == true)
    {
        SolidSyslogPlusFatFile_Initialise(&PlusFatFile_Pool[index].Base);
        handle = &PlusFatFile_Pool[index].Base;
    }
    else
    {
        PlusFatFile_Report(
            SOLIDSYSLOG_POOL_EXHAUSTED_SEVERITY,
            SOLIDSYSLOG_CAT_POOL_EXHAUSTED,
            SOLIDSYSLOG_PLUSFAT_FILE_ERROR_POOL_EXHAUSTED
        );
    }
    return handle;
}

void SolidSyslogPlusFatFile_Destroy(struct SolidSyslogFile* base)
{
    size_t index = PlusFatFile_IndexFromHandle(base);
    bool released =
        SolidSyslogPoolAllocator_IndexIsValid(&PlusFatFile_Allocator, index) &&
        SolidSyslogPoolAllocator_FreeIfInUse(&PlusFatFile_Allocator, index, PlusFatFile_CleanupAtIndex, NULL);
    if (!released)
    {
        PlusFatFile_Report(
            SOLIDSYSLOG_UNKNOWN_DESTROY_SEVERITY,
            SOLIDSYSLOG_CAT_UNKNOWN_DESTROY,
            SOLIDSYSLOG_PLUSFAT_FILE_ERROR_UNKNOWN_DESTROY
        );
    }
}

static inline size_t PlusFatFile_IndexFromHandle(const struct SolidSyslogFile* base)
{
    size_t result = SOLIDSYSLOG_FILE_POOL_SIZE;
    for (size_t poolIndex = 0; poolIndex < SOLIDSYSLOG_FILE_POOL_SIZE; poolIndex++)
    {
        if (base == &PlusFatFile_Pool[poolIndex].Base)
        {
            result = poolIndex;
            break;
        }
    }
    return result;
}

static inline void PlusFatFile_CleanupAtIndex(size_t index, void* context)
{
    (void) context;
    SolidSyslogPlusFatFile_Cleanup(&PlusFatFile_Pool[index].Base);
}
