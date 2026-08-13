/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogPassthroughBuffer.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "SolidSyslogError.h"
#include "SolidSyslogErrorCategory.h"
#include "SolidSyslogNullBuffer.h"
#include "SolidSyslogPassthroughBufferErrors.h"
#include "SolidSyslogPassthroughBufferPrivate.h"
#include "SolidSyslogPoolAllocator.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogTunables.h"

struct SolidSyslogBuffer;
struct SolidSyslogSender;

static inline size_t PassthroughBuffer_IndexFromHandle(const struct SolidSyslogBuffer* base);
static inline void PassthroughBuffer_CleanupAtIndex(size_t index, void* context);

static bool PassthroughBuffer_InUse[SOLIDSYSLOG_PASSTHROUGH_BUFFER_POOL_SIZE];
static struct SolidSyslogPassthroughBuffer PassthroughBuffer_Pool[SOLIDSYSLOG_PASSTHROUGH_BUFFER_POOL_SIZE];
static struct SolidSyslogPoolAllocator PassthroughBuffer_Allocator = {
    PassthroughBuffer_InUse,
    SOLIDSYSLOG_PASSTHROUGH_BUFFER_POOL_SIZE
};

struct SolidSyslogBuffer* SolidSyslogPassthroughBuffer_Create(struct SolidSyslogSender* sender)
{
    struct SolidSyslogBuffer* handle = SolidSyslogNullBuffer_Get();
    if (sender == NULL)
    {
        PassthroughBuffer_Report(
            SOLIDSYSLOG_BAD_CONFIG_FATAL_SEVERITY,
            SOLIDSYSLOG_CAT_BAD_CONFIG,
            SOLIDSYSLOG_PASSTHROUGH_BUFFER_ERROR_NULL_SENDER
        );
    }
    else
    {
        size_t index = SolidSyslogPoolAllocator_AcquireFirstFree(&PassthroughBuffer_Allocator);
        if (SolidSyslogPoolAllocator_IndexIsValid(&PassthroughBuffer_Allocator, index))
        {
            PassthroughBuffer_Initialise(&PassthroughBuffer_Pool[index].Base, sender);
            handle = &PassthroughBuffer_Pool[index].Base;
        }
        else
        {
            PassthroughBuffer_Report(
                SOLIDSYSLOG_POOL_EXHAUSTED_SEVERITY,
                SOLIDSYSLOG_CAT_POOL_EXHAUSTED,
                SOLIDSYSLOG_PASSTHROUGH_BUFFER_ERROR_POOL_EXHAUSTED
            );
        }
    }
    return handle;
}

void SolidSyslogPassthroughBuffer_Destroy(struct SolidSyslogBuffer* base)
{
    size_t index = PassthroughBuffer_IndexFromHandle(base);
    bool released = SolidSyslogPoolAllocator_IndexIsValid(&PassthroughBuffer_Allocator, index) &&
                    SolidSyslogPoolAllocator_FreeIfInUse(
                        &PassthroughBuffer_Allocator,
                        index,
                        PassthroughBuffer_CleanupAtIndex,
                        NULL
                    );
    if (!released)
    {
        PassthroughBuffer_Report(
            SOLIDSYSLOG_UNKNOWN_DESTROY_SEVERITY,
            SOLIDSYSLOG_CAT_UNKNOWN_DESTROY,
            SOLIDSYSLOG_PASSTHROUGH_BUFFER_ERROR_UNKNOWN_DESTROY
        );
    }
}

static inline size_t PassthroughBuffer_IndexFromHandle(const struct SolidSyslogBuffer* base)
{
    size_t result = SOLIDSYSLOG_PASSTHROUGH_BUFFER_POOL_SIZE;
    for (size_t poolIndex = 0; poolIndex < SOLIDSYSLOG_PASSTHROUGH_BUFFER_POOL_SIZE; poolIndex++)
    {
        if (base == &PassthroughBuffer_Pool[poolIndex].Base)
        {
            result = poolIndex;
            break;
        }
    }
    return result;
}

static inline void PassthroughBuffer_CleanupAtIndex(size_t index, void* context)
{
    (void) context;
    PassthroughBuffer_Cleanup(&PassthroughBuffer_Pool[index].Base);
}
