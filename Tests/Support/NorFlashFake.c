#include "NorFlashFake.h"

#include <string.h>

#include "SolidSyslogNorFlashDefinition.h"

enum
{
    ERASED_BYTE = 0xFF
};

static struct SolidSyslogNorFlash instance;
static uint8_t storage[NORFLASHFAKE_MAX_BLOCK_COUNT][NORFLASHFAKE_MAX_BLOCK_SIZE];
static size_t eraseCountOf[NORFLASHFAKE_MAX_BLOCK_COUNT];
static size_t configuredBlockSize;
static size_t configuredBlockCount;

static size_t eraseCount;
static size_t programCount;
static size_t readCount;

static bool failNextRead;
static bool failNextProgram;
static bool failNextErase;
static bool ignoreNextProgram;
static bool failReadsArmed;
static size_t failReadsBlock;
static size_t failReadsOffset;

static size_t writeOperationCount;
static size_t cutAt;
static enum NorFlashFakeCut cutKind;
static bool powerLost;

static bool Read(struct SolidSyslogNorFlash* base, size_t blockIndex, size_t offset, void* buf, size_t count);
static bool Program(struct SolidSyslogNorFlash* base, size_t blockIndex, size_t offset, const void* buf, size_t count);
static bool Erase(struct SolidSyslogNorFlash* base, size_t blockIndex);
static size_t GetBlockSize(struct SolidSyslogNorFlash* base);
static size_t GetBlockCount(struct SolidSyslogNorFlash* base);

struct SolidSyslogNorFlash* NorFlashFake_Create(size_t blockSize, size_t blockCount)
{
    instance.Read = Read;
    instance.Program = Program;
    instance.Erase = Erase;
    instance.GetBlockSize = GetBlockSize;
    instance.GetBlockCount = GetBlockCount;
    configuredBlockSize = (blockSize <= (size_t) NORFLASHFAKE_MAX_BLOCK_SIZE) ? blockSize : 0U;
    configuredBlockCount = (blockCount <= (size_t) NORFLASHFAKE_MAX_BLOCK_COUNT) ? blockCount : 0U;
    (void) memset(storage, ERASED_BYTE, sizeof(storage));
    (void) memset(eraseCountOf, 0, sizeof(eraseCountOf));
    NorFlashFake_ResetCounts();
    failNextRead = false;
    failNextProgram = false;
    failNextErase = false;
    ignoreNextProgram = false;
    failReadsArmed = false;
    writeOperationCount = 0U;
    cutAt = 0U;
    cutKind = NORFLASHFAKE_CUT_CLEAN;
    powerLost = false;
    return &instance;
}

size_t NorFlashFake_EraseCount(void)
{
    return eraseCount;
}

size_t NorFlashFake_ProgramCount(void)
{
    return programCount;
}

size_t NorFlashFake_ReadCount(void)
{
    return readCount;
}

size_t NorFlashFake_EraseCountOf(size_t block)
{
    return eraseCountOf[block];
}

void NorFlashFake_ResetCounts(void)
{
    eraseCount = 0U;
    programCount = 0U;
    readCount = 0U;
}

uint8_t NorFlashFake_Peek(size_t block, size_t offset)
{
    return storage[block][offset];
}

void NorFlashFake_Poke(size_t block, size_t offset, uint8_t value)
{
    storage[block][offset] = value;
}

void NorFlashFake_FailNextRead(void)
{
    failNextRead = true;
}

void NorFlashFake_FailNextProgram(void)
{
    failNextProgram = true;
}

void NorFlashFake_FailNextErase(void)
{
    failNextErase = true;
}

void NorFlashFake_FailReadsTouching(size_t block, size_t offset)
{
    failReadsArmed = true;
    failReadsBlock = block;
    failReadsOffset = offset;
}

void NorFlashFake_IgnoreNextProgram(void)
{
    ignoreNextProgram = true;
}

void NorFlashFake_CutPowerAt(size_t operation, enum NorFlashFakeCut cut)
{
    cutAt = writeOperationCount + operation;
    cutKind = cut;
}

bool NorFlashFake_PowerIsLost(void)
{
    return powerLost;
}

void NorFlashFake_PowerOn(void)
{
    powerLost = false;
    cutAt = 0U;
    writeOperationCount = 0U;
}

size_t NorFlashFake_WriteOperationCount(void)
{
    return writeOperationCount;
}

static bool SpanIsInBlock(size_t blockIndex, size_t offset, size_t count)
{
    return (blockIndex < configuredBlockCount) && (count <= configuredBlockSize) &&
           (offset <= (configuredBlockSize - count));
}

static bool TakeFlag(bool* flag)
{
    bool was = *flag;
    *flag = false;
    return was;
}

static bool ReadTouchesTheFailedByte(size_t blockIndex, size_t offset, size_t count)
{
    return failReadsArmed && (blockIndex == failReadsBlock) && (failReadsOffset >= offset) &&
           (failReadsOffset < (offset + count));
}

static bool Read(struct SolidSyslogNorFlash* base, size_t blockIndex, size_t offset, void* buf, size_t count)
{
    (void) base;
    bool read = !powerLost && !TakeFlag(&failNextRead) && SpanIsInBlock(blockIndex, offset, count) &&
                !ReadTouchesTheFailedByte(blockIndex, offset, count);

    if (read)
    {
        (void) memcpy(buf, &storage[blockIndex][offset], count);
        readCount++;
    }

    return read;
}

/* True when this program or erase is the one power is lost during. */
static bool PowerGoesDuringThisOperation(void)
{
    writeOperationCount++;

    if ((cutAt != 0U) && (writeOperationCount == cutAt))
    {
        powerLost = true;
    }

    return powerLost;
}

static void ClearBits(size_t blockIndex, size_t offset, const uint8_t* bytes, size_t count)
{
    for (size_t i = 0U; i < count; i++)
    {
        storage[blockIndex][offset + i] &= bytes[i];
    }
}

static bool Program(struct SolidSyslogNorFlash* base, size_t blockIndex, size_t offset, const void* buf, size_t count)
{
    (void) base;
    bool answers = !powerLost && SpanIsInBlock(blockIndex, offset, count);
    bool programmed = false;

    if (answers && PowerGoesDuringThisOperation())
    {
        if (cutKind == NORFLASHFAKE_CUT_TORN)
        {
            ClearBits(blockIndex, offset, (const uint8_t*) buf, count / 2U);
        }
    }
    else if (answers && !TakeFlag(&failNextProgram))
    {
        if (!TakeFlag(&ignoreNextProgram))
        {
            ClearBits(blockIndex, offset, (const uint8_t*) buf, count);
        }

        programCount++;
        programmed = true;
    }
    else
    {
        /* Nothing answers, or the program is refused. */
    }

    return programmed;
}

static bool Erase(struct SolidSyslogNorFlash* base, size_t blockIndex)
{
    (void) base;
    bool answers = !powerLost && (blockIndex < configuredBlockCount);
    bool erased = false;

    if (answers && PowerGoesDuringThisOperation())
    {
        if (cutKind == NORFLASHFAKE_CUT_TORN)
        {
            (void) memset(storage[blockIndex], ERASED_BYTE, configuredBlockSize / 2U);
        }
    }
    else if (answers && !TakeFlag(&failNextErase))
    {
        (void) memset(storage[blockIndex], ERASED_BYTE, configuredBlockSize);
        eraseCount++;
        eraseCountOf[blockIndex]++;
        erased = true;
    }
    else
    {
        /* Nothing answers, or the erase is refused. */
    }

    return erased;
}

static size_t GetBlockSize(struct SolidSyslogNorFlash* base)
{
    (void) base;
    return configuredBlockSize;
}

static size_t GetBlockCount(struct SolidSyslogNorFlash* base)
{
    (void) base;
    return configuredBlockCount;
}
