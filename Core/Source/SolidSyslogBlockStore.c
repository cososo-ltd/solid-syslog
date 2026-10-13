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
static enum SolidSyslogRecordStoreScanResult BlockStore_ScanReadBlock(struct SolidSyslogBlockStore* self);
static void BlockStore_CheckWriteBlock(struct SolidSyslogBlockStore* self);
static bool BlockStore_AbandonRestOfReadBlock(struct SolidSyslogBlockStore* self);
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
    self->LossReported = false;
    BlockStore_InitialiseVtable(self);

    if (SolidSyslogBlockSequence_Open(self->BlockSequence))
    {
        BlockStore_ResumeFromExistingBlock(self);
    }
    else
    {
        BlockStore_Freeze(self, SolidSyslogBlockSequence_FailedOperation(self->BlockSequence));
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

/* A corrupt block is abandoned and the block after it scanned in its turn, so
 * the cursor comes to rest on the first record still to send. */
static void BlockStore_ResumeFromExistingBlock(struct SolidSyslogBlockStore* self)
{
    enum SolidSyslogRecordStoreScanResult scanned = BlockStore_ScanReadBlock(self);

    while ((scanned == SOLIDSYSLOG_RECORD_STORE_SCAN_CORRUPT) && BlockStore_AbandonRestOfReadBlock(self))
    {
        scanned = BlockStore_ScanReadBlock(self);
    }

    if (scanned != SOLIDSYSLOG_RECORD_STORE_SCAN_DEVICE_FAILED)
    {
        BlockStore_CheckWriteBlock(self);
    }
}

/* Puts the cursor on the read block's first unsent record, or at its end where
 * the records stop being readable. A device that fails freezes the store. */
static enum SolidSyslogRecordStoreScanResult BlockStore_ScanReadBlock(struct SolidSyslogBlockStore* self)
{
    size_t readBlockSize = 0U;
    enum SolidSyslogRecordStoreScanResult scanned = SOLIDSYSLOG_RECORD_STORE_SCAN_DEVICE_FAILED;

    if (SolidSyslogBlockSequence_ReadBlockSize(self->BlockSequence, &readBlockSize))
    {
        size_t cursor = SolidSyslogRecordStore_FindFirstUnsent(
            self->RecordStore,
            SolidSyslogBlockSequence_BlockDevice(self->BlockSequence),
            SolidSyslogBlockSequence_ReadSequence(self->BlockSequence),
            readBlockSize,
            &scanned
        );
        SolidSyslogBlockSequence_SetReadCursor(self->BlockSequence, cursor);

        if (scanned == SOLIDSYSLOG_RECORD_STORE_SCAN_DEVICE_FAILED)
        {
            BlockStore_Freeze(self, SOLIDSYSLOG_BLOCK_STORE_ERROR_READ_FAILED);
        }
    }
    else
    {
        BlockStore_Freeze(self, SOLIDSYSLOG_BLOCK_STORE_ERROR_SIZE_FAILED);
    }

    return scanned;
}

/* Nothing reads the write block past its first unsent record before the next
 * write appends after its last, so the block has to end on a record boundary
 * now, or the write would land inside a record a power cut cut short. The scan
 * has walked the read block as far as the cursor; a write block the read has
 * not reached is walked whole. */
static void BlockStore_CheckWriteBlock(struct SolidSyslogBlockStore* self)
{
    size_t from = SolidSyslogBlockSequence_ReadIsBehindWrite(self->BlockSequence)
                      ? 0U
                      : SolidSyslogBlockSequence_ReadCursor(self->BlockSequence);
    enum SolidSyslogRecordStoreScanResult scanned = SolidSyslogRecordStore_ScanToEnd(
        self->RecordStore,
        SolidSyslogBlockSequence_BlockDevice(self->BlockSequence),
        SolidSyslogBlockSequence_WriteSequence(self->BlockSequence),
        from,
        SolidSyslogBlockSequence_WritePosition(self->BlockSequence)
    );

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
        /* Whole to the end: the next write appends after the last record. */
    }
}

/* A device that has failed once is not trusted with audit records again. The
 * store takes on the NullStore vtable through Cleanup, so it never calls the
 * device again: what it held stays on the media for the next start, and
 * Service sends new records directly because the store is now transient. The
 * slot is still in use, so Destroy releases it as usual. */
static void BlockStore_Freeze(struct SolidSyslogBlockStore* self, enum SolidSyslogBlockStoreErrors failedOperation)
{
    BlockStore_Report(SOLIDSYSLOG_SEVERITY_ERROR, SOLIDSYSLOG_CAT_STORE_FROZEN, failedOperation);
    SolidSyslogBlockStore_Cleanup(&self->Base);
}

static bool BlockStore_StoreRecord(struct SolidSyslogBlockStore* self, const void* data, size_t size);
static bool BlockStore_FreezeIfDeviceFailed(struct SolidSyslogBlockStore* self);
static inline void BlockStore_ReportSealFailed(void);

static bool BlockStore_Write(struct SolidSyslogStore* base, const void* data, size_t size)
{
    bool written = false;

    /* A record of no bytes would read back as nothing unsent and jam the cursor. */
    if (size > 0U)
    {
        written = BlockStore_StoreRecord(BlockStore_SelfFromBase(base), data, size);
    }

    return written;
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
static bool BlockStore_StepPast(struct SolidSyslogBlockStore* self, enum SolidSyslogRecordStoreReadResult result);
static bool BlockStore_DiscardCurrent(struct SolidSyslogBlockStore* self);
static void BlockStore_ReportLoss(struct SolidSyslogBlockStore* self, enum SolidSyslogBlockStoreErrors fault);
static bool BlockStore_ReadNextBlockIfBehind(struct SolidSyslogBlockStore* self);

static bool BlockStore_ReadNextUnsent(struct SolidSyslogStore* base, void* data, size_t maxSize, size_t* bytesRead)
{
    struct SolidSyslogBlockStore* self = BlockStore_SelfFromBase(base);
    *bytesRead = 0;

    if (SolidSyslogBlockSequence_HasUnsent(self->BlockSequence))
    {
        enum SolidSyslogRecordStoreReadResult result = BlockStore_ReadCurrent(self, data, maxSize, bytesRead);

        while (BlockStore_StepPast(self, result))
        {
            result = BlockStore_ReadCurrent(self, data, maxSize, bytesRead);
        }

        if (result == SOLIDSYSLOG_RECORD_STORE_READ_SUCCEEDED)
        {
            /* A record that reads cleanly ends a run of lost ones. */
            self->LossReported = false;
        }
        else if (result == SOLIDSYSLOG_RECORD_STORE_READ_DEVICE_FAILED)
        {
            /* A block size the sequence could not read is noted there and named
             * by it; any other failure is the read of the record itself. */
            if (!BlockStore_FreezeIfDeviceFailed(self))
            {
                BlockStore_Freeze(self, SOLIDSYSLOG_BLOCK_STORE_ERROR_READ_FAILED);
            }
        }
        else
        {
            /* Nothing left to read. */
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
    size_t readBlockSize = 0U;
    enum SolidSyslogRecordStoreReadResult result = SOLIDSYSLOG_RECORD_STORE_READ_DEVICE_FAILED;

    if (SolidSyslogBlockSequence_ReadBlockSize(self->BlockSequence, &readBlockSize))
    {
        result = SolidSyslogRecordStore_Read(
            self->RecordStore,
            SolidSyslogBlockSequence_BlockDevice(self->BlockSequence),
            SolidSyslogBlockSequence_ReadSequence(self->BlockSequence),
            SolidSyslogBlockSequence_ReadCursor(self->BlockSequence),
            readBlockSize,
            data,
            maxSize,
            bytesRead
        );
    }

    return result;
}

/* Moves the cursor past what the read met, where there is something to move
 * past: a record the policy rejected, the end of a block the write has left
 * behind, or damage. Says whether there is a next record to try. */
static bool BlockStore_StepPast(struct SolidSyslogBlockStore* self, enum SolidSyslogRecordStoreReadResult result)
{
    bool stepped = false;

    if (result == SOLIDSYSLOG_RECORD_STORE_READ_REJECTED)
    {
        stepped = BlockStore_DiscardCurrent(self);
    }
    else if (result == SOLIDSYSLOG_RECORD_STORE_READ_END_OF_DATA)
    {
        stepped = BlockStore_ReadNextBlockIfBehind(self);
    }
    else if (result == SOLIDSYSLOG_RECORD_STORE_READ_CORRUPT)
    {
        stepped = BlockStore_AbandonRestOfReadBlock(self);
    }
    else
    {
        /* A record to hand back, or a device failure. */
    }

    return stepped;
}

/* Reads on in the next block where the write has left this one behind; at the
 * end of the write block there is nothing more. */
static bool BlockStore_ReadNextBlockIfBehind(struct SolidSyslogBlockStore* self)
{
    bool behind = SolidSyslogBlockSequence_ReadIsBehindWrite(self->BlockSequence);

    if (behind)
    {
        SolidSyslogBlockSequence_AdvanceToNextReadBlock(self->BlockSequence);
        SolidSyslogRecordStore_ForgetLastRead(self->RecordStore);
    }

    return behind;
}

/* The rest of the read block cannot be read. Behind the write block the next
 * block is read instead. In the write block nothing unsent remains, and the
 * block is closed so the next write starts a new one rather than appending
 * after the damage. */
static bool BlockStore_AbandonRestOfReadBlock(struct SolidSyslogBlockStore* self)
{
    bool movedOn = BlockStore_ReadNextBlockIfBehind(self);

    BlockStore_ReportLoss(self, SOLIDSYSLOG_BLOCK_STORE_ERROR_RECORD_CORRUPT);

    if (!movedOn)
    {
        SolidSyslogBlockSequence_SetReadCursor(
            self->BlockSequence,
            SolidSyslogBlockSequence_WritePosition(self->BlockSequence)
        );
        SolidSyslogBlockSequence_MarkWriteBlockCorrupt(self->BlockSequence);
    }

    return movedOn;
}

/* Marks the rejected record on the media so nothing offers it again, and moves
 * the cursor to the record after it. A mark that will not write is a media
 * fault: the store freezes, and there is no next record to try. */
static bool BlockStore_DiscardCurrent(struct SolidSyslogBlockStore* self)
{
    size_t nextCursor = 0U;
    enum SolidSyslogRecordStoreMarkResult marked = SolidSyslogRecordStore_DiscardLastRead(
        self->RecordStore,
        SolidSyslogBlockSequence_BlockDevice(self->BlockSequence),
        &nextCursor
    );
    bool discarded = marked == SOLIDSYSLOG_RECORD_STORE_MARK_SUCCEEDED;

    if (discarded)
    {
        BlockStore_ReportLoss(self, SOLIDSYSLOG_BLOCK_STORE_ERROR_RECORD_REJECTED);
        SolidSyslogBlockSequence_SetReadCursor(self->BlockSequence, nextCursor);
    }
    else
    {
        BlockStore_Freeze(self, SOLIDSYSLOG_BLOCK_STORE_ERROR_DISCARD_FAILED);
    }

    return discarded;
}

/* One report stands for a whole run of records lost one after another; the
 * first of them says what went wrong. */
static void BlockStore_ReportLoss(struct SolidSyslogBlockStore* self, enum SolidSyslogBlockStoreErrors fault)
{
    if (!self->LossReported)
    {
        BlockStore_Report(SOLIDSYSLOG_SEVERITY_ERROR, SOLIDSYSLOG_CAT_STORE_RECORD_DISCARDED, fault);
        self->LossReported = true;
    }
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
