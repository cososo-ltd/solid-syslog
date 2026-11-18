/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogBlockSequencePrivate.h"

#include "SolidSyslogBlockDevice.h"
#include "SolidSyslogBlockStore.h"
#include "SolidSyslogBlockStoreErrors.h"
#include "SolidSyslogBlockStorePrivate.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogStoreCategories.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct SolidSyslogBlockDevice;

enum
{
    MIN_MAX_BLOCKS = 2U,
    /* Kept free below the device's block count: a failed eviction leaves one
     * block over the ceiling, and a resume needs an absent one to find the run. */
    SPARE_BLOCKS = 2U
};

/* The blocks in the run, from the oldest to the write block. */
static inline size_t BlockSequence_BlocksInRun(const struct SolidSyslogBlockSequence* blockSequence)
{
    return (((blockSequence->WriteSequence + blockSequence->DeviceBlocks) - blockSequence->OldestSequence) %
            blockSequence->DeviceBlocks) +
           1U;
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) -- value, min, max have distinct semantics
static inline size_t BlockSequence_ClampToRange(size_t value, size_t min, size_t max)
{
    size_t result = value;

    if (result < min)
    {
        result = min;
    }

    if (result > max)
    {
        result = max;
    }

    return result;
}

static void BlockSequence_NullThresholdCrossed(void* context);

void SolidSyslogBlockSequence_Initialise(
    struct SolidSyslogBlockSequence* blockSequence,
    const struct SolidSyslogBlockSequenceConfig* config
)
{
    blockSequence->BlockDevice = config->BlockDevice;
    blockSequence->DeviceBlocks = config->DeviceBlocks;
    blockSequence->MaxBlockSize = config->MaxBlockSize;
    /* The store has refused a device too small to leave the spare blocks. */
    blockSequence->MaxBlocks =
        BlockSequence_ClampToRange(config->MaxBlocks, MIN_MAX_BLOCKS, config->DeviceBlocks - (size_t) SPARE_BLOCKS);
    blockSequence->GetDiscardPolicy =
        (config->GetDiscardPolicy != NULL) ? config->GetDiscardPolicy : SolidSyslogDiscardPolicy_Default;
    blockSequence->DiscardPolicyContext = config->DiscardPolicyContext;
    blockSequence->GetCapacityThreshold = config->GetCapacityThreshold;
    blockSequence->OnThresholdCrossed =
        (config->OnThresholdCrossed != NULL) ? config->OnThresholdCrossed : BlockSequence_NullThresholdCrossed;
    blockSequence->ThresholdContext = config->ThresholdContext;
    blockSequence->AtCapacity = false;
    blockSequence->ThresholdCrossed = false;
    blockSequence->OldestSequence = 0;
    blockSequence->ReadSequence = 0;
    blockSequence->WriteSequence = 0;
    blockSequence->HolesAhead = 0;
    blockSequence->ReadCursor = 0;
    blockSequence->ReadBlockSize = 0;
    blockSequence->ReadBlockSizeKnown = false;
    blockSequence->WritePosition = 0;
    blockSequence->WriteBlockCorrupt = false;
    blockSequence->DeviceFailed = false;
    blockSequence->EvictedUnsent = false;
    blockSequence->FailedOperation = SOLIDSYSLOG_BLOCK_STORE_ERROR_MAX;
}

/* A threshold with no callback is still raised as an event. */
static void BlockSequence_NullThresholdCrossed(void* context)
{
    (void) context;
}

void SolidSyslogBlockSequence_Cleanup(struct SolidSyslogBlockSequence* blockSequence)
{
    /* No owned resources to release. The BlockDevice pointer is caller-owned
     * (integrator-supplied via BlockSequenceConfig.BlockDevice); the next
     * SolidSyslogBlockSequence_Initialise overwrites every field, so clearing here would be churn. */
    (void) blockSequence;
}

size_t SolidSyslogBlockSequence_MinDeviceBlocks(void)
{
    return (size_t) MIN_MAX_BLOCKS + (size_t) SPARE_BLOCKS;
}

static bool BlockSequence_ScanForExistingBlocks(struct SolidSyslogBlockSequence* blockSequence);
static inline size_t BlockSequence_NextPresentSequence(struct SolidSyslogBlockSequence* blockSequence, size_t current);
static inline void BlockSequence_NoteDeviceFailure(
    struct SolidSyslogBlockSequence* blockSequence,
    enum SolidSyslogBlockStoreErrors failedOperation
);
static inline void BlockSequence_NotifyThresholdCrossed(struct SolidSyslogBlockSequence* blockSequence);

bool SolidSyslogBlockSequence_Open(struct SolidSyslogBlockSequence* blockSequence)
{
    bool foundExistingBlocks = BlockSequence_ScanForExistingBlocks(blockSequence);
    bool ready = false;

    if (foundExistingBlocks)
    {
        size_t writePosition = 0U;
        ready = SolidSyslogBlockDevice_Size(blockSequence->BlockDevice, blockSequence->WriteSequence, &writePosition);
        blockSequence->WritePosition = writePosition;

        if (!ready)
        {
            BlockSequence_NoteDeviceFailure(blockSequence, SOLIDSYSLOG_BLOCK_STORE_ERROR_SIZE_FAILED);
        }
    }
    else
    {
        ready = SolidSyslogBlockDevice_Acquire(blockSequence->BlockDevice, 0);

        if (!ready)
        {
            BlockSequence_NoteDeviceFailure(blockSequence, SOLIDSYSLOG_BLOCK_STORE_ERROR_OPEN_FAILED);
        }
    }

    if (ready)
    {
        BlockSequence_NotifyThresholdCrossed(blockSequence); /* fire on resume if usage already at-or-above threshold */
    }

    return ready;
}

static inline void BlockSequence_NoteDeviceFailure(
    struct SolidSyslogBlockSequence* blockSequence,
    enum SolidSyslogBlockStoreErrors failedOperation
)
{
    if (!blockSequence->DeviceFailed)
    {
        blockSequence->DeviceFailed = true;
        blockSequence->FailedOperation = failedOperation;
    }
}

/* A run of absent sequence numbers: where it starts and how many it spans. */
struct Gap
{
    size_t Start;
    size_t Length;
};

/* What one pass over the device's blocks finds. */
struct Scan
{
    size_t Present; /* how many blocks the device holds */
    size_t FirstPresent;
    size_t LastPresent;
    struct Gap LargestInner; /* the largest gap with a present block on each side of it */
    struct Gap Current; /* the gap the pass is in, of no length while it is in none */
};

static inline size_t BlockSequence_CircularPrev(const struct SolidSyslogBlockSequence* blockSequence, size_t index);
static inline size_t BlockSequence_CircularNext(const struct SolidSyslogBlockSequence* blockSequence, size_t index);
static struct Scan BlockSequence_ScanForBlockPresence(const struct SolidSyslogBlockSequence* blockSequence);
static inline struct Gap BlockSequence_LargestGap(
    const struct SolidSyslogBlockSequence* blockSequence,
    const struct Scan* scan
);

static inline void BlockSequence_StartReadingBlock(struct SolidSyslogBlockSequence* blockSequence, size_t readSequence);

/* The device's block set is one contiguous run in the circular space of its
 * block numbers, which after enough rotations straddles the wrap from the last
 * block to the first (on a device of 100, {98, 99, 0, 1}). A block missing from
 * inside the run - a filesystem check removed it - splits the set with a
 * second, smaller gap. So the run is read as the complement of the largest gap: oldest sits
 * one past its end, write one before its start, and the hole stays inside the
 * run, counting towards the block ceiling exactly as the block it replaces
 * would have. That holds while the hole is shorter than the free gap: with
 * the store at its ceiling the free gap is two slots, and a hole of two or
 * more consecutive blocks is then read as the gap instead.
 *
 * There is no gap when every block is present: maxBlocks is clamped two short
 * of the device's blocks, so that cannot arise from the library's own
 * rotation, and the run is then read as starting at block 0. */
static bool BlockSequence_ScanForExistingBlocks(struct SolidSyslogBlockSequence* blockSequence)
{
    struct Scan scan = BlockSequence_ScanForBlockPresence(blockSequence);
    bool foundAny = scan.Present > 0U;

    if (foundAny)
    {
        struct Gap largest = BlockSequence_LargestGap(blockSequence, &scan);
        size_t oldest = 0U;
        size_t newest = blockSequence->DeviceBlocks - 1U;

        if (largest.Length > 0U)
        {
            oldest = (largest.Start + largest.Length) % blockSequence->DeviceBlocks;
            newest = BlockSequence_CircularPrev(blockSequence, largest.Start);
        }

        blockSequence->OldestSequence = oldest;
        BlockSequence_StartReadingBlock(blockSequence, oldest);
        blockSequence->WriteSequence = newest;
        blockSequence->HolesAhead = BlockSequence_BlocksInRun(blockSequence) - scan.Present;
    }

    return foundAny;
}

/* The cursor goes to the start of the block, and a size cached for the block
 * it leaves is forgotten. */
static inline void BlockSequence_StartReadingBlock(struct SolidSyslogBlockSequence* blockSequence, size_t readSequence)
{
    blockSequence->ReadSequence = readSequence;
    blockSequence->ReadCursor = 0;
    blockSequence->ReadBlockSizeKnown = false;
}

static inline void BlockSequence_NotePresent(struct Scan* scan, size_t sequence);
static inline void BlockSequence_NoteAbsent(struct Scan* scan, size_t sequence);

/* One pass over the device, asking about each block once. */
static struct Scan BlockSequence_ScanForBlockPresence(const struct SolidSyslogBlockSequence* blockSequence)
{
    struct Scan scan = {0U, 0U, 0U, {0U, 0U}, {0U, 0U}};

    for (size_t sequence = 0U; sequence < blockSequence->DeviceBlocks; sequence++)
    {
        if (SolidSyslogBlockDevice_Exists(blockSequence->BlockDevice, sequence))
        {
            BlockSequence_NotePresent(&scan, sequence);
        }
        else
        {
            BlockSequence_NoteAbsent(&scan, sequence);
        }
    }

    return scan;
}

/* A present block closes the gap before it. That gap is an inner one only when
 * a present block came before it too. */
static inline void BlockSequence_NotePresent(struct Scan* scan, size_t sequence)
{
    if (scan->Present == 0U)
    {
        scan->FirstPresent = sequence;
    }
    else if (scan->Current.Length > scan->LargestInner.Length)
    {
        scan->LargestInner = scan->Current;
    }
    else
    {
        /* No gap closed, or one no larger than the largest so far. */
    }

    scan->Present++;
    scan->LastPresent = sequence;
    scan->Current.Length = 0U;
}

static inline void BlockSequence_NoteAbsent(struct Scan* scan, size_t sequence)
{
    if (scan->Current.Length == 0U)
    {
        scan->Current.Start = sequence;
    }

    scan->Current.Length++;
}

/* The absent blocks after the last present one and before the first are one
 * gap, across the wrap.
 *
 * Two gaps of the same length cannot be told apart, and the library's own
 * rotation never leaves two, so which one a tie settles on is arbitrary. */
static inline struct Gap BlockSequence_LargestGap(
    const struct SolidSyslogBlockSequence* blockSequence,
    const struct Scan* scan
)
{
    struct Gap acrossTheWrap = {
        BlockSequence_CircularNext(blockSequence, scan->LastPresent),
        (blockSequence->DeviceBlocks - 1U - scan->LastPresent) + scan->FirstPresent
    };

    return (acrossTheWrap.Length >= scan->LargestInner.Length) ? acrossTheWrap : scan->LargestInner;
}

static inline size_t BlockSequence_CircularNext(const struct SolidSyslogBlockSequence* blockSequence, size_t index)
{
    return (index + 1U) % blockSequence->DeviceBlocks;
}

static inline size_t BlockSequence_CircularPrev(const struct SolidSyslogBlockSequence* blockSequence, size_t index)
{
    return (index + blockSequence->DeviceBlocks - 1U) % blockSequence->DeviceBlocks;
}

static inline bool BlockSequence_ThresholdEnabled(const struct SolidSyslogBlockSequence* blockSequence);
static inline bool BlockSequence_IsAboveThreshold(
    const struct SolidSyslogBlockSequence* blockSequence,
    size_t threshold
);

static inline void BlockSequence_NotifyThresholdCrossed(struct SolidSyslogBlockSequence* blockSequence)
{
    if (BlockSequence_ThresholdEnabled(blockSequence))
    {
        size_t threshold = blockSequence->GetCapacityThreshold(blockSequence->ThresholdContext);

        if (!BlockSequence_IsAboveThreshold(blockSequence, threshold))
        {
            blockSequence->ThresholdCrossed = false;
        }
        else if (!blockSequence->ThresholdCrossed)
        {
            blockSequence->ThresholdCrossed = true;
            BlockStore_Report(
                SOLIDSYSLOG_SEVERITY_WARNING,
                SOLIDSYSLOG_CAT_STORE_THRESHOLD_CROSSED,
                SOLIDSYSLOG_BLOCK_STORE_ERROR_THRESHOLD_CROSSED
            );
            blockSequence->OnThresholdCrossed(blockSequence->ThresholdContext);
        }
        else
        {
            /* still above threshold and already notified - no edge to report */
        }
    }
}

static inline bool BlockSequence_ThresholdEnabled(const struct SolidSyslogBlockSequence* blockSequence)
{
    return blockSequence->GetCapacityThreshold != NULL;
}

static inline bool BlockSequence_IsAboveThreshold(
    const struct SolidSyslogBlockSequence* blockSequence,
    size_t threshold
)
{
    return (threshold != 0U) && (SolidSyslogBlockSequence_UsedBytes(blockSequence) >= threshold);
}

static inline bool BlockSequence_BlockIsFull(const struct SolidSyslogBlockSequence* blockSequence, size_t recordSize);
static inline bool BlockSequence_StoreIsFull(const struct SolidSyslogBlockSequence* blockSequence);
static inline enum SolidSyslogDiscardPolicy BlockSequence_DiscardPolicy(
    const struct SolidSyslogBlockSequence* blockSequence
);
static bool BlockSequence_RotateToNextBlock(struct SolidSyslogBlockSequence* blockSequence, bool* readBlockChanged);

bool SolidSyslogBlockSequence_PrepareForWrite(
    struct SolidSyslogBlockSequence* blockSequence,
    size_t recordSize,
    bool* readBlockChanged
)
{
    bool blockFull = BlockSequence_BlockIsFull(blockSequence, recordSize);
    bool spaceAvailable = true;
    *readBlockChanged = false;

    if (blockFull && BlockSequence_StoreIsFull(blockSequence))
    {
        /* Fixes UsedBytes at total while the store is full and refusing, so a
         * threshold at 100% fires on the refusal even with slack in the blocks.
         * Cleared once a block drains or a record is accepted. */
        blockSequence->AtCapacity = true;
        BlockSequence_NotifyThresholdCrossed(blockSequence); /* ahead of the refusal the store reports next */
        spaceAvailable = false;
    }
    else if (blockFull)
    {
        spaceAvailable = BlockSequence_RotateToNextBlock(blockSequence, readBlockChanged);
    }
    else
    {
        /* current block has room - leave spaceAvailable=true */
    }

    return spaceAvailable;
}

static inline bool BlockSequence_BlockIsFull(const struct SolidSyslogBlockSequence* blockSequence, size_t recordSize)
{
    return (blockSequence->WriteBlockCorrupt) ||
           ((blockSequence->WritePosition + recordSize) > blockSequence->MaxBlockSize);
}

/* The policy is asked only once the store is at its block ceiling, so an
 * integrator's function answers at the moment it has to decide, and nothing
 * earlier. */
static inline bool BlockSequence_StoreIsFull(const struct SolidSyslogBlockSequence* blockSequence)
{
    return (BlockSequence_BlocksInRun(blockSequence) >= blockSequence->MaxBlocks) &&
           (BlockSequence_DiscardPolicy(blockSequence) != SOLIDSYSLOG_DISCARD_POLICY_OLDEST);
}

static inline enum SolidSyslogDiscardPolicy BlockSequence_DiscardPolicy(
    const struct SolidSyslogBlockSequence* blockSequence
)
{
    return blockSequence->GetDiscardPolicy(blockSequence->DiscardPolicyContext);
}

static bool BlockSequence_DiscardOldestBlock(struct SolidSyslogBlockSequence* blockSequence);
static void BlockSequence_ResetReadToOldest(struct SolidSyslogBlockSequence* blockSequence);

static inline void BlockSequence_AdvanceWriteToNewBlock(
    struct SolidSyslogBlockSequence* blockSequence,
    size_t nextSequence
);
static inline bool BlockSequence_ExceedsMaxBlocks(const struct SolidSyslogBlockSequence* blockSequence);
static bool BlockSequence_EvictDownToCeiling(struct SolidSyslogBlockSequence* blockSequence);
static inline bool BlockSequence_AcquireEmptyBlock(struct SolidSyslogBlockSequence* blockSequence, size_t blockIndex);

static bool BlockSequence_RotateToNextBlock(struct SolidSyslogBlockSequence* blockSequence, bool* readBlockChanged)
{
    bool acquired = false;
    bool drained = false;
    bool evicted = false;

    blockSequence->EvictedUnsent = false;

    /* A run resumed over its ceiling, after an eviction that failed, comes
     * down to it first: acquiring before that could take the last absent
     * block, and a start after a power cut there could not find the run. */
    evicted = BlockSequence_EvictDownToCeiling(blockSequence);

    /* A failed device is not called again. */
    if (!blockSequence->DeviceFailed)
    {
        size_t nextSequence = BlockSequence_CircularNext(blockSequence, blockSequence->WriteSequence);
        acquired = BlockSequence_AcquireEmptyBlock(blockSequence, nextSequence);

        if (acquired)
        {
            BlockSequence_AdvanceWriteToNewBlock(blockSequence, nextSequence);

            /* Sealing the prior write block can re-arm dispose-on-empty: drain that
             * fully MarkSent the block before rotation could not fire here because
             * BlockSequence_IsReadBlockActiveWrite was true at MarkSent time. It is
             * re-evaluated before any eviction, so a block with nothing left to send
             * is disposed rather than evicted, and an eviction always costs records. */
            SolidSyslogBlockSequence_DisposeReadBlockIfDrained(blockSequence, &drained);

            if (BlockSequence_EvictDownToCeiling(blockSequence))
            {
                evicted = true;
            }
        }
        else
        {
            /* AcquireEmptyBlock has noted the device failure. */
        }
    }

    *readBlockChanged = drained || evicted;

    return acquired;
}

/* Evicts the oldest block until the run is no longer over its ceiling, or the
 * device fails. Says whether any block went. */
static bool BlockSequence_EvictDownToCeiling(struct SolidSyslogBlockSequence* blockSequence)
{
    bool evicted = false;

    while (!blockSequence->DeviceFailed && BlockSequence_ExceedsMaxBlocks(blockSequence))
    {
        if (BlockSequence_DiscardOldestBlock(blockSequence))
        {
            evicted = true;
        }
    }

    return evicted;
}

/* Dispose-then-Acquire enforces the BlockDevice contract that an Acquired block
 * starts empty. Stale content can be left by a crash mid-Append on a previous
 * run, or by a Dispose that succeeded after our state had already advanced
 * past it. Flash drivers depend on this - Acquire = erase, and writing into
 * a non-erased block corrupts data on most flash families.
 *
 * If Dispose fails, Acquire is not tried: a "verify-and-use" flash driver would
 * reject the still-stale block anyway. The failure is noted as the operation
 * that failed, so a stale block that will not go is a dispose failure. */
static inline bool BlockSequence_AcquireEmptyBlock(struct SolidSyslogBlockSequence* blockSequence, size_t blockIndex)
{
    bool ready = true;

    if (SolidSyslogBlockDevice_Exists(blockSequence->BlockDevice, blockIndex))
    {
        ready = SolidSyslogBlockDevice_Dispose(blockSequence->BlockDevice, blockIndex);

        if (!ready)
        {
            BlockSequence_NoteDeviceFailure(blockSequence, SOLIDSYSLOG_BLOCK_STORE_ERROR_DISPOSE_FAILED);
        }
    }

    if (ready)
    {
        ready = SolidSyslogBlockDevice_Acquire(blockSequence->BlockDevice, blockIndex);

        if (!ready)
        {
            BlockSequence_NoteDeviceFailure(blockSequence, SOLIDSYSLOG_BLOCK_STORE_ERROR_ROTATE_FAILED);
        }
    }

    return ready;
}

static inline void BlockSequence_AdvanceWriteToNewBlock(
    struct SolidSyslogBlockSequence* blockSequence,
    size_t nextSequence
)
{
    blockSequence->WriteSequence = nextSequence;
    blockSequence->WritePosition = 0;
    blockSequence->WriteBlockCorrupt = false;
}

static inline bool BlockSequence_ExceedsMaxBlocks(const struct SolidSyslogBlockSequence* blockSequence)
{
    return BlockSequence_BlocksInRun(blockSequence) > blockSequence->MaxBlocks;
}

/* The oldest block is the read block - a block the read has finished with is
 * disposed of as the read leaves it - so an eviction always costs records
 * still to send, and the read starts again on the block after it. */
static bool BlockSequence_DiscardOldestBlock(struct SolidSyslogBlockSequence* blockSequence)
{
    bool readBlockChanged = false;

    if (SolidSyslogBlockDevice_Dispose(blockSequence->BlockDevice, blockSequence->OldestSequence))
    {
        blockSequence->EvictedUnsent = true;
        blockSequence->OldestSequence = BlockSequence_NextPresentSequence(blockSequence, blockSequence->OldestSequence);
        BlockSequence_ResetReadToOldest(blockSequence);
        readBlockChanged = true;
    }
    else
    {
        BlockSequence_NoteDeviceFailure(blockSequence, SOLIDSYSLOG_BLOCK_STORE_ERROR_DISPOSE_FAILED);
    }

    return readBlockChanged;
}

static void BlockSequence_ResetReadToOldest(struct SolidSyslogBlockSequence* blockSequence)
{
    BlockSequence_StartReadingBlock(blockSequence, blockSequence->OldestSequence);
}

static inline bool BlockSequence_IsHole(const struct SolidSyslogBlockSequence* blockSequence, size_t sequence);

/* Steps over a hole in the run, a block the device no longer has. It stops at
 * the write block whatever the device says of it, so it goes no further than
 * the run does. */
static inline size_t BlockSequence_NextPresentSequence(struct SolidSyslogBlockSequence* blockSequence, size_t current)
{
    size_t next = BlockSequence_CircularNext(blockSequence, current);

    while (BlockSequence_IsHole(blockSequence, next))
    {
        blockSequence->HolesAhead--;
        next = BlockSequence_CircularNext(blockSequence, next);
    }

    return next;
}

/* The device is asked only while a hole the resume found is still ahead: a
 * device that fails to answer would otherwise have its blocks taken for
 * holes. */
static inline bool BlockSequence_IsHole(const struct SolidSyslogBlockSequence* blockSequence, size_t sequence)
{
    return (blockSequence->HolesAhead > 0U) && (sequence != blockSequence->WriteSequence) &&
           !SolidSyslogBlockDevice_Exists(blockSequence->BlockDevice, sequence);
}

struct SolidSyslogBlockDevice* SolidSyslogBlockSequence_BlockDevice(const struct SolidSyslogBlockSequence* blockSequence
)
{
    return blockSequence->BlockDevice;
}

size_t SolidSyslogBlockSequence_WriteSequence(const struct SolidSyslogBlockSequence* blockSequence)
{
    return blockSequence->WriteSequence;
}

size_t SolidSyslogBlockSequence_WritePosition(const struct SolidSyslogBlockSequence* blockSequence)
{
    return blockSequence->WritePosition;
}

void SolidSyslogBlockSequence_NoteRecordWritten(struct SolidSyslogBlockSequence* blockSequence, size_t recordSize)
{
    blockSequence->WritePosition += recordSize;
    blockSequence->AtCapacity = false;
    BlockSequence_NotifyThresholdCrossed(blockSequence);
}

void SolidSyslogBlockSequence_MarkWriteBlockCorrupt(struct SolidSyslogBlockSequence* blockSequence)
{
    blockSequence->WriteBlockCorrupt = true;
}

size_t SolidSyslogBlockSequence_ReadSequence(const struct SolidSyslogBlockSequence* blockSequence)
{
    return blockSequence->ReadSequence;
}

size_t SolidSyslogBlockSequence_ReadCursor(const struct SolidSyslogBlockSequence* blockSequence)
{
    return blockSequence->ReadCursor;
}

void SolidSyslogBlockSequence_SetReadCursor(struct SolidSyslogBlockSequence* blockSequence, size_t cursor)
{
    blockSequence->ReadCursor = cursor;
}

static inline bool BlockSequence_IsReadBlockActiveWrite(const struct SolidSyslogBlockSequence* blockSequence);

/* A size the device could not read is a failed device, never an empty block:
 * read as empty, a block still holding unsent records would look drained. A
 * block behind the write block no longer grows, so its size is read once. */
bool SolidSyslogBlockSequence_ReadBlockSize(struct SolidSyslogBlockSequence* blockSequence, size_t* size)
{
    bool known = true;

    if (BlockSequence_IsReadBlockActiveWrite(blockSequence))
    {
        *size = blockSequence->WritePosition;
    }
    else if (blockSequence->ReadBlockSizeKnown)
    {
        *size = blockSequence->ReadBlockSize;
    }
    else
    {
        known = SolidSyslogBlockDevice_Size(blockSequence->BlockDevice, blockSequence->ReadSequence, size);
        blockSequence->ReadBlockSize = *size;
        blockSequence->ReadBlockSizeKnown = known;

        if (!known)
        {
            BlockSequence_NoteDeviceFailure(blockSequence, SOLIDSYSLOG_BLOCK_STORE_ERROR_SIZE_FAILED);
        }
    }

    return known;
}

static inline bool BlockSequence_IsReadBlockActiveWrite(const struct SolidSyslogBlockSequence* blockSequence)
{
    return blockSequence->ReadSequence == blockSequence->WriteSequence;
}

static inline bool BlockSequence_IsReadBlockFullyDrained(
    const struct SolidSyslogBlockSequence* blockSequence,
    size_t readBlockSize
);
static inline void BlockSequence_AdvancePastDisposedReadBlock(struct SolidSyslogBlockSequence* blockSequence);
static inline bool BlockSequence_DisposeReadBlock(struct SolidSyslogBlockSequence* blockSequence);

/* The read block is the oldest: every block before it was disposed of or
 * evicted as the read left it, so disposing it leaves no hole in the run. The
 * caller has checked the read is behind the write, so a next block exists. */
void SolidSyslogBlockSequence_LeaveReadBlock(struct SolidSyslogBlockSequence* blockSequence)
{
    (void) BlockSequence_DisposeReadBlock(blockSequence);
}

static inline bool BlockSequence_DisposeReadBlock(struct SolidSyslogBlockSequence* blockSequence)
{
    bool disposed = SolidSyslogBlockDevice_Dispose(blockSequence->BlockDevice, blockSequence->ReadSequence);

    if (disposed)
    {
        BlockSequence_AdvancePastDisposedReadBlock(blockSequence);
    }
    else
    {
        BlockSequence_NoteDeviceFailure(blockSequence, SOLIDSYSLOG_BLOCK_STORE_ERROR_DISPOSE_FAILED);
    }

    return disposed;
}

static inline void BlockSequence_AdvancePastDisposedReadBlock(struct SolidSyslogBlockSequence* blockSequence)
{
    blockSequence->OldestSequence = BlockSequence_NextPresentSequence(blockSequence, blockSequence->OldestSequence);
    BlockSequence_StartReadingBlock(blockSequence, blockSequence->OldestSequence);
    blockSequence->AtCapacity = false;
    BlockSequence_NotifyThresholdCrossed(blockSequence);
}

void SolidSyslogBlockSequence_DisposeReadBlockIfDrained(
    struct SolidSyslogBlockSequence* blockSequence,
    bool* readBlockChanged
)
{
    *readBlockChanged = false;

    if (!BlockSequence_IsReadBlockActiveWrite(blockSequence))
    {
        size_t readBlockSize = 0U;

        if (SolidSyslogBlockSequence_ReadBlockSize(blockSequence, &readBlockSize) &&
            BlockSequence_IsReadBlockFullyDrained(blockSequence, readBlockSize))
        {
            *readBlockChanged = BlockSequence_DisposeReadBlock(blockSequence);
        }
    }
}

static inline bool BlockSequence_IsReadBlockFullyDrained(
    const struct SolidSyslogBlockSequence* blockSequence,
    size_t readBlockSize
)
{
    return blockSequence->ReadCursor >= readBlockSize;
}

bool SolidSyslogBlockSequence_ReadIsBehindWrite(const struct SolidSyslogBlockSequence* blockSequence)
{
    return blockSequence->ReadSequence != blockSequence->WriteSequence;
}

bool SolidSyslogBlockSequence_DeviceFailed(const struct SolidSyslogBlockSequence* blockSequence)
{
    return blockSequence->DeviceFailed;
}

bool SolidSyslogBlockSequence_EvictedUnsent(const struct SolidSyslogBlockSequence* blockSequence)
{
    return blockSequence->EvictedUnsent;
}

enum SolidSyslogBlockStoreErrors SolidSyslogBlockSequence_FailedOperation(
    const struct SolidSyslogBlockSequence* blockSequence
)
{
    return blockSequence->FailedOperation;
}

bool SolidSyslogBlockSequence_HasUnsent(const struct SolidSyslogBlockSequence* blockSequence)
{
    return SolidSyslogBlockSequence_ReadIsBehindWrite(blockSequence) ||
           (blockSequence->ReadCursor < blockSequence->WritePosition);
}

size_t SolidSyslogBlockSequence_TotalBytes(const struct SolidSyslogBlockSequence* blockSequence)
{
    return blockSequence->MaxBlocks * blockSequence->MaxBlockSize;
}

size_t SolidSyslogBlockSequence_UsedBytes(const struct SolidSyslogBlockSequence* blockSequence)
{
    size_t used = 0U;

    if (blockSequence->AtCapacity)
    {
        used = SolidSyslogBlockSequence_TotalBytes(blockSequence);
    }
    else
    {
        size_t closedBlocks = BlockSequence_BlocksInRun(blockSequence) - 1U;
        used = (closedBlocks * blockSequence->MaxBlockSize) + blockSequence->WritePosition;
    }

    return used;
}
