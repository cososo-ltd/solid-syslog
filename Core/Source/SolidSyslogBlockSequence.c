/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogBlockSequencePrivate.h"

#include "SolidSyslogBlockDevice.h"
#include "SolidSyslogBlockStore.h"
#include "SolidSyslogBlockStoreErrors.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct SolidSyslogBlockDevice;

enum
{
    MIN_MAX_BLOCKS = 2U,
    SEQUENCE_MODULUS = 100U,
    /* Two short of the sequence space: a failed eviction leaves one block over
     * the ceiling, and a resume still needs an absent number to find the run. */
    MAX_MAX_BLOCKS = SEQUENCE_MODULUS - 2U
};

static inline uint8_t BlockSequence_NextSequence(uint8_t current)
{
    return (uint8_t) ((current + 1U) % SEQUENCE_MODULUS);
}

static inline size_t BlockSequence_BlockCount(const struct SolidSyslogBlockSequence* blockSequence)
{
    return (size_t) ((blockSequence->WriteSequence - blockSequence->OldestSequence + SEQUENCE_MODULUS) %
                     SEQUENCE_MODULUS) +
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

void SolidSyslogBlockSequence_Initialise(
    struct SolidSyslogBlockSequence* blockSequence,
    const struct SolidSyslogBlockSequenceConfig* config
)
{
    blockSequence->BlockDevice = config->BlockDevice;
    blockSequence->MaxBlockSize = config->MaxBlockSize;
    blockSequence->MaxBlocks = BlockSequence_ClampToRange(config->MaxBlocks, MIN_MAX_BLOCKS, MAX_MAX_BLOCKS);
    blockSequence->DiscardPolicy = config->DiscardPolicy;
    blockSequence->OnStoreFull = config->OnStoreFull;
    blockSequence->StoreFullContext = config->StoreFullContext;
    blockSequence->GetCapacityThreshold = config->GetCapacityThreshold;
    blockSequence->OnThresholdCrossed = config->OnThresholdCrossed;
    blockSequence->ThresholdContext = config->ThresholdContext;
    blockSequence->Halted = false;
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
    blockSequence->FailedOperation = SOLIDSYSLOG_BLOCK_STORE_ERROR_MAX;
}

void SolidSyslogBlockSequence_Cleanup(struct SolidSyslogBlockSequence* blockSequence)
{
    /* No owned resources to release. The BlockDevice pointer is caller-owned
     * (integrator-supplied via BlockSequenceConfig.BlockDevice); the next
     * SolidSyslogBlockSequence_Initialise overwrites every field, so clearing here would be churn. */
    (void) blockSequence;
}

static bool BlockSequence_ScanForExistingBlocks(struct SolidSyslogBlockSequence* blockSequence);
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

enum
{
    MAX_SEQUENCE = SEQUENCE_MODULUS
};

struct BlockPresence
{
    bool Present[MAX_SEQUENCE];
    bool FoundAny;
    bool FoundAbsent;
    int FirstAbsent;
};

static inline int BlockSequence_CircularPrev(int index);
static inline int BlockSequence_CircularNext(int index);
static void BlockSequence_ScanForBlockPresence(
    struct SolidSyslogBlockSequence* blockSequence,
    struct BlockPresence* presence
);
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) -- oldest / newest are positional run endpoints; distinct semantics
static void BlockSequence_LocateRunBoundaries(const struct BlockPresence* presence, int* oldest, int* newest);

static inline void BlockSequence_StartReadingBlock(
    struct SolidSyslogBlockSequence* blockSequence,
    uint8_t readSequence
);

/* The on-disk block set is a single contiguous run in the circular sequence
 * space [0, MAX_SEQUENCE). After enough rotations the run straddles the
 * 99 -> 00 boundary (e.g. {98, 99, 0, 1}). Naive lowest=oldest /
 * highest=write mis-classifies wrapped runs, so locate the gap (the absent
 * blocks) and read the run as the complement: oldest sits one past the gap
 * end, write sits one before the gap start. */
static bool BlockSequence_ScanForExistingBlocks(struct SolidSyslogBlockSequence* blockSequence)
{
    struct BlockPresence presence;
    BlockSequence_ScanForBlockPresence(blockSequence, &presence);

    if (presence.FoundAny)
    {
        int oldest = 0;
        int newest = MAX_SEQUENCE - 1;
        BlockSequence_LocateRunBoundaries(&presence, &oldest, &newest);

        blockSequence->OldestSequence = (uint8_t) oldest;
        BlockSequence_StartReadingBlock(blockSequence, (uint8_t) oldest);
        blockSequence->WriteSequence = (uint8_t) newest;
    }

    return presence.FoundAny;
}

/* The cursor goes to the start of the block, and a size cached for the block
 * it leaves is forgotten. */
static inline void BlockSequence_StartReadingBlock(struct SolidSyslogBlockSequence* blockSequence, uint8_t readSequence)
{
    blockSequence->ReadSequence = readSequence;
    blockSequence->ReadCursor = 0;
    blockSequence->ReadBlockSizeKnown = false;
}

static void BlockSequence_ScanForBlockPresence(
    struct SolidSyslogBlockSequence* blockSequence,
    struct BlockPresence* presence
)
{
    presence->FoundAny = false;
    presence->FoundAbsent = false;
    presence->FirstAbsent = 0;

    for (int seq = 0; seq < MAX_SEQUENCE; seq++)
    {
        presence->Present[seq] = SolidSyslogBlockDevice_Exists(blockSequence->BlockDevice, (size_t) seq);

        if (presence->Present[seq])
        {
            presence->FoundAny = true;
        }
        else if (!presence->FoundAbsent)
        {
            presence->FirstAbsent = seq;
            presence->FoundAbsent = true;
        }
        else
        {
            /* present run already closed - nothing to record */
        }
    }
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) -- oldest / newest are positional run endpoints; distinct semantics
static void BlockSequence_LocateRunBoundaries(const struct BlockPresence* presence, int* oldest, int* newest)
{
    if (presence->FoundAbsent)
    {
        *oldest = BlockSequence_CircularNext(presence->FirstAbsent);
        while (!presence->Present[*oldest])
        {
            *oldest = BlockSequence_CircularNext(*oldest);
        }

        *newest = BlockSequence_CircularPrev(presence->FirstAbsent);
        while (!presence->Present[*newest])
        {
            *newest = BlockSequence_CircularPrev(*newest);
        }
    }
    /* else: every block is present - maxBlocks is clamped to MAX_SEQUENCE - 1
     * so this cannot arise from the library's own rotation. Caller's defaults
     * for oldest=0, newest=MAX_SEQUENCE-1 stand. */
}

static inline int BlockSequence_CircularNext(int index)
{
    return (index + 1) % MAX_SEQUENCE;
}

static inline int BlockSequence_CircularPrev(int index)
{
    return (index + MAX_SEQUENCE - 1) % MAX_SEQUENCE;
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
    return (blockSequence->OnThresholdCrossed != NULL) && (blockSequence->GetCapacityThreshold != NULL);
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
static inline void BlockSequence_NotifyStoreFull(struct SolidSyslogBlockSequence* blockSequence);
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
        blockSequence->AtCapacity = true; /* sticky 100% - fixes UsedBytes at total */
        BlockSequence_NotifyThresholdCrossed(blockSequence); /* threshold first per S05.09 ordering */
        BlockSequence_NotifyStoreFull(blockSequence);
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

static inline bool BlockSequence_StoreIsFull(const struct SolidSyslogBlockSequence* blockSequence)
{
    return (BlockSequence_BlockCount(blockSequence) >= blockSequence->MaxBlocks) &&
           (blockSequence->DiscardPolicy != SOLIDSYSLOG_DISCARD_POLICY_OLDEST);
}

static inline void BlockSequence_NotifyStoreFull(struct SolidSyslogBlockSequence* blockSequence)
{
    if ((blockSequence->DiscardPolicy == SOLIDSYSLOG_DISCARD_POLICY_HALT) && !blockSequence->Halted)
    {
        blockSequence->Halted = true;

        if (blockSequence->OnStoreFull != NULL)
        {
            blockSequence->OnStoreFull(blockSequence->StoreFullContext);
        }
    }
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

        if (BlockSequence_ExceedsMaxBlocks(blockSequence))
        {
            *readBlockChanged = BlockSequence_DiscardOldestBlock(blockSequence);
        }

        /* Sealing the prior write block can re-arm dispose-on-empty: drain that
         * fully MarkSent the block before rotation could not fire here because
         * BlockSequence_IsReadBlockActiveWrite was true at MarkSent time. Re-evaluate now,
         * unless the eviction failed - a failed device is not called again. */
        if (!blockSequence->DeviceFailed)
        {
            bool disposedAfterRotate = false;
            SolidSyslogBlockSequence_DisposeReadBlockIfDrained(blockSequence, &disposedAfterRotate);
            *readBlockChanged = *readBlockChanged || disposedAfterRotate;
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
    struct SolidSyslogBlockDevice* device = blockSequence->BlockDevice;
    bool ready = true;

    if (SolidSyslogBlockDevice_Exists(device, blockIndex))
    {
        ready = SolidSyslogBlockDevice_Dispose(device, blockIndex);

        if (!ready)
        {
            BlockSequence_NoteDeviceFailure(blockSequence, SOLIDSYSLOG_BLOCK_STORE_ERROR_DISPOSE_FAILED);
        }
    }

    if (ready)
    {
        ready = SolidSyslogBlockDevice_Acquire(device, blockIndex);

        if (!ready)
        {
            BlockSequence_NoteDeviceFailure(blockSequence, SOLIDSYSLOG_BLOCK_STORE_ERROR_ROTATE_FAILED);
        }
    }

    return ready;
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

static bool BlockSequence_DiscardOldestBlock(struct SolidSyslogBlockSequence* blockSequence)
{
    bool readBlockChanged = false;

    if (SolidSyslogBlockDevice_Dispose(blockSequence->BlockDevice, blockSequence->OldestSequence))
    {
        bool readingOldestBlock = (blockSequence->ReadSequence == blockSequence->OldestSequence);
        blockSequence->OldestSequence = BlockSequence_NextSequence(blockSequence->OldestSequence);

        if (readingOldestBlock)
        {
            BlockSequence_ResetReadToOldest(blockSequence);
            readBlockChanged = true;
        }
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

struct SolidSyslogBlockDevice* SolidSyslogBlockSequence_BlockDevice(const struct SolidSyslogBlockSequence* blockSequence
)
{
    return blockSequence->BlockDevice;
}

size_t SolidSyslogBlockSequence_WriteSequence(const struct SolidSyslogBlockSequence* blockSequence)
{
    return blockSequence->WriteSequence;
}

void SolidSyslogBlockSequence_NoteRecordWritten(struct SolidSyslogBlockSequence* blockSequence, size_t recordSize)
{
    blockSequence->WritePosition += recordSize;
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

static inline bool BlockSequence_IsReadBlockOldest(const struct SolidSyslogBlockSequence* blockSequence);
static inline bool BlockSequence_IsReadBlockFullyDrained(
    const struct SolidSyslogBlockSequence* blockSequence,
    size_t readBlockSize
);
static inline void BlockSequence_AdvancePastDrainedReadBlock(struct SolidSyslogBlockSequence* blockSequence);

void SolidSyslogBlockSequence_DisposeReadBlockIfDrained(
    struct SolidSyslogBlockSequence* blockSequence,
    bool* readBlockChanged
)
{
    *readBlockChanged = false;

    if (!BlockSequence_IsReadBlockActiveWrite(blockSequence) && BlockSequence_IsReadBlockOldest(blockSequence))
    {
        size_t readBlockSize = 0U;

        if (SolidSyslogBlockSequence_ReadBlockSize(blockSequence, &readBlockSize) &&
            BlockSequence_IsReadBlockFullyDrained(blockSequence, readBlockSize))
        {
            if (SolidSyslogBlockDevice_Dispose(blockSequence->BlockDevice, blockSequence->ReadSequence))
            {
                BlockSequence_AdvancePastDrainedReadBlock(blockSequence);
                *readBlockChanged = true;
            }
            else
            {
                BlockSequence_NoteDeviceFailure(blockSequence, SOLIDSYSLOG_BLOCK_STORE_ERROR_DISPOSE_FAILED);
            }
        }
    }
}

static inline bool BlockSequence_IsReadBlockOldest(const struct SolidSyslogBlockSequence* blockSequence)
{
    return blockSequence->ReadSequence == blockSequence->OldestSequence;
}

static inline bool BlockSequence_IsReadBlockFullyDrained(
    const struct SolidSyslogBlockSequence* blockSequence,
    size_t readBlockSize
)
{
    return blockSequence->ReadCursor >= readBlockSize;
}

static inline void BlockSequence_AdvancePastDrainedReadBlock(struct SolidSyslogBlockSequence* blockSequence)
{
    blockSequence->OldestSequence = BlockSequence_NextSequence(blockSequence->OldestSequence);
    BlockSequence_StartReadingBlock(blockSequence, blockSequence->OldestSequence);
    BlockSequence_NotifyThresholdCrossed(blockSequence);
}

void SolidSyslogBlockSequence_AdvanceToNextReadBlock(struct SolidSyslogBlockSequence* blockSequence)
{
    BlockSequence_StartReadingBlock(blockSequence, BlockSequence_NextSequence(blockSequence->ReadSequence));
}

bool SolidSyslogBlockSequence_ReadIsBehindWrite(const struct SolidSyslogBlockSequence* blockSequence)
{
    return blockSequence->ReadSequence != blockSequence->WriteSequence;
}

bool SolidSyslogBlockSequence_DeviceFailed(const struct SolidSyslogBlockSequence* blockSequence)
{
    return blockSequence->DeviceFailed;
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

bool SolidSyslogBlockSequence_IsHalted(const struct SolidSyslogBlockSequence* blockSequence)
{
    return blockSequence->Halted;
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
