#include "BlockSequence.h"

#include "SolidSyslogBlockDevice.h"

enum
{
    MIN_MAX_BLOCKS = 2,
    MAX_MAX_BLOCKS = 99,
    SEQUENCE_MODULUS = 100
};

static inline uint8_t BlockSequence_NextSequence(uint8_t current)
{
    return (uint8_t) ((current + 1) % SEQUENCE_MODULUS);
}

static inline size_t BlockSequence_BlockCount(const struct BlockSequence* blockSequence)
{
    return (size_t) ((blockSequence->writeSequence - blockSequence->oldestSequence + SEQUENCE_MODULUS) %
                     SEQUENCE_MODULUS) +
           1;
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
    blockSequence->blockDevice = config->blockDevice;
    blockSequence->maxBlockSize = config->maxBlockSize;
    blockSequence->maxBlocks = BlockSequence_ClampToRange(config->maxBlocks, MIN_MAX_BLOCKS, MAX_MAX_BLOCKS);
    blockSequence->discardPolicy = config->discardPolicy;
    blockSequence->onStoreFull = config->onStoreFull;
    blockSequence->storeFullContext = config->storeFullContext;
    blockSequence->getCapacityThreshold = config->getCapacityThreshold;
    blockSequence->onThresholdCrossed = config->onThresholdCrossed;
    blockSequence->thresholdContext = config->thresholdContext;
    blockSequence->halted = false;
    blockSequence->atCapacity = false;
    blockSequence->thresholdCrossed = false;
    blockSequence->oldestSequence = 0;
    blockSequence->readSequence = 0;
    blockSequence->writeSequence = 0;
    blockSequence->readCursor = 0;
    blockSequence->writePosition = 0;
    blockSequence->writeBlockCorrupt = false;
}

static bool BlockSequence_ScanForExistingBlocks(struct BlockSequence* blockSequence);
static inline void BlockSequence_NotifyThresholdCrossed(struct BlockSequence* blockSequence);

bool BlockSequence_Open(struct BlockSequence* blockSequence)
{
    bool foundExistingBlocks = BlockSequence_ScanForExistingBlocks(blockSequence);
    bool ready = false;

    if (foundExistingBlocks)
    {
        blockSequence->writePosition =
            SolidSyslogBlockDevice_Size(blockSequence->blockDevice, blockSequence->writeSequence);
        ready = true;
    }
    else
    {
        ready = SolidSyslogBlockDevice_Acquire(blockSequence->blockDevice, 0);
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
    bool present[MAX_SEQUENCE];
    bool foundAny;
    bool foundAbsent;
    int firstAbsent;
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

    if (presence.foundAny)
    {
        int oldest = 0;
        int newest = MAX_SEQUENCE - 1;
        BlockSequence_LocateRunBoundaries(&presence, &oldest, &newest);

        blockSequence->oldestSequence = (uint8_t) oldest;
        blockSequence->readSequence = (uint8_t) oldest;
        blockSequence->writeSequence = (uint8_t) newest;
    }

    return presence.foundAny;
}

static void BlockSequence_ScanForBlockPresence(struct BlockSequence* blockSequence, struct BlockPresence* presence)
{
    presence->foundAny = false;
    presence->foundAbsent = false;
    presence->firstAbsent = 0;

    for (int seq = 0; seq < MAX_SEQUENCE; seq++)
    {
        presence->present[seq] = SolidSyslogBlockDevice_Exists(blockSequence->blockDevice, (size_t) seq);

        if (presence->present[seq])
        {
            presence->foundAny = true;
        }
        else if (!presence->foundAbsent)
        {
            presence->firstAbsent = seq;
            presence->foundAbsent = true;
        }
    }
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) -- oldest / newest are positional run endpoints; distinct semantics
static void BlockSequence_LocateRunBoundaries(const struct BlockPresence* presence, int* oldest, int* newest)
{
    if (presence->foundAbsent)
    {
        *oldest = BlockSequence_CircularNext(presence->firstAbsent);
        while (!presence->present[*oldest])
        {
            *oldest = BlockSequence_CircularNext(*oldest);
        }

        *newest = BlockSequence_CircularPrev(presence->firstAbsent);
        while (!presence->present[*newest])
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
        blockSequence->atCapacity = true; /* sticky 100% — fixes UsedBytes at total */
        BlockSequence_NotifyThresholdCrossed(blockSequence); /* threshold first per S05.09 ordering */
        BlockSequence_NotifyStoreFull(blockSequence);
        spaceAvailable = false;
    }
    else if (blockFull)
    {
        spaceAvailable = BlockSequence_RotateToNextBlock(blockSequence, readBlockChanged);
    }

    return spaceAvailable;
}

static inline bool BlockSequence_BlockIsFull(const struct BlockSequence* blockSequence, size_t recordSize)
{
    return (blockSequence->writeBlockCorrupt) ||
           ((blockSequence->writePosition + recordSize) > blockSequence->maxBlockSize);
}

static inline bool BlockSequence_StoreIsFull(const struct BlockSequence* blockSequence)
{
    return (BlockSequence_BlockCount(blockSequence) >= blockSequence->maxBlocks) &&
           (blockSequence->discardPolicy != SolidSyslogDiscardPolicy_Oldest);
}

static inline void BlockSequence_NotifyStoreFull(struct BlockSequence* blockSequence)
{
    if ((blockSequence->discardPolicy == SolidSyslogDiscardPolicy_Halt) && !blockSequence->halted)
    {
        blockSequence->halted = true;

        if (blockSequence->onStoreFull != NULL)
        {
            blockSequence->onStoreFull(blockSequence->storeFullContext);
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
    uint8_t nextSequence = BlockSequence_NextSequence(blockSequence->writeSequence);
    bool acquired = BlockSequence_AcquireEmptyBlock(blockSequence->blockDevice, nextSequence);

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
    blockSequence->writeSequence = nextSequence;
    blockSequence->writePosition = 0;
    blockSequence->writeBlockCorrupt = false;
}

static inline bool BlockSequence_ExceedsMaxBlocks(const struct BlockSequence* blockSequence)
{
    return BlockSequence_BlockCount(blockSequence) > blockSequence->maxBlocks;
}

static bool BlockSequence_DiscardOldestBlock(struct BlockSequence* blockSequence)
{
    bool readBlockChanged = false;

    if (SolidSyslogBlockDevice_Dispose(blockSequence->blockDevice, blockSequence->oldestSequence))
    {
        bool readingOldestBlock = (blockSequence->readSequence == blockSequence->oldestSequence);
        blockSequence->oldestSequence = BlockSequence_NextSequence(blockSequence->oldestSequence);

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
    blockSequence->readSequence = blockSequence->oldestSequence;
    blockSequence->readCursor = 0;
}

struct SolidSyslogBlockDevice* BlockSequence_BlockDevice(const struct BlockSequence* blockSequence)
{
    return blockSequence->blockDevice;
}

size_t BlockSequence_WriteSequence(const struct BlockSequence* blockSequence)
{
    return blockSequence->writeSequence;
}

void BlockSequence_NoteRecordWritten(struct BlockSequence* blockSequence, size_t recordSize)
{
    blockSequence->writePosition += recordSize;
    BlockSequence_NotifyThresholdCrossed(blockSequence);
}

static inline bool BlockSequence_ThresholdEnabled(const struct BlockSequence* blockSequence);
static inline bool BlockSequence_IsAboveThreshold(const struct BlockSequence* blockSequence, size_t threshold);

static inline void BlockSequence_NotifyThresholdCrossed(struct BlockSequence* blockSequence)
{
    if (BlockSequence_ThresholdEnabled(blockSequence))
    {
        size_t threshold = blockSequence->getCapacityThreshold(blockSequence->thresholdContext);

        if (!BlockSequence_IsAboveThreshold(blockSequence, threshold))
        {
            blockSequence->thresholdCrossed = false;
        }
        else if (!blockSequence->thresholdCrossed)
        {
            blockSequence->thresholdCrossed = true;
            blockSequence->onThresholdCrossed(blockSequence->thresholdContext);
        }
    }
}

static inline bool BlockSequence_ThresholdEnabled(const struct BlockSequence* blockSequence)
{
    return (blockSequence->onThresholdCrossed != NULL) && (blockSequence->getCapacityThreshold != NULL);
}

static inline bool BlockSequence_IsAboveThreshold(const struct BlockSequence* blockSequence, size_t threshold)
{
    return (threshold != 0) && (BlockSequence_UsedBytes(blockSequence) >= threshold);
}

void BlockSequence_MarkWriteBlockCorrupt(struct BlockSequence* blockSequence)
{
    blockSequence->writeBlockCorrupt = true;
}

size_t BlockSequence_ReadSequence(const struct BlockSequence* blockSequence)
{
    return blockSequence->readSequence;
}

size_t BlockSequence_ReadCursor(const struct BlockSequence* blockSequence)
{
    return blockSequence->readCursor;
}

void BlockSequence_SetReadCursor(struct BlockSequence* blockSequence, size_t cursor)
{
    blockSequence->readCursor = cursor;
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
        if (SolidSyslogBlockDevice_Dispose(blockSequence->blockDevice, blockSequence->readSequence))
        {
            BlockSequence_AdvancePastDrainedReadBlock(blockSequence);
            *readBlockChanged = true;
        }
    }
}

static inline bool BlockSequence_IsReadBlockActiveWrite(const struct BlockSequence* blockSequence)
{
    return blockSequence->readSequence == blockSequence->writeSequence;
}

static inline bool BlockSequence_IsReadBlockOldest(const struct BlockSequence* blockSequence)
{
    return blockSequence->readSequence == blockSequence->oldestSequence;
}

static inline bool BlockSequence_IsReadBlockFullyDrained(const struct BlockSequence* blockSequence)
{
    return blockSequence->readCursor >=
           SolidSyslogBlockDevice_Size(blockSequence->blockDevice, blockSequence->readSequence);
}

static inline void BlockSequence_AdvancePastDrainedReadBlock(struct BlockSequence* blockSequence)
{
    blockSequence->oldestSequence = BlockSequence_NextSequence(blockSequence->oldestSequence);
    blockSequence->readSequence = blockSequence->oldestSequence;
    blockSequence->readCursor = 0;
    BlockSequence_NotifyThresholdCrossed(blockSequence);
}

void BlockSequence_AdvanceToNextReadBlock(struct BlockSequence* blockSequence)
{
    blockSequence->readSequence = BlockSequence_NextSequence(blockSequence->readSequence);
    blockSequence->readCursor = 0;
}

bool BlockSequence_ReadIsBehindWrite(const struct BlockSequence* blockSequence)
{
    return blockSequence->readSequence != blockSequence->writeSequence;
}

bool BlockSequence_HasUnsent(const struct BlockSequence* blockSequence)
{
    return BlockSequence_ReadIsBehindWrite(blockSequence) || (blockSequence->readCursor < blockSequence->writePosition);
}

bool BlockSequence_IsHalted(const struct BlockSequence* blockSequence)
{
    return blockSequence->halted;
}

size_t BlockSequence_TotalBytes(const struct BlockSequence* blockSequence)
{
    return blockSequence->maxBlocks * blockSequence->maxBlockSize;
}

size_t BlockSequence_UsedBytes(const struct BlockSequence* blockSequence)
{
    size_t used = 0;

    if (blockSequence->atCapacity)
    {
        used = BlockSequence_TotalBytes(blockSequence);
    }
    else
    {
        size_t closedBlocks = BlockSequence_BlockCount(blockSequence) - 1;
        used = (closedBlocks * blockSequence->maxBlockSize) + blockSequence->writePosition;
    }

    return used;
}
