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
    CHUNK_SIZE = 64
};

/* A block's state, each reached from the one before by clearing bits. A block
 * is in a state only while its word reads as exactly these bytes, so a blank
 * word, and whatever an erase or a program cut short by power leaves, is none
 * of them. Disposed clears every bit: an erase sets bits, and for one cut
 * short to leave another state's word it would have to set the right bits of
 * all four bytes and no others. */
static const uint8_t STATE_ERASED[STATE_SIZE] = {0x5A, 0xFF, 0xFF, 0xFF};
static const uint8_t STATE_IN_USE[STATE_SIZE] = {0x5A, 0xA5, 0xFF, 0xFF};
static const uint8_t STATE_DISPOSED[STATE_SIZE] = {0x00, 0x00, 0x00, 0x00};

/* vtable - forward-declared because SolidSyslogNorFlashBlockDevice_Initialise wires them before their definitions */
static bool NorFlashBlockDevice_Acquire(struct SolidSyslogBlockDevice* base, size_t blockIndex);
static bool NorFlashBlockDevice_Dispose(struct SolidSyslogBlockDevice* base, size_t blockIndex);
static bool NorFlashBlockDevice_Exists(struct SolidSyslogBlockDevice* base, size_t blockIndex);
// NOLINTBEGIN(bugprone-easily-swappable-parameters) -- vtable signature: blockIndex / offset are positional, distinct semantics
static bool NorFlashBlockDevice_Read(
    struct SolidSyslogBlockDevice* base,
    size_t blockIndex,
    size_t offset,
    void* buf,
    size_t count
);
// NOLINTEND(bugprone-easily-swappable-parameters)
static bool NorFlashBlockDevice_Append(
    struct SolidSyslogBlockDevice* base,
    size_t blockIndex,
    const void* buf,
    size_t count
);
// NOLINTBEGIN(bugprone-easily-swappable-parameters) -- vtable signature: blockIndex / offset are positional, distinct semantics
static bool NorFlashBlockDevice_WriteAt(
    struct SolidSyslogBlockDevice* base,
    size_t blockIndex,
    size_t offset,
    const void* buf,
    size_t count
);
// NOLINTEND(bugprone-easily-swappable-parameters)
static bool NorFlashBlockDevice_Size(struct SolidSyslogBlockDevice* base, size_t blockIndex, size_t* size);
static size_t NorFlashBlockDevice_GetBlockSize(struct SolidSyslogBlockDevice* base);
static size_t NorFlashBlockDevice_GetBlockCount(struct SolidSyslogBlockDevice* base);

static inline struct SolidSyslogNorFlashBlockDevice* NorFlashBlockDevice_SelfFromBase(struct SolidSyslogBlockDevice* base
);

void SolidSyslogNorFlashBlockDevice_Initialise(
    struct SolidSyslogBlockDevice* base,
    const struct SolidSyslogNorFlashBlockDeviceGeometry* geometry
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
    self->Flash = geometry->Flash;
    self->FirstBlock = geometry->FirstBlock;
    self->BlockCount = geometry->BlockCount;
    self->BlockSize = geometry->BlockSize;
    self->KnownEnd = (struct KnownEnd) {.BlockIndex = 0, .End = 0, .IsKnown = false};
}

void SolidSyslogNorFlashBlockDevice_Cleanup(struct SolidSyslogBlockDevice* base)
{
    /* The abstract base takes the shared NullBlockDevice vtable, so use after
     * destroy is a safe no-op. The flash is the caller's and is not touched. */
    *base = *SolidSyslogNullBlockDevice_Get();
}

static inline struct SolidSyslogNorFlashBlockDevice* NorFlashBlockDevice_SelfFromBase(struct SolidSyslogBlockDevice* base
)
{
    return (struct SolidSyslogNorFlashBlockDevice*) base;
}

static inline bool NorFlashBlockDevice_IsValidBlockIndex(
    const struct SolidSyslogNorFlashBlockDevice* self,
    size_t blockIndex
)
{
    return blockIndex < self->BlockCount;
}

static inline size_t NorFlashBlockDevice_FlashBlock(const struct SolidSyslogNorFlashBlockDevice* self, size_t blockIndex)
{
    return self->FirstBlock + blockIndex;
}

static bool NorFlashBlockDevice_StateIs(
    const struct SolidSyslogNorFlashBlockDevice* self,
    size_t blockIndex,
    const uint8_t* state
);
static bool NorFlashBlockDevice_EnterState(
    const struct SolidSyslogNorFlashBlockDevice* self,
    size_t blockIndex,
    const uint8_t* state
);
static bool NorFlashBlockDevice_ProgramAndVerify(
    const struct SolidSyslogNorFlashBlockDevice* self,
    size_t blockIndex,
    size_t flashOffset,
    const void* buf,
    size_t count
);
static inline void NorFlashBlockDevice_RememberEnd(
    struct SolidSyslogNorFlashBlockDevice* self,
    size_t blockIndex,
    size_t end
);
static inline void NorFlashBlockDevice_ForgetEndOf(struct SolidSyslogNorFlashBlockDevice* self, size_t blockIndex);

/* A block Dispose left erased is taken with one program. Any other is erased
 * here first: a blank one too, because cells an erase was cut short on can
 * read as erased and not be. */
static bool NorFlashBlockDevice_Acquire(struct SolidSyslogBlockDevice* base, size_t blockIndex)
{
    struct SolidSyslogNorFlashBlockDevice* self = NorFlashBlockDevice_SelfFromBase(base);
    bool ready = false;

    if (NorFlashBlockDevice_IsValidBlockIndex(self, blockIndex))
    {
        ready = NorFlashBlockDevice_StateIs(self, blockIndex, STATE_ERASED);

        if (!ready)
        {
            ready = SolidSyslogNorFlash_Erase(self->Flash, NorFlashBlockDevice_FlashBlock(self, blockIndex));
        }

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
static bool NorFlashBlockDevice_StateIs(
    const struct SolidSyslogNorFlashBlockDevice* self,
    size_t blockIndex,
    const uint8_t* state
)
{
    uint8_t word[STATE_SIZE] = {0};
    bool read = SolidSyslogNorFlash_Read(
        self->Flash,
        NorFlashBlockDevice_FlashBlock(self, blockIndex),
        STATE_OFFSET,
        word,
        STATE_SIZE
    );

    return read && (memcmp(word, state, STATE_SIZE) == 0);
}

static bool NorFlashBlockDevice_EnterState(
    const struct SolidSyslogNorFlashBlockDevice* self,
    size_t blockIndex,
    const uint8_t* state
)
{
    return NorFlashBlockDevice_ProgramAndVerify(self, blockIndex, STATE_OFFSET, state, STATE_SIZE);
}

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
    const void* buf,
    size_t count
)
{
    bool programmed = SolidSyslogNorFlash_Program(
        self->Flash,
        NorFlashBlockDevice_FlashBlock(self, blockIndex),
        flashOffset,
        buf,
        count
    );

    if (programmed)
    {
        programmed = NorFlashBlockDevice_Holds(self, blockIndex, flashOffset, buf, count);
    }

    return programmed;
}

static inline size_t NorFlashBlockDevice_Smaller(size_t a, size_t b)
{
    return (a < b) ? a : b;
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
            self->Flash,
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
    self->KnownEnd = (struct KnownEnd) {.BlockIndex = blockIndex, .End = end, .IsKnown = true};
}

static inline bool NorFlashBlockDevice_EndIsKnown(const struct SolidSyslogNorFlashBlockDevice* self, size_t blockIndex)
{
    return self->KnownEnd.IsKnown && (self->KnownEnd.BlockIndex == blockIndex);
}

static inline void NorFlashBlockDevice_ForgetEndOf(struct SolidSyslogNorFlashBlockDevice* self, size_t blockIndex)
{
    if (NorFlashBlockDevice_EndIsKnown(self, blockIndex))
    {
        self->KnownEnd.IsKnown = false;
    }
}

/* The block is marked disposed before it is erased, so one whose erase power
 * cuts short is gone all the same, and marked erased after, so the Acquire
 * that next takes it need not erase. */
static bool NorFlashBlockDevice_Dispose(struct SolidSyslogBlockDevice* base, size_t blockIndex)
{
    struct SolidSyslogNorFlashBlockDevice* self = NorFlashBlockDevice_SelfFromBase(base);
    bool disposed = false;

    if (NorFlashBlockDevice_IsValidBlockIndex(self, blockIndex))
    {
        NorFlashBlockDevice_ForgetEndOf(self, blockIndex);
        disposed = NorFlashBlockDevice_EnterState(self, blockIndex, STATE_DISPOSED);

        if (disposed)
        {
            disposed = SolidSyslogNorFlash_Erase(self->Flash, NorFlashBlockDevice_FlashBlock(self, blockIndex));
        }

        if (disposed)
        {
            disposed = NorFlashBlockDevice_EnterState(self, blockIndex, STATE_ERASED);
        }
    }

    return disposed;
}

static bool NorFlashBlockDevice_Exists(struct SolidSyslogBlockDevice* base, size_t blockIndex)
{
    const struct SolidSyslogNorFlashBlockDevice* self = NorFlashBlockDevice_SelfFromBase(base);

    return NorFlashBlockDevice_IsValidBlockIndex(self, blockIndex) &&
           NorFlashBlockDevice_StateIs(self, blockIndex, STATE_IN_USE);
}

/* The subtraction keeps `offset + count` from wrapping past the comparison. */
// NOLINTBEGIN(bugprone-easily-swappable-parameters) -- offset and count are a position and a length; distinct semantics
static inline bool NorFlashBlockDevice_SpanIsWithinBlock(
    const struct SolidSyslogNorFlashBlockDevice* self,
    size_t offset,
    size_t count
)
{
    return (count <= self->BlockSize) && (offset <= (self->BlockSize - count));
}

// NOLINTEND(bugprone-easily-swappable-parameters)

// NOLINTBEGIN(bugprone-easily-swappable-parameters) -- vtable signature: blockIndex / offset are positional, distinct semantics
static bool NorFlashBlockDevice_Read(
    struct SolidSyslogBlockDevice* base,
    size_t blockIndex,
    size_t offset,
    void* buf,
    size_t count
)
{
    const struct SolidSyslogNorFlashBlockDevice* self = NorFlashBlockDevice_SelfFromBase(base);
    bool read = false;

    if (NorFlashBlockDevice_IsValidBlockIndex(self, blockIndex) &&
        NorFlashBlockDevice_SpanIsWithinBlock(self, offset, count))
    {
        read = SolidSyslogNorFlash_Read(
            self->Flash,
            NorFlashBlockDevice_FlashBlock(self, blockIndex),
            (size_t) DATA_OFFSET + offset,
            buf,
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
    const void* buf,
    size_t count
)
{
    struct SolidSyslogNorFlashBlockDevice* self = NorFlashBlockDevice_SelfFromBase(base);
    bool appended = false;

    if (NorFlashBlockDevice_IsValidBlockIndex(self, blockIndex))
    {
        size_t end = 0U;

        if (NorFlashBlockDevice_EndOfData(self, blockIndex, &end) &&
            NorFlashBlockDevice_SpanIsWithinBlock(self, end, count))
        {
            appended = NorFlashBlockDevice_ProgramAndVerify(self, blockIndex, (size_t) DATA_OFFSET + end, buf, count);
        }

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
    bool known = NorFlashBlockDevice_EndIsKnown(self, blockIndex);

    if (known)
    {
        *end = self->KnownEnd.End;
    }
    else
    {
        known = NorFlashBlockDevice_FindEndOfData(self, blockIndex, end);
    }

    return known;
}

static inline size_t NorFlashBlockDevice_LengthToLastProgrammedByte(const uint8_t* bytes, size_t count);

/* The data ends at the last byte that is not as an erase leaves it, looked for
 * from the end of the block back. A store ends every record on such a byte. */
static bool NorFlashBlockDevice_FindEndOfData(
    const struct SolidSyslogNorFlashBlockDevice* self,
    size_t blockIndex,
    size_t* end
)
{
    size_t unread = self->BlockSize;
    size_t found = 0U;
    bool readable = true;

    while (readable && (found == 0U) && (unread > 0U))
    {
        uint8_t chunk[CHUNK_SIZE] = {0};
        size_t length = NorFlashBlockDevice_Smaller(unread, (size_t) CHUNK_SIZE);
        size_t from = unread - length;

        readable = SolidSyslogNorFlash_Read(
            self->Flash,
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
    const void* buf,
    size_t count
)
{
    const struct SolidSyslogNorFlashBlockDevice* self = NorFlashBlockDevice_SelfFromBase(base);
    bool written = false;

    if (NorFlashBlockDevice_IsValidBlockIndex(self, blockIndex) &&
        NorFlashBlockDevice_SpanIsWithinBlock(self, offset, count))
    {
        written = NorFlashBlockDevice_ProgramAndVerify(self, blockIndex, (size_t) DATA_OFFSET + offset, buf, count);
    }

    return written;
}

// NOLINTEND(bugprone-easily-swappable-parameters)

/* Only of a block that is there: one that is not would scan as empty, and a
 * store must never take a missing block for an empty one. */
static bool NorFlashBlockDevice_Size(struct SolidSyslogBlockDevice* base, size_t blockIndex, size_t* size)
{
    const struct SolidSyslogNorFlashBlockDevice* self = NorFlashBlockDevice_SelfFromBase(base);
    bool known = false;
    *size = 0U;

    if (NorFlashBlockDevice_IsValidBlockIndex(self, blockIndex) &&
        NorFlashBlockDevice_StateIs(self, blockIndex, STATE_IN_USE))
    {
        known = NorFlashBlockDevice_EndOfData(self, blockIndex, size);
    }

    return known;
}

static size_t NorFlashBlockDevice_GetBlockSize(struct SolidSyslogBlockDevice* base)
{
    return NorFlashBlockDevice_SelfFromBase(base)->BlockSize;
}

static size_t NorFlashBlockDevice_GetBlockCount(struct SolidSyslogBlockDevice* base)
{
    return NorFlashBlockDevice_SelfFromBase(base)->BlockCount;
}
