#include "SolidSyslogBlockStore.h"

#include <stdbool.h>

#include "BlockSequencePrivate.h"
#include "RecordStorePrivate.h"
#include "SolidSyslogBlockDevice.h"
#include "SolidSyslogBlockStorePrivate.h"
#include "SolidSyslogNullStore.h"
#include "SolidSyslogStoreDefinition.h"
#include "SolidSyslogTunables.h"

/* vtable — forward-declared because BlockStore_InitialiseVtable references them before their definitions */
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

/* ------------------------------------------------------------------
 * Initialise / Cleanup — private lifecycle pair invoked by Static.c.
 *
 * Initialise stores the two inner pool pointers (Static.c acquired them
 * before this call), wires the vtable, and runs the existing-block
 * resume scan. Cleanup is a pure vtable swap to NullStore for
 * use-after-destroy crash-safety — Static.c destroys the inner pool
 * slots outside the outer FreeIfInUse lock, which keeps the per-pool
 * ConfigLock acquisitions sequential rather than nested.
 * ----------------------------------------------------------------*/

void BlockStore_Initialise(
    struct SolidSyslogStore* base,
    struct RecordStore* recordStore,
    struct BlockSequence* blockSequence,
    const struct SolidSyslogBlockStoreConfig* config
)
{
    (void) config; /* config drove the inner Creates already; nothing left to copy. */
    struct SolidSyslogBlockStore* self = BlockStore_SelfFromBase(base);
    self->RecordStore = recordStore;
    self->BlockSequence = blockSequence;
    BlockStore_InitialiseVtable(self);

    if (BlockSequence_Open(self->BlockSequence))
    {
        BlockStore_ResumeFromExistingBlock(self);
    }
}

void BlockStore_Cleanup(struct SolidSyslogStore* base)
{
    /* Overwrite the abstract base with the shared NullStore vtable so
     * use-after-destroy is a safe no-op rather than a NULL-fn-pointer crash.
     * The inner pool pointers are still in the slot at this point — Static.c
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
    struct SolidSyslogBlockDevice* device = BlockSequence_BlockDevice(self->BlockSequence);
    size_t readSequence = BlockSequence_ReadSequence(self->BlockSequence);
    /* Bound the scan by the read block's actual size, not WritePosition. On a
     * multi-block resume the read block is a closed earlier block whose size
     * is independent of the write block's fill level. */
    size_t readBlockSize = SolidSyslogBlockDevice_Size(device, readSequence);

    bool corrupt = false;
    size_t cursor = RecordStore_FindFirstUnsent(self->RecordStore, device, readSequence, readBlockSize, &corrupt);

    BlockSequence_SetReadCursor(self->BlockSequence, cursor);

    if (corrupt)
    {
        BlockSequence_MarkWriteBlockCorrupt(self->BlockSequence);
    }
}

/* ------------------------------------------------------------------
 * BlockStore_Write
 * ----------------------------------------------------------------*/

static bool BlockStore_StoreRecord(struct SolidSyslogBlockStore* self, const void* data, size_t size);

static bool BlockStore_Write(struct SolidSyslogStore* base, const void* data, size_t size)
{
    return BlockStore_StoreRecord(BlockStore_SelfFromBase(base), data, size);
}

static bool BlockStore_StoreRecord(struct SolidSyslogBlockStore* self, const void* data, size_t size)
{
    size_t recordSize = RecordStore_RecordSize(self->RecordStore, (uint16_t) size);
    bool readBlockChanged = false;
    bool written = false;

    if (BlockSequence_PrepareForWrite(self->BlockSequence, recordSize, &readBlockChanged))
    {
        if (readBlockChanged)
        {
            RecordStore_ForgetLastRead(self->RecordStore);
        }

        if (RecordStore_Append(
                self->RecordStore,
                BlockSequence_BlockDevice(self->BlockSequence),
                BlockSequence_WriteSequence(self->BlockSequence),
                data,
                size
            ))
        {
            BlockSequence_NoteRecordWritten(self->BlockSequence, recordSize);
            written = true;
        }
    }

    return written;
}

/* ------------------------------------------------------------------
 * BlockStore_HasUnsent / BlockStore_IsHalted / BlockStore_GetTotalBytes / BlockStore_GetUsedBytes
 * ----------------------------------------------------------------*/

static bool BlockStore_HasUnsent(struct SolidSyslogStore* base)
{
    return BlockSequence_HasUnsent(BlockStore_SelfFromBase(base)->BlockSequence);
}

static bool BlockStore_IsHalted(struct SolidSyslogStore* base)
{
    return BlockSequence_IsHalted(BlockStore_SelfFromBase(base)->BlockSequence);
}

static size_t BlockStore_GetTotalBytes(struct SolidSyslogStore* base)
{
    return BlockSequence_TotalBytes(BlockStore_SelfFromBase(base)->BlockSequence);
}

static size_t BlockStore_GetUsedBytes(struct SolidSyslogStore* base)
{
    return BlockSequence_UsedBytes(BlockStore_SelfFromBase(base)->BlockSequence);
}

/* BlockStore retains records — a BlockStore_Write rejection here is the discard
 * policy speaking (DISCARD_NEWEST or HALT), and the message must NOT
 * bypass older stored records via a Service direct-send fallback. */
static bool BlockStore_IsTransient(struct SolidSyslogStore* base)
{
    (void) base;
    return false;
}

/* ------------------------------------------------------------------
 * BlockStore_ReadNextUnsent
 * ----------------------------------------------------------------*/

static bool BlockStore_ReadCurrent(struct SolidSyslogBlockStore* self, void* data, size_t maxSize, size_t* bytesRead);

static bool BlockStore_ReadNextUnsent(struct SolidSyslogStore* base, void* data, size_t maxSize, size_t* bytesRead)
{
    struct SolidSyslogBlockStore* self = BlockStore_SelfFromBase(base);
    bool read = false;
    *bytesRead = 0;

    if (BlockSequence_HasUnsent(self->BlockSequence))
    {
        read = BlockStore_ReadCurrent(self, data, maxSize, bytesRead);

        while (!read && BlockSequence_ReadIsBehindWrite(self->BlockSequence))
        {
            BlockSequence_AdvanceToNextReadBlock(self->BlockSequence);
            RecordStore_ForgetLastRead(self->RecordStore);
            read = BlockStore_ReadCurrent(self, data, maxSize, bytesRead);
        }
    }

    return read;
}

static bool BlockStore_ReadCurrent(struct SolidSyslogBlockStore* self, void* data, size_t maxSize, size_t* bytesRead)
{
    return RecordStore_Read(
        self->RecordStore,
        BlockSequence_BlockDevice(self->BlockSequence),
        BlockSequence_ReadSequence(self->BlockSequence),
        BlockSequence_ReadCursor(self->BlockSequence),
        data,
        maxSize,
        bytesRead
    );
}

/* ------------------------------------------------------------------
 * BlockStore_MarkSent
 * ----------------------------------------------------------------*/

static void BlockStore_MarkSent(struct SolidSyslogStore* base)
{
    struct SolidSyslogBlockStore* self = BlockStore_SelfFromBase(base);
    size_t nextCursor = 0;

    if (RecordStore_MarkLastReadAsSent(self->RecordStore, BlockSequence_BlockDevice(self->BlockSequence), &nextCursor))
    {
        BlockSequence_SetReadCursor(self->BlockSequence, nextCursor);

        bool readBlockChanged = false;
        BlockSequence_DisposeReadBlockIfDrained(self->BlockSequence, &readBlockChanged);

        if (readBlockChanged)
        {
            RecordStore_ForgetLastRead(self->RecordStore);
        }
    }
}
