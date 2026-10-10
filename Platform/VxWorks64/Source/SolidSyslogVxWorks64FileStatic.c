/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogVxWorks64File.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "SolidSyslogError.h"
#include "SolidSyslogErrorCategory.h"
#include "SolidSyslogNullFile.h"
#include "SolidSyslogPoolAllocator.h"
#include "SolidSyslogVxWorks64FileErrors.h"
#include "SolidSyslogVxWorks64FilePrivate.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogTunables.h"

struct SolidSyslogFile;

static inline size_t VxWorks64File_IndexFromHandle(const struct SolidSyslogFile* base);
static inline void VxWorks64File_CleanupAtIndex(size_t index, void* context);

static bool VxWorks64File_InUse[SOLIDSYSLOG_FILE_POOL_SIZE];
static struct SolidSyslogVxWorks64File VxWorks64File_Pool[SOLIDSYSLOG_FILE_POOL_SIZE];
static struct SolidSyslogPoolAllocator VxWorks64File_Allocator = {VxWorks64File_InUse, SOLIDSYSLOG_FILE_POOL_SIZE};

struct SolidSyslogFile* SolidSyslogVxWorks64File_Create(void)
{
    size_t index = SolidSyslogPoolAllocator_AcquireFirstFree(&VxWorks64File_Allocator);
    struct SolidSyslogFile* handle = SolidSyslogNullFile_Get();
    if (SolidSyslogPoolAllocator_IndexIsValid(&VxWorks64File_Allocator, index) == true)
    {
        SolidSyslogVxWorks64File_Initialise(&VxWorks64File_Pool[index].Base);
        handle = &VxWorks64File_Pool[index].Base;
    }
    else
    {
        VxWorks64File_Report(
            SOLIDSYSLOG_POOL_EXHAUSTED_SEVERITY,
            SOLIDSYSLOG_CAT_POOL_EXHAUSTED,
            SOLIDSYSLOG_FILE_ERROR_POOL_EXHAUSTED
        );
    }
    return handle;
}

void SolidSyslogVxWorks64File_Destroy(struct SolidSyslogFile* base)
{
    size_t index = VxWorks64File_IndexFromHandle(base);
    bool released =
        SolidSyslogPoolAllocator_IndexIsValid(&VxWorks64File_Allocator, index) &&
        SolidSyslogPoolAllocator_FreeIfInUse(&VxWorks64File_Allocator, index, VxWorks64File_CleanupAtIndex, NULL);
    if (!released)
    {
        VxWorks64File_Report(
            SOLIDSYSLOG_UNKNOWN_DESTROY_SEVERITY,
            SOLIDSYSLOG_CAT_UNKNOWN_DESTROY,
            SOLIDSYSLOG_FILE_ERROR_UNKNOWN_DESTROY
        );
    }
}

static inline size_t VxWorks64File_IndexFromHandle(const struct SolidSyslogFile* base)
{
    size_t result = SOLIDSYSLOG_FILE_POOL_SIZE;
    for (size_t poolIndex = 0U; poolIndex < SOLIDSYSLOG_FILE_POOL_SIZE; poolIndex++)
    {
        if (base == &VxWorks64File_Pool[poolIndex].Base)
        {
            result = poolIndex;
            break;
        }
    }
    return result;
}

static inline void VxWorks64File_CleanupAtIndex(size_t index, void* context)
{
    (void) context;
    SolidSyslogVxWorks64File_Cleanup(&VxWorks64File_Pool[index].Base);
}
