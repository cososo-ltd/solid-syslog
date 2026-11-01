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
    BLOCKDEVICEFAKE_KEPT_WRITES = 256
};

struct Block
{
    bool Present;
    size_t Size;
    uint8_t Bytes[BLOCKDEVICEFAKE_BLOCK_CAPACITY];
};

static struct SolidSyslogBlockDevice device;
static struct Block blocks[BLOCKDEVICEFAKE_BLOCKS];
static size_t blockSize;
static size_t operationCount;
static size_t writeCount;
static struct BlockDeviceFakeWrite keptWrites[BLOCKDEVICEFAKE_KEPT_WRITES];

static void CountOperation(void)
{
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

/* The block at @p blockIndex when it is present, else NULL. Every operation
 * but Acquire and Exists goes through here, so an absent block fails them. */
static struct Block* PresentBlockAt(size_t blockIndex)
{
    struct Block* block = NULL;

    if ((blockIndex < BLOCKDEVICEFAKE_BLOCKS) && blocks[blockIndex].Present)
    {
        block = &blocks[blockIndex];
    }

    return block;
}

static bool SpanIsWithin(size_t offset, size_t count, size_t limit)
{
    return (offset + count) <= limit;
}

static bool Acquire(struct SolidSyslogBlockDevice* base, size_t blockIndex)
{
    (void) base;
    CountOperation();
    bool acquired = blockIndex < BLOCKDEVICEFAKE_BLOCKS;

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
    return PresentBlockAt(blockIndex) != NULL;
}

static bool Read(struct SolidSyslogBlockDevice* base, size_t blockIndex, size_t offset, void* buf, size_t count)
{
    (void) base;
    CountOperation();
    const struct Block* block = PresentBlockAt(blockIndex);
    bool read = (block != NULL) && SpanIsWithin(offset, count, block->Size);

    if (read)
    {
        memcpy(buf, &block->Bytes[offset], count);
    }

    return read;
}

static bool Append(struct SolidSyslogBlockDevice* base, size_t blockIndex, const void* buf, size_t count)
{
    (void) base;
    CountOperation();
    struct Block* block = PresentBlockAt(blockIndex);
    bool appended = (block != NULL) && SpanIsWithin(block->Size, count, blockSize);

    if (appended)
    {
        LogWrite(blockIndex, block->Size, count, true);
        memcpy(&block->Bytes[block->Size], buf, count);
        block->Size += count;
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
    blockSize = size;
    operationCount = 0U;
    writeCount = 0U;
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
