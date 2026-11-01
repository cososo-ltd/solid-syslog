#include "BlockDeviceFake.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "SolidSyslogBlockDeviceDefinition.h"

enum
{
    BLOCKDEVICEFAKE_BLOCKS = 100,
    BLOCKDEVICEFAKE_BLOCK_CAPACITY = 4096,
    BLOCKDEVICEFAKE_KEPT_WRITES = 1024,
    BLOCKDEVICEFAKE_OVERLAYS = 8
};

/* A byte that reads as Value whatever the storage holds, or an offset no Read
 * may touch. */
struct Overlay
{
    size_t Block;
    size_t Offset;
    bool Active;
    uint8_t Value;
};

struct Block
{
    bool Present;
    bool FailsEverything;
    size_t Size;
    uint8_t Bytes[BLOCKDEVICEFAKE_BLOCK_CAPACITY];
};

static struct SolidSyslogBlockDevice device;
static struct Block blocks[BLOCKDEVICEFAKE_BLOCKS];
static size_t blockSize;
static size_t operationCount;
static size_t writeCount;
static struct BlockDeviceFakeWrite keptWrites[BLOCKDEVICEFAKE_KEPT_WRITES];
static struct Overlay stuckBytes[BLOCKDEVICEFAKE_OVERLAYS];
static struct Overlay failedOffsets[BLOCKDEVICEFAKE_OVERLAYS];
static enum BlockDeviceFakeAbsentBlockBehaviour absentBehaviour;
static bool removalArmed;
static size_t removalBlock;
static size_t removalAfterOperation;
static bool tearArmed;
static size_t tearBytesThatLand;

static void AddOverlay(struct Overlay* overlays, size_t block, size_t offset, uint8_t value)
{
    for (size_t i = 0U; i < BLOCKDEVICEFAKE_OVERLAYS; i++)
    {
        if (!overlays[i].Active)
        {
            overlays[i].Active = true;
            overlays[i].Block = block;
            overlays[i].Offset = offset;
            overlays[i].Value = value;
            break;
        }
    }
}

static bool OverlayTouchesSpan(const struct Overlay* overlay, size_t block, size_t offset, size_t count)
{
    return overlay->Active && (overlay->Block == block) && (overlay->Offset >= offset) &&
           (overlay->Offset < (offset + count));
}

static bool AnyFailedOffsetWithin(size_t block, size_t offset, size_t count)
{
    bool failed = false;

    for (size_t i = 0U; i < BLOCKDEVICEFAKE_OVERLAYS; i++)
    {
        failed = failed || OverlayTouchesSpan(&failedOffsets[i], block, offset, count);
    }

    return failed;
}

static void ApplyStuckBytes(size_t block, size_t offset, uint8_t* buf, size_t count)
{
    for (size_t i = 0U; i < BLOCKDEVICEFAKE_OVERLAYS; i++)
    {
        if (OverlayTouchesSpan(&stuckBytes[i], block, offset, count))
        {
            buf[stuckBytes[i].Offset - offset] = stuckBytes[i].Value;
        }
    }
}

/* A pending removal takes effect once the operation it names has completed,
 * which is when the next one starts. */
static void CountOperation(void)
{
    if (removalArmed && (operationCount == removalAfterOperation))
    {
        blocks[removalBlock].Present = false;
        removalArmed = false;
    }

    operationCount++;
}

static void LogWrite(size_t blockIndex, size_t offset, size_t count, bool appended)
{
    if (writeCount < BLOCKDEVICEFAKE_KEPT_WRITES)
    {
        keptWrites[writeCount].Block = blockIndex;
        keptWrites[writeCount].Offset = offset;
        keptWrites[writeCount].Count = count;
        keptWrites[writeCount].Appended = appended;
    }
    writeCount++;
}

static bool IsValidIndex(size_t blockIndex)
{
    return blockIndex < BLOCKDEVICEFAKE_BLOCKS;
}

/* The block at @p blockIndex when an operation on it can go ahead, else NULL:
 * every operation but Acquire and Exists goes through here, so an absent block
 * and one that fails everything fail them. */
static struct Block* PresentBlockAt(size_t blockIndex)
{
    struct Block* block = NULL;

    if (IsValidIndex(blockIndex) && !blocks[blockIndex].FailsEverything)
    {
        if (blocks[blockIndex].Present || (absentBehaviour == BLOCKDEVICEFAKE_ABSENT_IS_GHOST))
        {
            block = &blocks[blockIndex];
        }
        else if (absentBehaviour == BLOCKDEVICEFAKE_ABSENT_IS_CREATED_ON_TOUCH)
        {
            blocks[blockIndex].Present = true;
            blocks[blockIndex].Size = 0U;
            block = &blocks[blockIndex];
        }
        else
        {
            /* An absent block fails the operation. */
        }
    }

    return block;
}

/* How much of an Append lands: all of it, or what a torn write left. */
static size_t BytesThatLand(size_t count)
{
    size_t landed = count;

    if (tearArmed)
    {
        landed = (tearBytesThatLand < count) ? tearBytesThatLand : count;
        tearArmed = false;
    }

    return landed;
}

static bool SpanIsWithin(size_t offset, size_t count, size_t limit)
{
    return (offset + count) <= limit;
}

static bool Acquire(struct SolidSyslogBlockDevice* base, size_t blockIndex)
{
    (void) base;
    CountOperation();
    bool acquired = IsValidIndex(blockIndex) && !blocks[blockIndex].FailsEverything;

    if (acquired)
    {
        blocks[blockIndex].Present = true;
        blocks[blockIndex].Size = 0U;
    }

    return acquired;
}

static bool Dispose(struct SolidSyslogBlockDevice* base, size_t blockIndex)
{
    (void) base;
    CountOperation();
    struct Block* block = PresentBlockAt(blockIndex);

    if (block != NULL)
    {
        block->Present = false;
    }

    return block != NULL;
}

static bool Exists(struct SolidSyslogBlockDevice* base, size_t blockIndex)
{
    (void) base;
    CountOperation();
    return IsValidIndex(blockIndex) && blocks[blockIndex].Present;
}

static bool Read(struct SolidSyslogBlockDevice* base, size_t blockIndex, size_t offset, void* buf, size_t count)
{
    (void) base;
    CountOperation();
    const struct Block* block = PresentBlockAt(blockIndex);
    bool read = (block != NULL) && SpanIsWithin(offset, count, block->Size) &&
                !AnyFailedOffsetWithin(blockIndex, offset, count);

    if (read)
    {
        memcpy(buf, &block->Bytes[offset], count);
        ApplyStuckBytes(blockIndex, offset, buf, count);
    }

    return read;
}

static bool Append(struct SolidSyslogBlockDevice* base, size_t blockIndex, const void* buf, size_t count)
{
    (void) base;
    CountOperation();
    struct Block* block = PresentBlockAt(blockIndex);
    bool appended = (block != NULL) && SpanIsWithin(block->Size, count, blockSize) &&
                    SpanIsWithin(block->Size, count, BLOCKDEVICEFAKE_BLOCK_CAPACITY);

    if (appended)
    {
        size_t landed = BytesThatLand(count);
        LogWrite(blockIndex, block->Size, landed, true);
        memcpy(&block->Bytes[block->Size], buf, landed);
        block->Size += landed;
    }

    return appended;
}

static bool WriteAt(
    struct SolidSyslogBlockDevice* base,
    size_t blockIndex,
    size_t offset,
    const void* buf,
    size_t count
)
{
    (void) base;
    CountOperation();
    struct Block* block = PresentBlockAt(blockIndex);
    bool written = (block != NULL) && SpanIsWithin(offset, count, block->Size);

    if (written)
    {
        LogWrite(blockIndex, offset, count, false);
        memcpy(&block->Bytes[offset], buf, count);
    }

    return written;
}

static bool Size(struct SolidSyslogBlockDevice* base, size_t blockIndex, size_t* size)
{
    (void) base;
    CountOperation();
    const struct Block* block = PresentBlockAt(blockIndex);
    *size = (block != NULL) ? block->Size : 0U;
    return block != NULL;
}

static size_t GetBlockSize(struct SolidSyslogBlockDevice* base)
{
    (void) base;
    return blockSize;
}

struct SolidSyslogBlockDevice* BlockDeviceFake_Create(size_t size)
{
    memset(blocks, 0, sizeof(blocks));
    memset(keptWrites, 0, sizeof(keptWrites));
    memset(stuckBytes, 0, sizeof(stuckBytes));
    memset(failedOffsets, 0, sizeof(failedOffsets));
    blockSize = size;
    operationCount = 0U;
    writeCount = 0U;
    absentBehaviour = BLOCKDEVICEFAKE_ABSENT_FAILS;
    removalArmed = false;
    tearArmed = false;
    device.Acquire = Acquire;
    device.Dispose = Dispose;
    device.Exists = Exists;
    device.Read = Read;
    device.Append = Append;
    device.WriteAt = WriteAt;
    device.Size = Size;
    device.GetBlockSize = GetBlockSize;
    return &device;
}

size_t BlockDeviceFake_OperationCount(void)
{
    return operationCount;
}

size_t BlockDeviceFake_WriteCount(void)
{
    return writeCount;
}

struct BlockDeviceFakeWrite BlockDeviceFake_Write(size_t index)
{
    struct BlockDeviceFakeWrite none = {0U, 0U, 0U, false};
    return (index < BLOCKDEVICEFAKE_KEPT_WRITES) ? keptWrites[index] : none;
}

void BlockDeviceFake_FlipBit(size_t block, size_t offset, unsigned bit)
{
    blocks[block].Bytes[offset] ^= (uint8_t) (1U << bit);
}

void BlockDeviceFake_StickByte(size_t block, size_t offset, uint8_t value)
{
    AddOverlay(stuckBytes, block, offset, value);
}

void BlockDeviceFake_FailReadsTouching(size_t block, size_t offset)
{
    AddOverlay(failedOffsets, block, offset, 0U);
}

void BlockDeviceFake_TruncateTo(size_t block, size_t size)
{
    blocks[block].Size = size;
}

void BlockDeviceFake_RemoveBlock(size_t block)
{
    blocks[block].Present = false;
}

void BlockDeviceFake_FailEveryOperationOn(size_t block)
{
    blocks[block].FailsEverything = true;
}

void BlockDeviceFake_RemoveBlockAfterOperation(size_t block, size_t afterOperation)
{
    removalArmed = true;
    removalBlock = block;
    removalAfterOperation = afterOperation;
}

void BlockDeviceFake_SetAbsentBlockBehaviour(enum BlockDeviceFakeAbsentBlockBehaviour behaviour)
{
    absentBehaviour = behaviour;
}

void BlockDeviceFake_TearNextAppend(size_t bytesThatLand)
{
    tearArmed = true;
    tearBytesThatLand = bytesThatLand;
}
