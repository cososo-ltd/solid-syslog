/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogBlockSequencePrivate.h"

#include "SolidSyslogBlockDevice.h"
#include "SolidSyslogBlockStore.h"
#include "SolidSyslogBlockStoreErrors.h"
#include "SolidSyslogError.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogStoreCategories.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct SolidSyslogBlockDevice;

enum
{
    MIN_MAX_BLOCKS = 2U,
    /* Two short of the sequence space: a failed eviction leaves one block over
     * the ceiling, and a resume still needs an absent number to find the run. */
    MAX_MAX_BLOCKS = BLOCK_SEQUENCE_MODULUS - 2U
};

static inline uint8_t BlockSequence_NextSequence(uint8_t current)
{
    return (uint8_t) ((current + 1U) % BLOCK_SEQUENCE_MODULUS);
}

static inline size_t BlockSequence_BlockCount(const struct SolidSyslogBlockSequence* blockSequence)
{
    return (size_t) ((blockSequence->WriteSequence - blockSequence->OldestSequence + BLOCK_SEQUENCE_MODULUS) %
                     BLOCK_SEQUENCE_MODULUS) +
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
    blockSequence->MaxBlockSize = config->MaxBlockSize;
    blockSequence->MaxBlocks = BlockSequence_ClampToRange(config->MaxBlocks, MIN_MAX_BLOCKS, MAX_MAX_BLOCKS);
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
    blockSequence->ReadCursor = 0;
    blockSequence->ReadBlockSize = 0;
    blockSequence->ReadBlockSizeKnown = false;
    blockSequence->WritePosition = 0;
    blockSequence->WriteBlockCorrupt = false;
    blockSequence->DeviceFailed = false;
    blockSequence->EvictedUnsent = false;
    blockSequence->FailedOperation = SOLIDSYSLOG_BLOCK_STORE_ERROR_MAX;

    for (size_t seq = 0; seq < BLOCK_SEQUENCE_MODULUS; seq++)
    {
        blockSequence->BlockPresent[seq] = false;
    }
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

static bool BlockSequence_ScanForExistingBlocks(struct SolidSyslogBlockSequence* blockSequence);
static inline bool BlockSequence_AcquireBlock(struct SolidSyslogBlockSequence* blockSequence, size_t blockIndex);
static inline bool BlockSequence_DisposeBlock(struct SolidSyslogBlockSequence* blockSequence, size_t blockIndex);
static inline uint8_t BlockSequence_NextPresentSequence(
    const struct SolidSyslogBlockSequence* blockSequence,
    uint8_t current
);
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
        ready = BlockSequence_AcquireBlock(blockSequence, 0);

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

/* The map follows the device: a block it acquired is present... */
static inline bool BlockSequence_AcquireBlock(struct SolidSyslogBlockSequence* blockSequence, size_t blockIndex)
{
    bool acquired = SolidSyslogBlockDevice_Acquire(blockSequence->BlockDevice, blockIndex);

    if (acquired)
    {
        blockSequence->BlockPresent[blockIndex] = true;
    }

    return acquired;
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
    int Start;
    int Length;
};

static inline int BlockSequence_CircularPrev(int index);
static inline int BlockSequence_CircularNext(int index);
static int BlockSequence_ScanForBlockPresence(struct SolidSyslogBlockSequence* blockSequence);
static void BlockSequence_LocateRunBoundaries(
    const struct SolidSyslogBlockSequence* blockSequence,
    int lastPresent,
    int* oldest, // NOLINT(bugprone-easily-swappable-parameters) -- oldest / newest are positional run endpoints
    int* newest
);

static inline void BlockSequence_StartReadingBlock(
    struct SolidSyslogBlockSequence* blockSequence,
    uint8_t readSequence
);

/* The on-disk block set is one contiguous run in the circular sequence space
 * [0, BLOCK_SEQUENCE_MODULUS), which after enough rotations straddles the
 * 99 -> 00 boundary (e.g. {98, 99, 0, 1}). A block missing from inside the
 * run - a filesystem check removed it - splits the set with a second, smaller
 * gap. So the run is read as the complement of the largest gap: oldest sits
 * one past its end, write one before its start, and the hole stays inside the
 * run, counting towards the block ceiling exactly as the block it replaces
 * would have. That holds while the hole is shorter than the free gap: with
 * the store at its ceiling the free gap is two slots, and a hole of two or
 * more consecutive blocks is then read as the gap instead. */
static bool BlockSequence_ScanForExistingBlocks(struct SolidSyslogBlockSequence* blockSequence)
{
    int lastPresent = BlockSequence_ScanForBlockPresence(blockSequence);
    bool foundAny = lastPresent >= 0;

    if (foundAny)
    {
        int oldest = 0;
        int newest = BLOCK_SEQUENCE_MODULUS - 1;
        BlockSequence_LocateRunBoundaries(blockSequence, lastPresent, &oldest, &newest);

        blockSequence->OldestSequence = (uint8_t) oldest;
        BlockSequence_StartReadingBlock(blockSequence, (uint8_t) oldest);
        blockSequence->WriteSequence = (uint8_t) newest;
    }

    return foundAny;
}

/* The cursor goes to the start of the block, and a size cached for the block
 * it leaves is forgotten. */
static inline void BlockSequence_StartReadingBlock(struct SolidSyslogBlockSequence* blockSequence, uint8_t readSequence)
{
    blockSequence->ReadSequence = readSequence;
    blockSequence->ReadCursor = 0;
    blockSequence->ReadBlockSizeKnown = false;
}

/* Fills the presence map, and says which sequence number holds the last
 * present block - or -1 when there is none. */
static int BlockSequence_ScanForBlockPresence(struct SolidSyslogBlockSequence* blockSequence)
{
    int lastPresent = -1;

    for (int seq = 0; seq < BLOCK_SEQUENCE_MODULUS; seq++)
    {
        blockSequence->BlockPresent[seq] = SolidSyslogBlockDevice_Exists(blockSequence->BlockDevice, (size_t) seq);

        if (blockSequence->BlockPresent[seq])
        {
            lastPresent = seq;
        }
    }

    return lastPresent;
}

/* Walks once round the sequence space from just after a present block, so
 * every gap is met whole, and keeps the largest. Nothing changes when every
 * block is present: maxBlocks is clamped to BLOCK_SEQUENCE_MODULUS - 2, so
 * that cannot arise from the library's own rotation, and the caller's
 * defaults stand. */
static void BlockSequence_LocateRunBoundaries(
    const struct SolidSyslogBlockSequence* blockSequence,
    int lastPresent,
    int* oldest, // NOLINT(bugprone-easily-swappable-parameters) -- oldest / newest are positional run endpoints
    int* newest
)
{
    struct Gap largest = {0, 0};
    struct Gap current = {0, 0};
    int index = lastPresent;

    for (int step = 0; step < BLOCK_SEQUENCE_MODULUS; step++)
    {
        index = BlockSequence_CircularNext(index);

        if (blockSequence->BlockPresent[index])
        {
            current.Length = 0;
        }
        else
        {
            if (current.Length == 0)
            {
                current.Start = index;
            }

            current.Length++;

            if (current.Length > largest.Length)
            {
                largest = current;
            }
        }
    }

    if (largest.Length > 0)
    {
        *oldest = (largest.Start + largest.Length) % BLOCK_SEQUENCE_MODULUS;
        *newest = BlockSequence_CircularPrev(largest.Start);
    }
}

static inline int BlockSequence_CircularNext(int index)
{
    return (index + 1) % BLOCK_SEQUENCE_MODULUS;
}

static inline int BlockSequence_CircularPrev(int index)
{
    return (index + BLOCK_SEQUENCE_MODULUS - 1) % BLOCK_SEQUENCE_MODULUS;
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
            SolidSyslog_Error(
                SOLIDSYSLOG_SEVERITY_WARNING,
                &SolidSyslogBlockStoreErrorSource,
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
        BlockSequence_NotifyThresholdCrossed(blockSequence); /* threshold first per S05.09 ordering */
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
    return (BlockSequence_BlockCount(blockSequence) >= blockSequence->MaxBlocks) &&
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
    uint8_t nextSequence
);
static inline bool BlockSequence_ExceedsMaxBlocks(const struct SolidSyslogBlockSequence* blockSequence);
static inline bool BlockSequence_AcquireEmptyBlock(struct SolidSyslogBlockSequence* blockSequence, size_t blockIndex);

static bool BlockSequence_RotateToNextBlock(struct SolidSyslogBlockSequence* blockSequence, bool* readBlockChanged)
{
    uint8_t nextSequence = BlockSequence_NextSequence(blockSequence->WriteSequence);
    bool acquired = BlockSequence_AcquireEmptyBlock(blockSequence, nextSequence);

    *readBlockChanged = false;

    if (acquired)
    {
        BlockSequence_AdvanceWriteToNewBlock(blockSequence, nextSequence);
        blockSequence->EvictedUnsent = false;

        /* Sealing the prior write block can re-arm dispose-on-empty: drain that
         * fully MarkSent the block before rotation could not fire here because
         * BlockSequence_IsReadBlockActiveWrite was true at MarkSent time. It is
         * re-evaluated before any eviction, so a block with nothing left to send
         * is disposed rather than evicted, and an eviction always costs records. */
        SolidSyslogBlockSequence_DisposeReadBlockIfDrained(blockSequence, readBlockChanged);

        /* A failed device is not called again. */
        if (!blockSequence->DeviceFailed && BlockSequence_ExceedsMaxBlocks(blockSequence))
        {
            *readBlockChanged = BlockSequence_DiscardOldestBlock(blockSequence) || *readBlockChanged;
        }
    }
    else
    {
        /* Only a device failure gets here, and AcquireEmptyBlock has noted it: a
         * full store that keeps what it has never rotates. */
    }

    return acquired;
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
        ready = BlockSequence_DisposeBlock(blockSequence, blockIndex);

        if (!ready)
        {
            BlockSequence_NoteDeviceFailure(blockSequence, SOLIDSYSLOG_BLOCK_STORE_ERROR_DISPOSE_FAILED);
        }
    }

    if (ready)
    {
        ready = BlockSequence_AcquireBlock(blockSequence, blockIndex);

        if (!ready)
        {
            BlockSequence_NoteDeviceFailure(blockSequence, SOLIDSYSLOG_BLOCK_STORE_ERROR_ROTATE_FAILED);
        }
    }

    return ready;
}

/* ...and one it disposed of is not. */
static inline bool BlockSequence_DisposeBlock(struct SolidSyslogBlockSequence* blockSequence, size_t blockIndex)
{
    bool disposed = SolidSyslogBlockDevice_Dispose(blockSequence->BlockDevice, blockIndex);

    if (disposed)
    {
        blockSequence->BlockPresent[blockIndex] = false;
    }

    return disposed;
}

static inline void BlockSequence_AdvanceWriteToNewBlock(
    struct SolidSyslogBlockSequence* blockSequence,
    uint8_t nextSequence
)
{
    blockSequence->WriteSequence = nextSequence;
    blockSequence->WritePosition = 0;
    blockSequence->WriteBlockCorrupt = false;
}

static inline bool BlockSequence_ExceedsMaxBlocks(const struct SolidSyslogBlockSequence* blockSequence)
{
    return BlockSequence_BlockCount(blockSequence) > blockSequence->MaxBlocks;
}

/* The oldest block is the read block - a block the read has finished with is
 * disposed of as the read leaves it - so an eviction always costs records
 * still to send, and the read starts again on the block after it. */
static bool BlockSequence_DiscardOldestBlock(struct SolidSyslogBlockSequence* blockSequence)
{
    bool readBlockChanged = false;

    if (BlockSequence_DisposeBlock(blockSequence, blockSequence->OldestSequence))
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

/* Steps over a hole in the run. Terminates because the write block is always
 * present: the scan ends the run on one, and a rotation marks the one it
 * acquires. */
static inline uint8_t BlockSequence_NextPresentSequence(
    const struct SolidSyslogBlockSequence* blockSequence,
    uint8_t current
)
{
    uint8_t next = BlockSequence_NextSequence(current);

    while (!blockSequence->BlockPresent[next])
    {
        next = BlockSequence_NextSequence(next);
    }

    return next;
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
    bool disposed = BlockSequence_DisposeBlock(blockSequence, blockSequence->ReadSequence);

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
        size_t closedBlocks = BlockSequence_BlockCount(blockSequence) - 1U;
        used = (closedBlocks * blockSequence->MaxBlockSize) + blockSequence->WritePosition;
    }

    return used;
}
