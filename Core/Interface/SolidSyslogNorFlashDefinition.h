/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  The NorFlash vtable (Read / Program / Erase / GetBlockSize /
 *  GetBlockCount) - the contract an implementor fills in to put a
 *  SolidSyslogNorFlashBlockDevice on a NOR flash (the NorFlash extension
 *  point). */
#ifndef SOLIDSYSLOGNORFLASHDEFINITION_H
#define SOLIDSYSLOGNORFLASHDEFINITION_H

#include <stdbool.h>
#include <stddef.h>

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    /** The NOR flash extension point: an implementor fills this vtable and embeds it
     *  as the first member of its own struct, so @p base downcasts back to that
     *  struct. Each method's contract is the matching SolidSyslogNorFlash_* wrapper
     *  in SolidSyslogNorFlash.h. SolidSyslogNullNorFlash is the "no flash"
     *  implementation: every method returns false / 0.
     *
     *  A block is the unit Erase works on, and the flash is addressed by block and
     *  by offset within it, so a part of any capacity fits the same signatures.
     *
     *  Access is single-threaded, so an implementation need not be reentrant. Where
     *  the part is shared with another user, the implementation takes whatever
     *  lock that needs around each call; the library holds none.
     *
     *  The part must allow a byte that has been programmed to be programmed
     *  again before its block is erased: a SolidSyslogNorFlashBlockDevice marks a
     *  record sent, and advances a block's state, by clearing further bits in
     *  place. */
    struct SolidSyslogNorFlash
    {
        bool (*Read)(struct SolidSyslogNorFlash* base, size_t blockIndex, size_t offset, void* buf, size_t count);
        bool (*Program)(
            struct SolidSyslogNorFlash* base,
            size_t blockIndex,
            size_t offset,
            const void* buf,
            size_t count
        );
        bool (*Erase)(struct SolidSyslogNorFlash* base, size_t blockIndex);
        /** Bytes in a block; fixed for the flash's lifetime. */
        size_t (*GetBlockSize)(struct SolidSyslogNorFlash* base);
        /** Blocks in the flash; fixed for the flash's lifetime. */
        size_t (*GetBlockCount)(struct SolidSyslogNorFlash* base);
    };

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGNORFLASHDEFINITION_H */
