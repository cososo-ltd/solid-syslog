/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogNorFlashBlockDevice.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "SolidSyslogBlockDeviceDefinition.h"
#include "SolidSyslogError.h"
#include "SolidSyslogNorFlash.h"
#include "SolidSyslogNorFlashBlockDeviceErrors.h"
#include "SolidSyslogNorFlashBlockDevicePrivate.h"
#include "SolidSyslogNorFlashCategories.h"
#include "SolidSyslogNullBlockDevice.h"

const struct SolidSyslogErrorSource SolidSyslogNorFlashBlockDeviceErrorSource = {
    "NorFlashBlockDevice",
    SolidSyslogNorFlashBlockDeviceErrors_Name
};

enum
{
    STATE_SIZE = SOLIDSYSLOG_NOR_FLASH_BLOCK_DEVICE_STATE_SIZE,
    STATE_OFFSET = 0,
    /* A block's data follows its state. */
    DATA_OFFSET = STATE_SIZE,
    ERASED_BYTE = 0xFF,
    /* How much is read at a time, onto the stack, to look for the end of a
     * block's data or to check what a program left. */
    CHUNK_SIZE = 64,
    /* A block's states, in the order it passes through them. */
    STATE_IN_USE = 0,
    STATE_DISPOSED = 1,
    STATE_COUNT = 2
};

/* The word for each state, disposed reached from in use by clearing bits. A
 * block is in a state only while its word reads as exactly these bytes, so a
 * blank word, and almost anything an erase or a program cut short by power
 * leaves, is neither. No state says a block is erased: Acquire erases whatever
 * it takes, so nothing a power cut can leave is trusted as erased. The one
 * case left, an erase that stops on the in-use word, is accepted; the device
 * header says what it costs. */
static const uint8_t NorFlashBlockDevice_StateWords[STATE_COUNT][STATE_SIZE] =
    {[STATE_IN_USE] = {0x5A, 0xA5, 0xFF, 0xFF}, [STATE_DISPOSED] = {0x00, 0x00, 0x00, 0x00}};

/* vtable - forward-declared because SolidSyslogNorFlashBlockDevice_Initialise wires them before their definitions */
static bool NorFlashBlockDevice_Acquire(struct SolidSyslogBlockDevice* base, size_t blockIndex);
static bool NorFlashBlockDevice_Dispose(struct SolidSyslogBlockDevice* base, size_t blockIndex);
static bool NorFlashBlockDevice_Exists(struct SolidSyslogBlockDevice* base, size_t blockIndex);
// NOLINTBEGIN(bugprone-easily-swappable-parameters) -- vtable signature: blockIndex / offset are positional, distinct semantics
static bool NorFlashBlockDevice_Read(
    struct SolidSyslogBlockDevice* base,
    size_t blockIndex,
    size_t offset,
    void* buffer,
    size_t count
);
// NOLINTEND(bugprone-easily-swappable-parameters)
static bool NorFlashBlockDevice_Append(
    struct SolidSyslogBlockDevice* base,
    size_t blockIndex,
    const void* buffer,
    size_t count
);
// NOLINTBEGIN(bugprone-easily-swappable-parameters) -- vtable signature: blockIndex / offset are positional, distinct semantics
static bool NorFlashBlockDevice_WriteAt(
    struct SolidSyslogBlockDevice* base,
    size_t blockIndex,
    size_t offset,
    const void* buffer,
    size_t count
);
// NOLINTEND(bugprone-easily-swappable-parameters)
static bool NorFlashBlockDevice_Size(struct SolidSyslogBlockDevice* base, size_t blockIndex, size_t* size);
static size_t NorFlashBlockDevice_GetBlockSize(struct SolidSyslogBlockDevice* base);
static size_t NorFlashBlockDevice_GetBlockCount(struct SolidSyslogBlockDevice* base);

static inline struct SolidSyslogNorFlashBlockDevice* NorFlashBlockDevice_SelfFromBase(
    struct SolidSyslogBlockDevice* base
);

void SolidSyslogNorFlashBlockDevice_Initialise(
    struct SolidSyslogBlockDevice* base,
    const struct SolidSyslogNorFlashBlockDeviceConfig* config
)
{
    struct SolidSyslogNorFlashBlockDevice* self = NorFlashBlockDevice_SelfFromBase(base);
    self->Base.Acquire = NorFlashBlockDevice_Acquire;
    self->Base.Dispose = NorFlashBlockDevice_Dispose;
    self->Base.Exists = NorFlashBlockDevice_Exists;
    self->Base.Read = NorFlashBlockDevice_Read;
    self->Base.Append = NorFlashBlockDevice_Append;
    self->Base.WriteAt = NorFlashBlockDevice_WriteAt;
    self->Base.Size = NorFlashBlockDevice_Size;
    self->Base.GetBlockSize = NorFlashBlockDevice_GetBlockSize;
    self->Base.GetBlockCount = NorFlashBlockDevice_GetBlockCount;
    self->Config = *config;
    self->DataCapacity = SolidSyslogNorFlashBlockDevice_DataCapacity(SolidSyslogNorFlash_GetBlockSize(config->Flash));
    self->EndBlock = 0U;
    self->End = 0U;
    self->EndIsKnown = false;
}

void SolidSyslogNorFlashBlockDevice_Cleanup(struct SolidSyslogBlockDevice* base)
{
    /* The abstract base takes the shared NullBlockDevice vtable, so use after
     * destroy is a safe no-op. The flash is the caller's and is not touched. */
    *base = *SolidSyslogNullBlockDevice_Get();
}

size_t SolidSyslogNorFlashBlockDevice_DataCapacity(size_t flashBlockSize)
{
    return (flashBlockSize > (size_t) DATA_OFFSET) ? (flashBlockSize - (size_t) DATA_OFFSET) : 0U;
}

static inline struct SolidSyslogNorFlashBlockDevice* NorFlashBlockDevice_SelfFromBase(
    struct SolidSyslogBlockDevice* base
)
{
    return (struct SolidSyslogNorFlashBlockDevice*) base;
}

static inline bool NorFlashBlockDevice_IsValidBlockIndex(
    const struct SolidSyslogNorFlashBlockDevice* self,
    size_t blockIndex
)
{
    return blockIndex < self->Config.BlockCount;
}

static inline size_t NorFlashBlockDevice_FlashBlock(
    const struct SolidSyslogNorFlashBlockDevice* self,
    size_t blockIndex
)
{
    return self->Config.FirstBlock + blockIndex;
}

// NOLINTBEGIN(bugprone-easily-swappable-parameters) -- a block and a state; distinct semantics
static bool NorFlashBlockDevice_StateIs(
    const struct SolidSyslogNorFlashBlockDevice* self,
    size_t blockIndex,
    size_t state
);
static bool NorFlashBlockDevice_EnterState(
    const struct SolidSyslogNorFlashBlockDevice* self,
    size_t blockIndex,
    size_t state
);
// NOLINTEND(bugprone-easily-swappable-parameters)
static bool NorFlashBlockDevice_ProgramAndVerify(
    const struct SolidSyslogNorFlashBlockDevice* self,
    size_t blockIndex,
    size_t flashOffset,
    const void* buffer,
    size_t count
);
static inline void NorFlashBlockDevice_RememberEnd(
    struct SolidSyslogNorFlashBlockDevice* self,
    size_t blockIndex,
    size_t end
);
static inline void NorFlashBlockDevice_ForgetEndOf(struct SolidSyslogNorFlashBlockDevice* self, size_t blockIndex);
static bool NorFlashBlockDevice_IsInUse(const struct SolidSyslogNorFlashBlockDevice* self, size_t blockIndex);

/* Every block is erased as it is taken, whatever it reads as: cells an erase
 * was cut short on can read as erased and not be, and an erase repeated is
 * what clears them. */
static bool NorFlashBlockDevice_Acquire(struct SolidSyslogBlockDevice* base, size_t blockIndex)
{
    struct SolidSyslogNorFlashBlockDevice* self = NorFlashBlockDevice_SelfFromBase(base);
    bool ready = false;

    if (NorFlashBlockDevice_IsValidBlockIndex(self, blockIndex))
    {
        ready = SolidSyslogNorFlash_Erase(self->Config.Flash, NorFlashBlockDevice_FlashBlock(self, blockIndex));

        if (ready)
        {
            ready = NorFlashBlockDevice_EnterState(self, blockIndex, STATE_IN_USE);
        }

        if (ready)
        {
            NorFlashBlockDevice_RememberEnd(self, blockIndex, 0U);
        }
        else
        {
            NorFlashBlockDevice_ForgetEndOf(self, blockIndex);
        }
    }

    return ready;
}

/* A state word that cannot be read is in no state. */
// NOLINTBEGIN(bugprone-easily-swappable-parameters) -- a block and a state; distinct semantics
static bool NorFlashBlockDevice_StateIs(
    const struct SolidSyslogNorFlashBlockDevice* self,
    size_t blockIndex,
    size_t state
)
{
    uint8_t word[STATE_SIZE] = {0};
    bool read = SolidSyslogNorFlash_Read(
        self->Config.Flash,
        NorFlashBlockDevice_FlashBlock(self, blockIndex),
        STATE_OFFSET,
        word,
        STATE_SIZE
    );

    return read && (memcmp(word, NorFlashBlockDevice_StateWords[state], STATE_SIZE) == 0);
}

// NOLINTEND(bugprone-easily-swappable-parameters)

// NOLINTBEGIN(bugprone-easily-swappable-parameters) -- a block and a state; distinct semantics
static bool NorFlashBlockDevice_EnterState(
    const struct SolidSyslogNorFlashBlockDevice* self,
    size_t blockIndex,
    size_t state
)
{
    return NorFlashBlockDevice_ProgramAndVerify(
        self,
        blockIndex,
        STATE_OFFSET,
        NorFlashBlockDevice_StateWords[state],
        STATE_SIZE
    );
}

// NOLINTEND(bugprone-easily-swappable-parameters)

static bool NorFlashBlockDevice_Holds(
    const struct SolidSyslogNorFlashBlockDevice* self,
    size_t blockIndex,
    size_t flashOffset,
    const uint8_t* expected,
    size_t count
);

/* The flash does not say whether a program took, so what it left is read
 * back. A read that fails is the flash's to report; bytes that differ are
 * reported here. */
static bool NorFlashBlockDevice_ProgramAndVerify(
    const struct SolidSyslogNorFlashBlockDevice* self,
    size_t blockIndex,
    size_t flashOffset,
    const void* buffer,
    size_t count
)
{
    bool programmed = SolidSyslogNorFlash_Program(
        self->Config.Flash,
        NorFlashBlockDevice_FlashBlock(self, blockIndex),
        flashOffset,
        buffer,
        count
    );

    if (programmed)
    {
        programmed = NorFlashBlockDevice_Holds(self, blockIndex, flashOffset, buffer, count);
    }

    return programmed;
}

static inline size_t NorFlashBlockDevice_Smaller(size_t first, size_t second)
{
    return (first < second) ? first : second;
}

static bool NorFlashBlockDevice_Holds(
    const struct SolidSyslogNorFlashBlockDevice* self,
    size_t blockIndex,
    size_t flashOffset,
    const uint8_t* expected,
    size_t count
)
{
    size_t checked = 0U;
    bool holds = true;

    while (holds && (checked < count))
    {
        uint8_t chunk[CHUNK_SIZE] = {0};
        size_t length = NorFlashBlockDevice_Smaller(count - checked, (size_t) CHUNK_SIZE);

        holds = SolidSyslogNorFlash_Read(
            self->Config.Flash,
            NorFlashBlockDevice_FlashBlock(self, blockIndex),
            flashOffset + checked,
            chunk,
            length
        );

        if (holds && (memcmp(chunk, &expected[checked], length) != 0))
        {
            NorFlashBlockDevice_Report(
                SOLIDSYSLOG_NOR_FLASH_IO_FAILED_SEVERITY,
                SOLIDSYSLOG_CAT_NOR_FLASH_IO_FAILED,
                SOLIDSYSLOG_NOR_FLASH_BLOCK_DEVICE_ERROR_VERIFY_FAILED
            );
            holds = false;
        }

        checked += length;
    }

    return holds;
}

static inline void NorFlashBlockDevice_RememberEnd(
    struct SolidSyslogNorFlashBlockDevice* self,
    size_t blockIndex,
    size_t end
)
{
    self->EndBlock = blockIndex;
    self->End = end;
    self->EndIsKnown = true;
}

static inline bool NorFlashBlockDevice_KnowsEndOf(const struct SolidSyslogNorFlashBlockDevice* self, size_t blockIndex)
{
    return self->EndIsKnown && (self->EndBlock == blockIndex);
}

static inline void NorFlashBlockDevice_ForgetEndOf(struct SolidSyslogNorFlashBlockDevice* self, size_t blockIndex)
{
    if (NorFlashBlockDevice_KnowsEndOf(self, blockIndex))
    {
        self->EndIsKnown = false;
    }
}

/* The block is gone once it is marked disposed, and that one program is all
 * Dispose does: the erase waits for the Acquire that next takes the block. */
static bool NorFlashBlockDevice_Dispose(struct SolidSyslogBlockDevice* base, size_t blockIndex)
{
    struct SolidSyslogNorFlashBlockDevice* self = NorFlashBlockDevice_SelfFromBase(base);
    bool disposed = false;

    if (NorFlashBlockDevice_IsValidBlockIndex(self, blockIndex))
    {
        NorFlashBlockDevice_ForgetEndOf(self, blockIndex);
        disposed = NorFlashBlockDevice_EnterState(self, blockIndex, STATE_DISPOSED);
    }

    return disposed;
}

static bool NorFlashBlockDevice_Exists(struct SolidSyslogBlockDevice* base, size_t blockIndex)
{
    return NorFlashBlockDevice_IsInUse(NorFlashBlockDevice_SelfFromBase(base), blockIndex);
}

/* A state that cannot be read is no state, so such a block is not in use. */
static bool NorFlashBlockDevice_IsInUse(const struct SolidSyslogNorFlashBlockDevice* self, size_t blockIndex)
{
    return NorFlashBlockDevice_IsValidBlockIndex(self, blockIndex) &&
           NorFlashBlockDevice_StateIs(self, blockIndex, STATE_IN_USE);
}

/* The flash block is the bound, not the block size the device reports: a
 * store lets its largest record run past a block size smaller than one. The
 * subtraction keeps `offset + count` from wrapping past the comparison. */
// NOLINTBEGIN(bugprone-easily-swappable-parameters) -- offset and count are a position and a length; distinct semantics
static inline bool NorFlashBlockDevice_SpanIsWithinFlashBlock(
    const struct SolidSyslogNorFlashBlockDevice* self,
    size_t offset,
    size_t count
)
{
    return (count <= self->DataCapacity) && (offset <= (self->DataCapacity - count));
}

/* Where Read and WriteAt may go: a block of the run, and a span of its flash
 * block. */
static inline bool NorFlashBlockDevice_IsReachableSpan(
    const struct SolidSyslogNorFlashBlockDevice* self,
    size_t blockIndex,
    size_t offset,
    size_t count
)
{
    return NorFlashBlockDevice_IsValidBlockIndex(self, blockIndex) &&
           NorFlashBlockDevice_SpanIsWithinFlashBlock(self, offset, count);
}

// NOLINTEND(bugprone-easily-swappable-parameters)

// NOLINTBEGIN(bugprone-easily-swappable-parameters) -- vtable signature: blockIndex / offset are positional, distinct semantics
static bool NorFlashBlockDevice_Read(
    struct SolidSyslogBlockDevice* base,
    size_t blockIndex,
    size_t offset,
    void* buffer,
    size_t count
)
{
    const struct SolidSyslogNorFlashBlockDevice* self = NorFlashBlockDevice_SelfFromBase(base);
    bool read = false;

    if (NorFlashBlockDevice_IsReachableSpan(self, blockIndex, offset, count))
    {
        read = SolidSyslogNorFlash_Read(
            self->Config.Flash,
            NorFlashBlockDevice_FlashBlock(self, blockIndex),
            (size_t) DATA_OFFSET + offset,
            buffer,
            count
        );
    }

    return read;
}

// NOLINTEND(bugprone-easily-swappable-parameters)

static bool NorFlashBlockDevice_EndOfData(
    const struct SolidSyslogNorFlashBlockDevice* self,
    size_t blockIndex,
    size_t* end
);
static bool NorFlashBlockDevice_FindEndOfData(
    const struct SolidSyslogNorFlashBlockDevice* self,
    size_t blockIndex,
    size_t* end
);

static bool NorFlashBlockDevice_Append(
    struct SolidSyslogBlockDevice* base,
    size_t blockIndex,
    const void* buffer,
    size_t count
)
{
    struct SolidSyslogNorFlashBlockDevice* self = NorFlashBlockDevice_SelfFromBase(base);
    bool appended = false;
    size_t end = 0U;

    if (NorFlashBlockDevice_IsValidBlockIndex(self, blockIndex) &&
        NorFlashBlockDevice_EndOfData(self, blockIndex, &end) &&
        NorFlashBlockDevice_SpanIsWithinFlashBlock(self, end, count))
    {
        appended = NorFlashBlockDevice_ProgramAndVerify(self, blockIndex, (size_t) DATA_OFFSET + end, buffer, count);

        /* After a program that did not take, where the data ends is not known. */
        if (appended)
        {
            NorFlashBlockDevice_RememberEnd(self, blockIndex, end + count);
        }
        else
        {
            NorFlashBlockDevice_ForgetEndOf(self, blockIndex);
        }
    }

    return appended;
}

static bool NorFlashBlockDevice_EndOfData(
    const struct SolidSyslogNorFlashBlockDevice* self,
    size_t blockIndex,
    size_t* end
)
{
    bool known = NorFlashBlockDevice_KnowsEndOf(self, blockIndex);

    if (known)
    {
        *end = self->End;
    }
    else
    {
        known = NorFlashBlockDevice_FindEndOfData(self, blockIndex, end);
    }

    return known;
}

static inline size_t NorFlashBlockDevice_LengthToLastProgrammedByte(const uint8_t* bytes, size_t count);

/* The data ends at the last byte that is not as an erase leaves it, looked for
 * from the end of the flash block back. A store ends every record on such a
 * byte. */
static bool NorFlashBlockDevice_FindEndOfData(
    const struct SolidSyslogNorFlashBlockDevice* self,
    size_t blockIndex,
    size_t* end
)
{
    size_t unread = self->DataCapacity;
    size_t found = 0U;
    bool readable = true;

    while (readable && (found == 0U) && (unread > 0U))
    {
        uint8_t chunk[CHUNK_SIZE] = {0};
        size_t length = NorFlashBlockDevice_Smaller(unread, (size_t) CHUNK_SIZE);
        size_t from = unread - length;

        readable = SolidSyslogNorFlash_Read(
            self->Config.Flash,
            NorFlashBlockDevice_FlashBlock(self, blockIndex),
            (size_t) DATA_OFFSET + from,
            chunk,
            length
        );

        if (readable)
        {
            size_t programmed = NorFlashBlockDevice_LengthToLastProgrammedByte(chunk, length);

            if (programmed > 0U)
            {
                found = from + programmed;
            }

            unread = from;
        }
    }

    *end = found;
    return readable;
}

static inline size_t NorFlashBlockDevice_LengthToLastProgrammedByte(const uint8_t* bytes, size_t count)
{
    size_t length = count;

    while ((length > 0U) && (bytes[length - 1U] == (uint8_t) ERASED_BYTE))
    {
        length--;
    }

    return length;
}

// NOLINTBEGIN(bugprone-easily-swappable-parameters) -- vtable signature: blockIndex / offset are positional, distinct semantics
static bool NorFlashBlockDevice_WriteAt(
    struct SolidSyslogBlockDevice* base,
    size_t blockIndex,
    size_t offset,
    const void* buffer,
    size_t count
)
{
    const struct SolidSyslogNorFlashBlockDevice* self = NorFlashBlockDevice_SelfFromBase(base);
    bool written = false;

    if (NorFlashBlockDevice_IsReachableSpan(self, blockIndex, offset, count))
    {
        written = NorFlashBlockDevice_ProgramAndVerify(self, blockIndex, (size_t) DATA_OFFSET + offset, buffer, count);
    }

    return written;
}

// NOLINTEND(bugprone-easily-swappable-parameters)

/* Only of a block that is there: one that is not would scan as empty, and a
 * store must never take a missing block for an empty one. An end found while
 * none is kept is kept, which on a restart is the write block's: a store asks
 * its size before it asks any other's. */
static bool NorFlashBlockDevice_Size(struct SolidSyslogBlockDevice* base, size_t blockIndex, size_t* size)
{
    struct SolidSyslogNorFlashBlockDevice* self = NorFlashBlockDevice_SelfFromBase(base);
    bool known = false;
    *size = 0U;

    if (NorFlashBlockDevice_IsInUse(self, blockIndex))
    {
        known = NorFlashBlockDevice_EndOfData(self, blockIndex, size);

        if (known && !self->EndIsKnown)
        {
            NorFlashBlockDevice_RememberEnd(self, blockIndex, *size);
        }
    }

    return known;
}

static size_t NorFlashBlockDevice_GetBlockSize(struct SolidSyslogBlockDevice* base)
{
    return NorFlashBlockDevice_SelfFromBase(base)->Config.BlockSize;
}

static size_t NorFlashBlockDevice_GetBlockCount(struct SolidSyslogBlockDevice* base)
{
    return NorFlashBlockDevice_SelfFromBase(base)->Config.BlockCount;
}
