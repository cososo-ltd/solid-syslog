#include "BlockSequence.h"

#include "SolidSyslogBlockDevice.h"

enum
{
    MIN_MAX_FILES    = 2,
    MAX_MAX_FILES    = 99,
    SEQUENCE_MODULUS = 100
};

static inline uint8_t NextSequence(uint8_t current)
{
    return (uint8_t) ((current + 1) % SEQUENCE_MODULUS);
}

static inline size_t BlockCount(const struct BlockSequence* blockSequence)
{
    return (size_t) ((blockSequence->writeSequence - blockSequence->oldestSequence + SEQUENCE_MODULUS) % SEQUENCE_MODULUS) + 1;
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) -- value, min, max have distinct semantics
static inline size_t ClampToRange(size_t value, size_t min, size_t max)
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
    blockSequence->blockDevice          = config->blockDevice;
    blockSequence->maxFileSize          = config->maxFileSize;
    blockSequence->maxFiles             = ClampToRange(config->maxFiles, MIN_MAX_FILES, MAX_MAX_FILES);
    blockSequence->discardPolicy        = config->discardPolicy;
    blockSequence->onStoreFull          = config->onStoreFull;
    blockSequence->storeFullContext     = config->storeFullContext;
    blockSequence->getCapacityThreshold = config->getCapacityThreshold;
    blockSequence->onThresholdCrossed   = config->onThresholdCrossed;
    blockSequence->thresholdContext     = config->thresholdContext;
    blockSequence->halted               = false;
    blockSequence->atCapacity           = false;
    blockSequence->thresholdCrossed     = false;
    blockSequence->oldestSequence       = 0;
    blockSequence->readSequence         = 0;
    blockSequence->writeSequence        = 0;
    blockSequence->readCursor           = 0;
    blockSequence->writePosition        = 0;
    blockSequence->writeBlockCorrupt    = false;
}

static bool        ScanForExistingBlocks(struct BlockSequence* blockSequence);
static inline void NotifyThresholdCrossed(struct BlockSequence* blockSequence);

bool BlockSequence_Open(struct BlockSequence* blockSequence)
{
    bool foundExistingBlocks = ScanForExistingBlocks(blockSequence);
    bool ready               = false;

    if (foundExistingBlocks)
    {
        blockSequence->writePosition = SolidSyslogBlockDevice_Size(blockSequence->blockDevice, blockSequence->writeSequence);
        ready                        = true;
    }
    else
    {
        ready = SolidSyslogBlockDevice_Acquire(blockSequence->blockDevice, 0);
    }

    if (ready)
    {
        NotifyThresholdCrossed(blockSequence); /* fire on resume if usage already at-or-above threshold */
    }

    return ready;
}

enum
{
    MAX_SEQUENCE = SEQUENCE_MODULUS
};

static inline int CircularPrev(int index);
static inline int CircularNext(int index);

/* The on-disk block set is a single contiguous run in the circular sequence
 * space [0, MAX_SEQUENCE). After enough rotations the run straddles the
 * 99 -> 00 boundary (e.g. {98, 99, 0, 1}). Naive lowest=oldest /
 * highest=write mis-classifies wrapped runs, so locate the gap (the absent
 * blocks) and read the run as the complement: oldest sits one past the gap
 * end, write sits one before the gap start. */
static bool ScanForExistingBlocks(struct BlockSequence* blockSequence)
{
    bool presence[MAX_SEQUENCE];
    bool foundAny    = false;
    bool foundAbsent = false;
    int  firstAbsent = 0;

    for (int seq = 0; seq < MAX_SEQUENCE; seq++)
    {
        presence[seq] = SolidSyslogBlockDevice_Exists(blockSequence->blockDevice, (size_t) seq);

        if (presence[seq])
        {
            foundAny = true;
        }
        else if (!foundAbsent)
        {
            firstAbsent = seq;
            foundAbsent = true;
        }
    }

    if (!foundAny)
    {
        return false;
    }

    int oldest = 0;
    int newest = MAX_SEQUENCE - 1;

    if (foundAbsent)
    {
        oldest = CircularNext(firstAbsent);
        while (!presence[oldest])
        {
            oldest = CircularNext(oldest);
        }

        newest = CircularPrev(firstAbsent);
        while (!presence[newest])
        {
            newest = CircularPrev(newest);
        }
    }
    /* else: every block is present — maxFiles is clamped to MAX_SEQUENCE - 1
     * so this cannot arise from the library's own rotation. Treat the run as
     * [0, MAX_SEQUENCE - 1] (defaults above). */

    blockSequence->oldestSequence = (uint8_t) oldest;
    blockSequence->readSequence   = (uint8_t) oldest;
    blockSequence->writeSequence  = (uint8_t) newest;

    return true;
}

static inline int CircularNext(int index)
{
    return (index + 1) % MAX_SEQUENCE;
}

static inline int CircularPrev(int index)
{
    return (index + MAX_SEQUENCE - 1) % MAX_SEQUENCE;
}

static inline bool BlockIsFull(const struct BlockSequence* blockSequence, size_t recordSize);
static inline bool StoreIsFull(const struct BlockSequence* blockSequence);
static inline void NotifyStoreFull(struct BlockSequence* blockSequence);
static bool        RotateToNextBlock(struct BlockSequence* blockSequence);

bool BlockSequence_PrepareForWrite(struct BlockSequence* blockSequence, size_t recordSize, bool* readBlockChanged)
{
    bool spaceAvailable = true;
    *readBlockChanged   = false;

    if (BlockIsFull(blockSequence, recordSize) && StoreIsFull(blockSequence))
    {
        blockSequence->atCapacity = true;      /* sticky 100% — fixes UsedBytes at total */
        NotifyThresholdCrossed(blockSequence); /* threshold first per S05.09 ordering */
        NotifyStoreFull(blockSequence);
        spaceAvailable = false;
    }
    else if (BlockIsFull(blockSequence, recordSize))
    {
        *readBlockChanged = RotateToNextBlock(blockSequence);
    }

    return spaceAvailable;
}

static inline bool BlockIsFull(const struct BlockSequence* blockSequence, size_t recordSize)
{
    return blockSequence->writeBlockCorrupt || (blockSequence->writePosition + recordSize) > blockSequence->maxFileSize;
}

static inline bool StoreIsFull(const struct BlockSequence* blockSequence)
{
    return (BlockCount(blockSequence) >= blockSequence->maxFiles) && (blockSequence->discardPolicy != SOLIDSYSLOG_DISCARD_OLDEST);
}

static inline void NotifyStoreFull(struct BlockSequence* blockSequence)
{
    if ((blockSequence->discardPolicy == SOLIDSYSLOG_HALT) && !blockSequence->halted)
    {
        blockSequence->halted = true;

        if (blockSequence->onStoreFull != NULL)
        {
            blockSequence->onStoreFull(blockSequence->storeFullContext);
        }
    }
}

static bool DiscardOldestBlock(struct BlockSequence* blockSequence);
static void ResetReadToOldest(struct BlockSequence* blockSequence);

static bool RotateToNextBlock(struct BlockSequence* blockSequence)
{
    blockSequence->writeSequence     = NextSequence(blockSequence->writeSequence);
    blockSequence->writePosition     = 0;
    blockSequence->writeBlockCorrupt = false;
    SolidSyslogBlockDevice_Acquire(blockSequence->blockDevice, blockSequence->writeSequence);

    bool readBlockChanged = false;

    if (BlockCount(blockSequence) > blockSequence->maxFiles)
    {
        readBlockChanged = DiscardOldestBlock(blockSequence);
    }

    return readBlockChanged;
}

static bool DiscardOldestBlock(struct BlockSequence* blockSequence)
{
    bool readingOldestBlock = (blockSequence->readSequence == blockSequence->oldestSequence);

    SolidSyslogBlockDevice_Dispose(blockSequence->blockDevice, blockSequence->oldestSequence);
    blockSequence->oldestSequence = NextSequence(blockSequence->oldestSequence);

    if (readingOldestBlock)
    {
        ResetReadToOldest(blockSequence);
    }

    return readingOldestBlock;
}

static void ResetReadToOldest(struct BlockSequence* blockSequence)
{
    blockSequence->readSequence = blockSequence->oldestSequence;
    blockSequence->readCursor   = 0;
}

struct SolidSyslogBlockDevice* BlockSequence_BlockDevice(const struct BlockSequence* blockSequence)
{
    return blockSequence->blockDevice;
}

size_t BlockSequence_WriteSequence(const struct BlockSequence* blockSequence)
{
    return blockSequence->writeSequence;
}

size_t BlockSequence_WritePosition(const struct BlockSequence* blockSequence)
{
    return blockSequence->writePosition;
}

void BlockSequence_NoteRecordWritten(struct BlockSequence* blockSequence, size_t recordSize)
{
    blockSequence->writePosition += recordSize;
    NotifyThresholdCrossed(blockSequence);
}

static inline bool ThresholdEnabled(const struct BlockSequence* blockSequence);

static inline void NotifyThresholdCrossed(struct BlockSequence* blockSequence)
{
    if (ThresholdEnabled(blockSequence))
    {
        size_t threshold = blockSequence->getCapacityThreshold(blockSequence->thresholdContext);
        size_t used      = BlockSequence_UsedBytes(blockSequence);

        if ((threshold == 0) || (used < threshold))
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

static inline bool ThresholdEnabled(const struct BlockSequence* blockSequence)
{
    return (blockSequence->onThresholdCrossed != NULL) && (blockSequence->getCapacityThreshold != NULL);
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

void BlockSequence_AdvanceToNextReadBlock(struct BlockSequence* blockSequence)
{
    blockSequence->readSequence = NextSequence(blockSequence->readSequence);
    blockSequence->readCursor   = 0;
}

bool BlockSequence_IsReadingOlderBlock(const struct BlockSequence* blockSequence)
{
    return blockSequence->readSequence != blockSequence->writeSequence;
}

bool BlockSequence_HasUnsent(const struct BlockSequence* blockSequence)
{
    return BlockSequence_IsReadingOlderBlock(blockSequence) || (blockSequence->readCursor < blockSequence->writePosition);
}

bool BlockSequence_IsHalted(const struct BlockSequence* blockSequence)
{
    return blockSequence->halted;
}

size_t BlockSequence_TotalBytes(const struct BlockSequence* blockSequence)
{
    return blockSequence->maxFiles * blockSequence->maxFileSize;
}

size_t BlockSequence_UsedBytes(const struct BlockSequence* blockSequence)
{
    if (blockSequence->atCapacity)
    {
        return BlockSequence_TotalBytes(blockSequence);
    }

    size_t closedBlocks = BlockCount(blockSequence) - 1;
    return (closedBlocks * blockSequence->maxFileSize) + blockSequence->writePosition;
}
