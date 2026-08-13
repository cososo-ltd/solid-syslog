/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  The block-device role: block-indexed storage (Acquire / Dispose / Exists /
 *  Read / Append / WriteAt / Size / GetBlockSize) beneath a BlockStore. These
 *  calls dispatch to the injected device's vtable, so behaviour is that
 *  device's. */
#ifndef SOLIDSYSLOGBLOCKDEVICE_H
#define SOLIDSYSLOGBLOCKDEVICE_H

#include "SolidSyslogExternC.h"
#include <stdbool.h>
#include <stddef.h>

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogBlockDevice;

    /** Make @p blockIndex ready for fresh writes, leaving it empty (file drivers
     *  create-or-truncate, flash drivers erase). Not for a block whose content is
     *  to be kept: use Exists + Size to detect and resume a populated block.
     *  @retval false the block could not be made ready; the caller does not write to it. */
    bool SolidSyslogBlockDevice_Acquire(struct SolidSyslogBlockDevice * device, size_t blockIndex);

    /** Release @p blockIndex, freeing its storage (file drivers delete the backing file).
     *  @retval false the block could not be released; the caller does not treat it as gone
     *          (a stale block must be Disposed before it is Acquired again for reuse). */
    bool SolidSyslogBlockDevice_Dispose(struct SolidSyslogBlockDevice * device, size_t blockIndex);

    /** @retval true @p blockIndex is present (Acquired and not yet Disposed). Drives crash
     *          recovery: a BlockStore scans the index space with Exists to find the run of
     *          blocks left by a previous run before deciding where to resume. */
    bool SolidSyslogBlockDevice_Exists(struct SolidSyslogBlockDevice * device, size_t blockIndex);

    /** Read @p count bytes from @p blockIndex starting at @p offset into caller-owned @p buf.
     *  @retval false nothing usable was read (missing block or short read); @p buf is undefined. */
    bool SolidSyslogBlockDevice_Read(
        struct SolidSyslogBlockDevice * device,
        size_t blockIndex,
        size_t offset,
        void* buf,
        size_t count
    );

    /** Write @p count bytes from @p buf at the current end of @p blockIndex, growing it.
     *  Records are laid down with Append; the offset is implicit. Contrast WriteAt.
     *  @retval false the bytes were not stored. */
    bool SolidSyslogBlockDevice_Append(
        struct SolidSyslogBlockDevice * device,
        size_t blockIndex,
        const void* buf,
        size_t count
    );

    /** Overwrite @p count bytes at @p offset within @p blockIndex from @p buf, in place.
     *  Used to patch bytes already present (e.g. flip a stored record's sent flag), not to
     *  extend the block. Contrast Append.
     *  @retval false the bytes were not stored. */
    bool SolidSyslogBlockDevice_WriteAt(
        struct SolidSyslogBlockDevice * device,
        size_t blockIndex,
        size_t offset,
        const void* buf,
        size_t count
    );

    /** Current occupancy of @p blockIndex in bytes: how far Append has filled it, and the
     *  offset at which the next Append lands. Contrast GetBlockSize (the fixed capacity).
     *  @retval 0 the block is empty or absent. */
    size_t SolidSyslogBlockDevice_Size(struct SolidSyslogBlockDevice * device, size_t blockIndex);

    /** The device-wide per-block capacity in bytes: how large each block may grow. Distinct
     *  from Size(blockIndex), the current occupancy of one block. The device is the single
     *  source of truth; a BlockStore reads this at construction rather than taking a separately
     *  configured size. Fixed for the device's lifetime. */
    size_t SolidSyslogBlockDevice_GetBlockSize(struct SolidSyslogBlockDevice * device);

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGBLOCKDEVICE_H */
