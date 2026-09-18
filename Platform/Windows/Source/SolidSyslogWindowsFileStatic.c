/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogWindowsFile.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "SolidSyslogError.h"
#include "SolidSyslogErrorCategory.h"
#include "SolidSyslogNullFile.h"
#include "SolidSyslogPoolAllocator.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogTunables.h"
#include "SolidSyslogWindowsFileErrors.h"
#include "SolidSyslogWindowsFilePrivate.h"

struct SolidSyslogFile;

static inline size_t WindowsFile_IndexFromHandle(const struct SolidSyslogFile* base);
static inline void WindowsFile_CleanupAtIndex(size_t index, void* context);

static bool WindowsFile_InUse[SOLIDSYSLOG_FILE_POOL_SIZE];
static struct SolidSyslogWindowsFile WindowsFile_Pool[SOLIDSYSLOG_FILE_POOL_SIZE];
static struct SolidSyslogPoolAllocator WindowsFile_Allocator = {WindowsFile_InUse, SOLIDSYSLOG_FILE_POOL_SIZE};

struct SolidSyslogFile* SolidSyslogWindowsFile_Create(void)
{
    size_t index = SolidSyslogPoolAllocator_AcquireFirstFree(&WindowsFile_Allocator);
    struct SolidSyslogFile* handle = SolidSyslogNullFile_Get();
    if (SolidSyslogPoolAllocator_IndexIsValid(&WindowsFile_Allocator, index) == true)
    {
        SolidSyslogWindowsFile_Initialise(&WindowsFile_Pool[index].Base);
        handle = &WindowsFile_Pool[index].Base;
    }
    else
    {
        WindowsFile_Report(
            SOLIDSYSLOG_POOL_EXHAUSTED_SEVERITY,
            SOLIDSYSLOG_CAT_POOL_EXHAUSTED,
            SOLIDSYSLOG_WINDOWS_FILE_ERROR_POOL_EXHAUSTED
        );
    }
    return handle;
}

void SolidSyslogWindowsFile_Destroy(struct SolidSyslogFile* base)
{
    size_t index = WindowsFile_IndexFromHandle(base);
    bool released =
        SolidSyslogPoolAllocator_IndexIsValid(&WindowsFile_Allocator, index) &&
        SolidSyslogPoolAllocator_FreeIfInUse(&WindowsFile_Allocator, index, WindowsFile_CleanupAtIndex, NULL);
    if (!released)
    {
        WindowsFile_Report(
            SOLIDSYSLOG_UNKNOWN_DESTROY_SEVERITY,
            SOLIDSYSLOG_CAT_UNKNOWN_DESTROY,
            SOLIDSYSLOG_WINDOWS_FILE_ERROR_UNKNOWN_DESTROY
        );
    }
}

static inline size_t WindowsFile_IndexFromHandle(const struct SolidSyslogFile* base)
{
    size_t result = SOLIDSYSLOG_FILE_POOL_SIZE;
    for (size_t poolIndex = 0; poolIndex < SOLIDSYSLOG_FILE_POOL_SIZE; poolIndex++)
    {
        if (base == &WindowsFile_Pool[poolIndex].Base)
        {
            result = poolIndex;
            break;
        }
    }
    return result;
}

static inline void WindowsFile_CleanupAtIndex(size_t index, void* context)
{
    (void) context;
    SolidSyslogWindowsFile_Cleanup(&WindowsFile_Pool[index].Base);
}
