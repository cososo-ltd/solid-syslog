/* A NorFlash that counts what passes through it - see BddTargetCountingNorFlash.h.
 *
 * It counts each call it passes on, whether or not the flash beneath then
 * carries it out. The counts are written on the thread that uses the store and
 * read on the console's, and nothing here keeps the two apart: whoever asks
 * for the counts holds the store's thread off the flash while it does. */

#include "BddTargetCountingNorFlash.h"

#include "SolidSyslogNorFlash.h"
#include "SolidSyslogNorFlashDefinition.h"

#include <stdio.h>

// NOLINTBEGIN(bugprone-easily-swappable-parameters) -- vtable signatures: blockIndex / offset / count are positional, distinct semantics
static bool CountingNorFlash_Read(
    struct SolidSyslogNorFlash* base,
    size_t blockIndex,
    size_t offset,
    void* buffer,
    size_t count
);
static bool CountingNorFlash_Program(
    struct SolidSyslogNorFlash* base,
    size_t blockIndex,
    size_t offset,
    const void* buffer,
    size_t count
);
// NOLINTEND(bugprone-easily-swappable-parameters)
static bool CountingNorFlash_Erase(struct SolidSyslogNorFlash* base, size_t blockIndex);
static size_t CountingNorFlash_GetBlockSize(struct SolidSyslogNorFlash* base);
static size_t CountingNorFlash_GetBlockCount(struct SolidSyslogNorFlash* base);

static struct SolidSyslogNorFlash countingFlash = {
    CountingNorFlash_Read,
    CountingNorFlash_Program,
    CountingNorFlash_Erase,
    CountingNorFlash_GetBlockSize,
    CountingNorFlash_GetBlockCount,
};

static struct SolidSyslogNorFlash* flashBeneath;
static unsigned long erases;
static unsigned long programs;
static unsigned long reads;

struct SolidSyslogNorFlash* BddTargetCountingNorFlash_Over(struct SolidSyslogNorFlash* beneath)
{
    flashBeneath = beneath;
    erases = 0U;
    programs = 0U;
    reads = 0U;
    return &countingFlash;
}

bool BddTargetCountingNorFlash_Describe(char* text, size_t size)
{
    bool described = flashBeneath != NULL;
    if (described)
    {
        (void) snprintf(text, size, "flash: erases %lu, programs %lu, reads %lu", erases, programs, reads);
    }
    return described;
}

// NOLINTBEGIN(bugprone-easily-swappable-parameters) -- vtable signatures: blockIndex / offset / count are positional, distinct semantics
static bool CountingNorFlash_Read(
    struct SolidSyslogNorFlash* base,
    size_t blockIndex,
    size_t offset,
    void* buffer,
    size_t count
)
{
    (void) base;
    reads++;
    return SolidSyslogNorFlash_Read(flashBeneath, blockIndex, offset, buffer, count);
}

static bool CountingNorFlash_Program(
    struct SolidSyslogNorFlash* base,
    size_t blockIndex,
    size_t offset,
    const void* buffer,
    size_t count
)
{
    (void) base;
    programs++;
    return SolidSyslogNorFlash_Program(flashBeneath, blockIndex, offset, buffer, count);
}

// NOLINTEND(bugprone-easily-swappable-parameters)

static bool CountingNorFlash_Erase(struct SolidSyslogNorFlash* base, size_t blockIndex)
{
    (void) base;
    erases++;
    return SolidSyslogNorFlash_Erase(flashBeneath, blockIndex);
}

static size_t CountingNorFlash_GetBlockSize(struct SolidSyslogNorFlash* base)
{
    (void) base;
    return SolidSyslogNorFlash_GetBlockSize(flashBeneath);
}

static size_t CountingNorFlash_GetBlockCount(struct SolidSyslogNorFlash* base)
{
    (void) base;
    return SolidSyslogNorFlash_GetBlockCount(flashBeneath);
}
