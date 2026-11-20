/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  The NOR flash role: a flash addressed by erase block and offset (Read /
 *  Program / Erase / GetBlockSize / GetBlockCount) beneath a
 *  SolidSyslogNorFlashBlockDevice. These calls dispatch to the injected
 *  flash's vtable, so behaviour is that flash's. */
#ifndef SOLIDSYSLOGNORFLASH_H
#define SOLIDSYSLOGNORFLASH_H

#include "SolidSyslogExternC.h"
#include <stdbool.h>
#include <stddef.h>

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogNorFlash;

    /** Read @p count bytes from @p blockIndex starting at @p offset into caller-owned
     *  @p buffer. The span lies within the block.
     *  @retval false nothing usable was read; @p buffer is undefined. */
    bool SolidSyslogNorFlash_Read(
        struct SolidSyslogNorFlash * flash,
        size_t blockIndex,
        size_t offset,
        void* buffer,
        size_t count
    );

    /** Program @p count bytes from @p buffer into @p blockIndex starting at @p offset,
     *  returning once the flash has finished. The span lies within the block, and
     *  may cross whatever unit the part programs in: splitting it is the
     *  implementation's work. A program clears bits and sets none, so each byte
     *  becomes its old value ANDed with the new one.
     *  @retval false the flash did not carry the program out. A true says only that
     *          it was carried out, not that the bytes took: the caller reads back. */
    bool SolidSyslogNorFlash_Program(
        struct SolidSyslogNorFlash * flash,
        size_t blockIndex,
        size_t offset,
        const void* buffer,
        size_t count
    );

    /** Set every byte of @p blockIndex to 0xFF, returning once the flash has
     *  finished.
     *  @retval false the flash did not carry the erase out. */
    bool SolidSyslogNorFlash_Erase(struct SolidSyslogNorFlash * flash, size_t blockIndex);

    /** Bytes in a block, the unit Erase works on. Fixed for the flash's lifetime. */
    size_t SolidSyslogNorFlash_GetBlockSize(struct SolidSyslogNorFlash * flash);

    /** Blocks in the flash: every @p blockIndex it takes is below this. Fixed for
     *  the flash's lifetime. */
    size_t SolidSyslogNorFlash_GetBlockCount(struct SolidSyslogNorFlash * flash);

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGNORFLASH_H */
