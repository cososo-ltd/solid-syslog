/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogBlockStore.h"

#include <stdbool.h>
#include <stdint.h>

#include "SolidSyslogBlockSequencePrivate.h"
#include "SolidSyslogRecordStorePrivate.h"
#include "SolidSyslogBlockDevice.h"
#include "SolidSyslogBlockStoreErrors.h"
#include "SolidSyslogBlockStorePrivate.h"
#include "SolidSyslogError.h"
#include "SolidSyslogNullStore.h"
#include "SolidSyslogStoreCategories.h"
#include "SolidSyslogStoreDefinition.h"

const struct SolidSyslogErrorSource SolidSyslogBlockStoreErrorSource = {"BlockStore"};

/* vtable - forward-declared because BlockStore_InitialiseVtable references them before their definitions */
static bool BlockStore_Write(struct SolidSyslogStore* base, const void* data, size_t size);
static bool BlockStore_ReadNextUnsent(struct SolidSyslogStore* base, void* data, size_t maxSize, size_t* bytesRead);
static void BlockStore_MarkSent(struct SolidSyslogStore* base);
static bool BlockStore_HasUnsent(struct SolidSyslogStore* base);
static bool BlockStore_IsHalted(struct SolidSyslogStore* base);
static size_t BlockStore_GetTotalBytes(struct SolidSyslogStore* base);
static size_t BlockStore_GetUsedBytes(struct SolidSyslogStore* base);
static bool BlockStore_IsTransient(struct SolidSyslogStore* base);

static inline struct SolidSyslogBlockStore* BlockStore_SelfFromBase(struct SolidSyslogStore* base);
static inline void BlockStore_InitialiseVtable(struct SolidSyslogBlockStore* self);
static void BlockStore_ResumeFromExistingBlock(struct SolidSyslogBlockStore* self);
static void BlockStore_Freeze(struct SolidSyslogBlockStore* self, enum SolidSyslogBlockStoreErrors failedOperation);

/* ------------------------------------------------------------------
 * Initialise / Cleanup - private lifecycle pair invoked by Static.c.
 *
 * Initialise stores the two inner pool pointers (Static.c acquired them
 * before this call), wires the vtable, and runs the existing-block
 * resume scan. Cleanup is a pure vtable swap to NullStore for
 * use-after-destroy crash-safety - Static.c destroys the inner pool
 * slots outside the outer FreeIfInUse lock, which keeps the per-pool
 * ConfigLock acquisitions sequential rather than nested.
 * ----------------------------------------------------------------*/

void SolidSyslogBlockStore_Initialise(
    struct SolidSyslogStore* base,
    struct SolidSyslogRecordStore* recordStore,
    struct SolidSyslogBlockSequence* blockSequence,
    const struct SolidSyslogBlockStoreConfig* config
)
{
    (void) config; /* config drove the inner Creates already; nothing left to copy. */
    struct SolidSyslogBlockStore* self = BlockStore_SelfFromBase(base);
    self->RecordStore = recordStore;
    self->BlockSequence = blockSequence;
    BlockStore_InitialiseVtable(self);

    if (SolidSyslogBlockSequence_Open(self->BlockSequence))
    {
        BlockStore_ResumeFromExistingBlock(self);
    }
    else
    {
        BlockStore_Freeze(self, SOLIDSYSLOG_BLOCK_STORE_ERROR_OPEN_FAILED);
    }
}

void SolidSyslogBlockStore_Cleanup(struct SolidSyslogStore* base)
{
    /* Overwrite the abstract base with the shared NullStore vtable so
     * use-after-destroy is a safe no-op rather than a NULL-fn-pointer crash.
     * The inner pool pointers are still in the slot at this point - Static.c
     * pulls them out before FreeIfInUse and destroys them after the outer
     * lock is released. */
    *base = *SolidSyslogNullStore_Get();
}

static inline struct SolidSyslogBlockStore* BlockStore_SelfFromBase(struct SolidSyslogStore* base)
{
    return (struct SolidSyslogBlockStore*) base;
}

static inline void BlockStore_InitialiseVtable(struct SolidSyslogBlockStore* self)
{
    self->Base.Write = BlockStore_Write;
    self->Base.ReadNextUnsent = BlockStore_ReadNextUnsent;
    self->Base.MarkSent = BlockStore_MarkSent;
    self->Base.HasUnsent = BlockStore_HasUnsent;
    self->Base.IsHalted = BlockStore_IsHalted;
    self->Base.GetTotalBytes = BlockStore_GetTotalBytes;
    self->Base.GetUsedBytes = BlockStore_GetUsedBytes;
    self->Base.IsTransient = BlockStore_IsTransient;
}

static void BlockStore_ResumeFromExistingBlock(struct SolidSyslogBlockStore* self)
{
    struct SolidSyslogBlockDevice* device = SolidSyslogBlockSequence_BlockDevice(self->BlockSequence);
    size_t readSequence = SolidSyslogBlockSequence_ReadSequence(self->BlockSequence);
    /* Bound the scan by the read block's actual size, not WritePosition. On a
     * multi-block resume the read block is a closed earlier block whose size
     * is independent of the write block's fill level. */
    size_t readBlockSize = SolidSyslogBlockDevice_Size(device, readSequence);

    enum SolidSyslogRecordStoreScanResult scanned = SOLIDSYSLOG_RECORD_STORE_SCAN_CLEAN;
    size_t cursor =
        SolidSyslogRecordStore_FindFirstUnsent(self->RecordStore, device, readSequence, readBlockSize, &scanned);

    SolidSyslogBlockSequence_SetReadCursor(self->BlockSequence, cursor);

    if (scanned == SOLIDSYSLOG_RECORD_STORE_SCAN_DEVICE_FAILED)
    {
        BlockStore_Freeze(self, SOLIDSYSLOG_BLOCK_STORE_ERROR_READ_FAILED);
    }
    else if (scanned == SOLIDSYSLOG_RECORD_STORE_SCAN_CORRUPT)
    {
        SolidSyslogBlockSequence_MarkWriteBlockCorrupt(self->BlockSequence);
    }
    else
    {
        /* Every record before the cursor was sent. */
    }
}

/* A device that has failed once is not trusted with audit records again. The
 * store takes on the NullStore vtable, as Cleanup does, so it never calls the
 * device again: what it held stays on the media for the next start, and
 * Service sends new records directly because the store is now transient. The
 * slot is still in use, so Destroy releases it as usual. */
static void BlockStore_Freeze(struct SolidSyslogBlockStore* self, enum SolidSyslogBlockStoreErrors failedOperation)
{
    BlockStore_Report(SOLIDSYSLOG_SEVERITY_ERROR, SOLIDSYSLOG_CAT_STORE_FROZEN, failedOperation);
    self->Base = *SolidSyslogNullStore_Get();
}

static bool BlockStore_StoreRecord(struct SolidSyslogBlockStore* self, const void* data, size_t size);
static bool BlockStore_FreezeIfDeviceFailed(struct SolidSyslogBlockStore* self);
static inline void BlockStore_ReportSealFailed(void);

static bool BlockStore_Write(struct SolidSyslogStore* base, const void* data, size_t size)
{
    return BlockStore_StoreRecord(BlockStore_SelfFromBase(base), data, size);
}

static bool BlockStore_StoreRecord(struct SolidSyslogBlockStore* self, const void* data, size_t size)
{
    size_t recordSize = SolidSyslogRecordStore_RecordSize(self->RecordStore, (uint16_t) size);
    bool readBlockChanged = false;
    bool written = false;
    bool prepared = SolidSyslogBlockSequence_PrepareForWrite(self->BlockSequence, recordSize, &readBlockChanged);

    if (!BlockStore_FreezeIfDeviceFailed(self) && prepared)
    {
        if (readBlockChanged)
        {
            SolidSyslogRecordStore_ForgetLastRead(self->RecordStore);
        }

        enum SolidSyslogRecordStoreAppendResult appended = SolidSyslogRecordStore_Append(
            self->RecordStore,
            SolidSyslogBlockSequence_BlockDevice(self->BlockSequence),
            SolidSyslogBlockSequence_WriteSequence(self->BlockSequence),
            data,
            size
        );
        written = appended == SOLIDSYSLOG_RECORD_STORE_APPEND_SUCCEEDED;

        if (written)
        {
            SolidSyslogBlockSequence_NoteRecordWritten(self->BlockSequence, recordSize);
        }
        else if (appended == SOLIDSYSLOG_RECORD_STORE_APPEND_DEVICE_FAILED)
        {
            BlockStore_Freeze(self, SOLIDSYSLOG_BLOCK_STORE_ERROR_APPEND_FAILED);
        }
        else
        {
            BlockStore_ReportSealFailed();
        }
    }

    return written;
}

static bool BlockStore_FreezeIfDeviceFailed(struct SolidSyslogBlockStore* self)
{
    bool failed = SolidSyslogBlockSequence_DeviceFailed(self->BlockSequence);

    if (failed)
    {
        BlockStore_Freeze(self, SolidSyslogBlockSequence_FailedOperation(self->BlockSequence));
    }

    return failed;
}

/* The policy reports why it would not seal; this reports that the record is
 * lost. */
static inline void BlockStore_ReportSealFailed(void)
{
    BlockStore_Report(
        SOLIDSYSLOG_SEVERITY_ERROR,
        SOLIDSYSLOG_CAT_STORE_WRITE_FAILED,
        SOLIDSYSLOG_BLOCK_STORE_ERROR_SEAL_FAILED
    );
}

static bool BlockStore_HasUnsent(struct SolidSyslogStore* base)
{
    return SolidSyslogBlockSequence_HasUnsent(BlockStore_SelfFromBase(base)->BlockSequence);
}

static bool BlockStore_IsHalted(struct SolidSyslogStore* base)
{
    return SolidSyslogBlockSequence_IsHalted(BlockStore_SelfFromBase(base)->BlockSequence);
}

static size_t BlockStore_GetTotalBytes(struct SolidSyslogStore* base)
{
    return SolidSyslogBlockSequence_TotalBytes(BlockStore_SelfFromBase(base)->BlockSequence);
}

static size_t BlockStore_GetUsedBytes(struct SolidSyslogStore* base)
{
    return SolidSyslogBlockSequence_UsedBytes(BlockStore_SelfFromBase(base)->BlockSequence);
}

/* BlockStore retains records - a BlockStore_Write rejection here is the discard
 * policy speaking (DISCARD_NEWEST or HALT), and the message must NOT
 * bypass older stored records via a Service direct-send fallback. */
static bool BlockStore_IsTransient(struct SolidSyslogStore* base)
{
    (void) base;
    return false;
}

static enum SolidSyslogRecordStoreReadResult BlockStore_ReadCurrent(
    struct SolidSyslogBlockStore* self,
    void* data,
    size_t maxSize,
    size_t* bytesRead
);

static bool BlockStore_ReadNextUnsent(struct SolidSyslogStore* base, void* data, size_t maxSize, size_t* bytesRead)
{
    struct SolidSyslogBlockStore* self = BlockStore_SelfFromBase(base);
    *bytesRead = 0;

    if (SolidSyslogBlockSequence_HasUnsent(self->BlockSequence))
    {
        enum SolidSyslogRecordStoreReadResult result = BlockStore_ReadCurrent(self, data, maxSize, bytesRead);

        while ((result == SOLIDSYSLOG_RECORD_STORE_READ_NO_RECORD) &&
               SolidSyslogBlockSequence_ReadIsBehindWrite(self->BlockSequence))
        {
            SolidSyslogBlockSequence_AdvanceToNextReadBlock(self->BlockSequence);
            SolidSyslogRecordStore_ForgetLastRead(self->RecordStore);
            result = BlockStore_ReadCurrent(self, data, maxSize, bytesRead);
        }

        if (result == SOLIDSYSLOG_RECORD_STORE_READ_DEVICE_FAILED)
        {
            BlockStore_Freeze(self, SOLIDSYSLOG_BLOCK_STORE_ERROR_READ_FAILED);
        }
    }

    /* A record of no bytes is nothing to send. */
    return *bytesRead > 0U;
}

static enum SolidSyslogRecordStoreReadResult BlockStore_ReadCurrent(
    struct SolidSyslogBlockStore* self,
    void* data,
    size_t maxSize,
    size_t* bytesRead
)
{
    return SolidSyslogRecordStore_Read(
        self->RecordStore,
        SolidSyslogBlockSequence_BlockDevice(self->BlockSequence),
        SolidSyslogBlockSequence_ReadSequence(self->BlockSequence),
        SolidSyslogBlockSequence_ReadCursor(self->BlockSequence),
        data,
        maxSize,
        bytesRead
    );
}

static void BlockStore_MarkSent(struct SolidSyslogStore* base)
{
    struct SolidSyslogBlockStore* self = BlockStore_SelfFromBase(base);
    size_t nextCursor = 0U;
    enum SolidSyslogRecordStoreMarkResult marked = SolidSyslogRecordStore_MarkLastReadAsSent(
        self->RecordStore,
        SolidSyslogBlockSequence_BlockDevice(self->BlockSequence),
        &nextCursor
    );

    if (marked == SOLIDSYSLOG_RECORD_STORE_MARK_DEVICE_FAILED)
    {
        /* The record was delivered. Freezing is what stops it being read and
         * sent again, since its flag still says unsent. */
        BlockStore_Freeze(self, SOLIDSYSLOG_BLOCK_STORE_ERROR_MARK_SENT_FAILED);
    }
    else if (marked == SOLIDSYSLOG_RECORD_STORE_MARK_SUCCEEDED)
    {
        SolidSyslogBlockSequence_SetReadCursor(self->BlockSequence, nextCursor);

        bool readBlockChanged = false;
        SolidSyslogBlockSequence_DisposeReadBlockIfDrained(self->BlockSequence, &readBlockChanged);

        if (!BlockStore_FreezeIfDeviceFailed(self) && readBlockChanged)
        {
            SolidSyslogRecordStore_ForgetLastRead(self->RecordStore);
        }
    }
    else
    {
        /* Nothing read since the last mark - nothing to do. */
    }
}
