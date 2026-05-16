#include "BlockSequence.h"

#include "SolidSyslogBlockDevice.h"

enum
{
    MIN_MAX_BLOCKS = 2U,
    MAX_MAX_BLOCKS = 99U,
    SEQUENCE_MODULUS = 100U
};

static inline uint8_t BlockSequence_NextSequence(uint8_t current)
{
    return (uint8_t) ((current + 1U) % SEQUENCE_MODULUS);
}

static inline size_t BlockSequence_BlockCount(const struct BlockSequence* blockSequence)
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

void BlockSequence_Init(struct BlockSequence* blockSequence, const struct BlockSequenceConfig* config)
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
    blockSequence->WritePosition = 0;
    blockSequence->WriteBlockCorrupt = false;
}

static bool BlockSequence_ScanForExistingBlocks(struct BlockSequence* blockSequence);
static inline void BlockSequence_NotifyThresholdCrossed(struct BlockSequence* blockSequence);

bool BlockSequence_Open(struct BlockSequence* blockSequence)
{
    bool foundExistingBlocks = BlockSequence_ScanForExistingBlocks(blockSequence);
    bool ready = false;

    if (foundExistingBlocks)
    {
        blockSequence->WritePosition =
            SolidSyslogBlockDevice_Size(blockSequence->BlockDevice, blockSequence->WriteSequence);
        ready = true;
    }
    else
    {
        ready = SolidSyslogBlockDevice_Acquire(blockSequence->BlockDevice, 0);
    }

    if (ready)
    {
        BlockSequence_NotifyThresholdCrossed(blockSequence); /* fire on resume if usage already at-or-above threshold */
    }

    return ready;
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
static void BlockSequence_ScanForBlockPresence(struct BlockSequence* blockSequence, struct BlockPresence* presence);
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) -- oldest / newest are positional run endpoints; distinct semantics
static void BlockSequence_LocateRunBoundaries(const struct BlockPresence* presence, int* oldest, int* newest);

/* The on-disk block set is a single contiguous run in the circular sequence
 * space [0, MAX_SEQUENCE). After enough rotations the run straddles the
 * 99 -> 00 boundary (e.g. {98, 99, 0, 1}). Naive lowest=oldest /
 * highest=write mis-classifies wrapped runs, so locate the gap (the absent
 * blocks) and read the run as the complement: oldest sits one past the gap
 * end, write sits one before the gap start. */
static bool BlockSequence_ScanForExistingBlocks(struct BlockSequence* blockSequence)
{
    struct BlockPresence presence;
    BlockSequence_ScanForBlockPresence(blockSequence, &presence);

    if (presence.FoundAny)
    {
        int oldest = 0;
        int newest = MAX_SEQUENCE - 1;
        BlockSequence_LocateRunBoundaries(&presence, &oldest, &newest);

        blockSequence->OldestSequence = (uint8_t) oldest;
        blockSequence->ReadSequence = (uint8_t) oldest;
        blockSequence->WriteSequence = (uint8_t) newest;
    }

    return presence.FoundAny;
}

static void BlockSequence_ScanForBlockPresence(struct BlockSequence* blockSequence, struct BlockPresence* presence)
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
            /* present run already closed — nothing to record */
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
    /* else: every block is present — maxBlocks is clamped to MAX_SEQUENCE - 1
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

static inline bool BlockSequence_BlockIsFull(const struct BlockSequence* blockSequence, size_t recordSize);
static inline bool BlockSequence_StoreIsFull(const struct BlockSequence* blockSequence);
static inline void BlockSequence_NotifyStoreFull(struct BlockSequence* blockSequence);
static bool BlockSequence_RotateToNextBlock(struct BlockSequence* blockSequence, bool* readBlockChanged);

bool BlockSequence_PrepareForWrite(struct BlockSequence* blockSequence, size_t recordSize, bool* readBlockChanged)
{
    bool blockFull = BlockSequence_BlockIsFull(blockSequence, recordSize);
    bool spaceAvailable = true;
    *readBlockChanged = false;

    if (blockFull && BlockSequence_StoreIsFull(blockSequence))
    {
        blockSequence->AtCapacity = true; /* sticky 100% — fixes UsedBytes at total */
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
        /* current block has room — leave spaceAvailable=true */
    }

    return spaceAvailable;
}

static inline bool BlockSequence_BlockIsFull(const struct BlockSequence* blockSequence, size_t recordSize)
{
    return (blockSequence->WriteBlockCorrupt) ||
           ((blockSequence->WritePosition + recordSize) > blockSequence->MaxBlockSize);
}

static inline bool BlockSequence_StoreIsFull(const struct BlockSequence* blockSequence)
{
    return (BlockSequence_BlockCount(blockSequence) >= blockSequence->MaxBlocks) &&
           (blockSequence->DiscardPolicy != SolidSyslogDiscardPolicy_Oldest);
}

static inline void BlockSequence_NotifyStoreFull(struct BlockSequence* blockSequence)
{
    if ((blockSequence->DiscardPolicy == SolidSyslogDiscardPolicy_Halt) && !blockSequence->Halted)
    {
        blockSequence->Halted = true;

        if (blockSequence->OnStoreFull != NULL)
        {
            blockSequence->OnStoreFull(blockSequence->StoreFullContext);
        }
    }
}

static bool BlockSequence_DiscardOldestBlock(struct BlockSequence* blockSequence);
static void BlockSequence_ResetReadToOldest(struct BlockSequence* blockSequence);

static inline void BlockSequence_AdvanceWriteToNewBlock(struct BlockSequence* blockSequence, uint8_t nextSequence);
static inline bool BlockSequence_ExceedsMaxBlocks(const struct BlockSequence* blockSequence);
static inline bool BlockSequence_AcquireEmptyBlock(struct SolidSyslogBlockDevice* device, size_t blockIndex);

static bool BlockSequence_RotateToNextBlock(struct BlockSequence* blockSequence, bool* readBlockChanged)
{
    uint8_t nextSequence = BlockSequence_NextSequence(blockSequence->WriteSequence);
    bool acquired = BlockSequence_AcquireEmptyBlock(blockSequence->BlockDevice, nextSequence);

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
         * BlockSequence_IsReadBlockActiveWrite was true at MarkSent time. Re-evaluate now. */
        bool disposedAfterRotate = false;
        BlockSequence_DisposeReadBlockIfDrained(blockSequence, &disposedAfterRotate);
        *readBlockChanged = *readBlockChanged || disposedAfterRotate;
    }

    return acquired;
}

/* Dispose-then-Acquire enforces the BlockDevice contract that an Acquired block
 * starts empty. Stale content can be left by a crash mid-Append on a previous
 * run, or by a Dispose that succeeded after our state had already advanced
 * past it. Flash drivers depend on this — Acquire = erase, and writing into
 * a non-erased block corrupts data on most flash families.
 *
 * If Dispose fails we surface the failure rather than letting Acquire mask it:
 * a "verify-and-use" flash driver (S18.04 design notes) would Acquire-fail on
 * the still-stale block anyway, and skipping the wasted Acquire keeps callers'
 * retry path (slice 3) symmetric with the dispose-failure path. */
static inline bool BlockSequence_AcquireEmptyBlock(struct SolidSyslogBlockDevice* device, size_t blockIndex)
{
    bool ready = true;

    if (SolidSyslogBlockDevice_Exists(device, blockIndex))
    {
        ready = SolidSyslogBlockDevice_Dispose(device, blockIndex);
    }

    if (ready)
    {
        ready = SolidSyslogBlockDevice_Acquire(device, blockIndex);
    }

    return ready;
}

static inline void BlockSequence_AdvanceWriteToNewBlock(struct BlockSequence* blockSequence, uint8_t nextSequence)
{
    blockSequence->WriteSequence = nextSequence;
    blockSequence->WritePosition = 0;
    blockSequence->WriteBlockCorrupt = false;
}

static inline bool BlockSequence_ExceedsMaxBlocks(const struct BlockSequence* blockSequence)
{
    return BlockSequence_BlockCount(blockSequence) > blockSequence->MaxBlocks;
}

static bool BlockSequence_DiscardOldestBlock(struct BlockSequence* blockSequence)
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

    return readBlockChanged;
}

static void BlockSequence_ResetReadToOldest(struct BlockSequence* blockSequence)
{
    blockSequence->ReadSequence = blockSequence->OldestSequence;
    blockSequence->ReadCursor = 0;
}

struct SolidSyslogBlockDevice* BlockSequence_BlockDevice(const struct BlockSequence* blockSequence)
{
    return blockSequence->BlockDevice;
}

size_t BlockSequence_WriteSequence(const struct BlockSequence* blockSequence)
{
    return blockSequence->WriteSequence;
}

void BlockSequence_NoteRecordWritten(struct BlockSequence* blockSequence, size_t recordSize)
{
    blockSequence->WritePosition += recordSize;
    BlockSequence_NotifyThresholdCrossed(blockSequence);
}

static inline bool BlockSequence_ThresholdEnabled(const struct BlockSequence* blockSequence);
static inline bool BlockSequence_IsAboveThreshold(const struct BlockSequence* blockSequence, size_t threshold);

static inline void BlockSequence_NotifyThresholdCrossed(struct BlockSequence* blockSequence)
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
            /* still above threshold and already notified — no edge to report */
        }
    }
}

static inline bool BlockSequence_ThresholdEnabled(const struct BlockSequence* blockSequence)
{
    return (blockSequence->OnThresholdCrossed != NULL) && (blockSequence->GetCapacityThreshold != NULL);
}

static inline bool BlockSequence_IsAboveThreshold(const struct BlockSequence* blockSequence, size_t threshold)
{
    return (threshold != 0U) && (BlockSequence_UsedBytes(blockSequence) >= threshold);
}

void BlockSequence_MarkWriteBlockCorrupt(struct BlockSequence* blockSequence)
{
    blockSequence->WriteBlockCorrupt = true;
}

size_t BlockSequence_ReadSequence(const struct BlockSequence* blockSequence)
{
    return blockSequence->ReadSequence;
}

size_t BlockSequence_ReadCursor(const struct BlockSequence* blockSequence)
{
    return blockSequence->ReadCursor;
}

void BlockSequence_SetReadCursor(struct BlockSequence* blockSequence, size_t cursor)
{
    blockSequence->ReadCursor = cursor;
}

static inline bool BlockSequence_IsReadBlockActiveWrite(const struct BlockSequence* blockSequence);
static inline bool BlockSequence_IsReadBlockOldest(const struct BlockSequence* blockSequence);
static inline bool BlockSequence_IsReadBlockFullyDrained(const struct BlockSequence* blockSequence);
static inline void BlockSequence_AdvancePastDrainedReadBlock(struct BlockSequence* blockSequence);

void BlockSequence_DisposeReadBlockIfDrained(struct BlockSequence* blockSequence, bool* readBlockChanged)
{
    *readBlockChanged = false;

    if (!BlockSequence_IsReadBlockActiveWrite(blockSequence) && BlockSequence_IsReadBlockOldest(blockSequence) &&
        BlockSequence_IsReadBlockFullyDrained(blockSequence))
    {
        if (SolidSyslogBlockDevice_Dispose(blockSequence->BlockDevice, blockSequence->ReadSequence))
        {
            BlockSequence_AdvancePastDrainedReadBlock(blockSequence);
            *readBlockChanged = true;
        }
    }
}

static inline bool BlockSequence_IsReadBlockActiveWrite(const struct BlockSequence* blockSequence)
{
    return blockSequence->ReadSequence == blockSequence->WriteSequence;
}

static inline bool BlockSequence_IsReadBlockOldest(const struct BlockSequence* blockSequence)
{
    return blockSequence->ReadSequence == blockSequence->OldestSequence;
}

static inline bool BlockSequence_IsReadBlockFullyDrained(const struct BlockSequence* blockSequence)
{
    return blockSequence->ReadCursor >=
           SolidSyslogBlockDevice_Size(blockSequence->BlockDevice, blockSequence->ReadSequence);
}

static inline void BlockSequence_AdvancePastDrainedReadBlock(struct BlockSequence* blockSequence)
{
    blockSequence->OldestSequence = BlockSequence_NextSequence(blockSequence->OldestSequence);
    blockSequence->ReadSequence = blockSequence->OldestSequence;
    blockSequence->ReadCursor = 0;
    BlockSequence_NotifyThresholdCrossed(blockSequence);
}

void BlockSequence_AdvanceToNextReadBlock(struct BlockSequence* blockSequence)
{
    blockSequence->ReadSequence = BlockSequence_NextSequence(blockSequence->ReadSequence);
    blockSequence->ReadCursor = 0;
}

bool BlockSequence_ReadIsBehindWrite(const struct BlockSequence* blockSequence)
{
    return blockSequence->ReadSequence != blockSequence->WriteSequence;
}

bool BlockSequence_HasUnsent(const struct BlockSequence* blockSequence)
{
    return BlockSequence_ReadIsBehindWrite(blockSequence) || (blockSequence->ReadCursor < blockSequence->WritePosition);
}

bool BlockSequence_IsHalted(const struct BlockSequence* blockSequence)
{
    return blockSequence->Halted;
}

size_t BlockSequence_TotalBytes(const struct BlockSequence* blockSequence)
{
    return blockSequence->MaxBlocks * blockSequence->MaxBlockSize;
}

size_t BlockSequence_UsedBytes(const struct BlockSequence* blockSequence)
{
    size_t used = 0;

    if (blockSequence->AtCapacity)
    {
        used = BlockSequence_TotalBytes(blockSequence);
    }
    else
    {
        size_t closedBlocks = BlockSequence_BlockCount(blockSequence) - 1U;
        used = (closedBlocks * blockSequence->MaxBlockSize) + blockSequence->WritePosition;
    }

    return used;
}
