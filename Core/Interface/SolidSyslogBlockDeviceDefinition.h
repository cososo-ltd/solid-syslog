/** @file
 *  The BlockDevice vtable (Acquire / Dispose / Exists / Read / Append / WriteAt /
 *  Size / GetBlockSize) — the contract an implementor fills in (the BlockDevice
 *  extension point). */
#ifndef SOLIDSYSLOGBLOCKDEVICEDEFINITION_H
#define SOLIDSYSLOGBLOCKDEVICEDEFINITION_H

#include <stdbool.h>
#include <stddef.h>

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    /** The block-device extension point: an implementor fills this vtable and embeds it as
     *  the first member of its own struct, so @p base downcasts back to that struct. Each
     *  method's contract is the matching SolidSyslogBlockDevice_* wrapper in
     *  SolidSyslogBlockDevice.h. SolidSyslogNullBlockDevice is the "no disk" implementation:
     *  every method returns false / 0.
     *
     *  Access is single-threaded, so an implementation need not be reentrant.
     *  A true from Append is taken as stored: nothing re-reads or verifies it,
     *  and a restart resumes from what Exists and Size report, so a device that
     *  answers true before the bytes are durable loses records the store
     *  believes it holds. Size is that resume point, not only a fill level. */
    struct SolidSyslogBlockDevice
    {
        bool (*Acquire)(struct SolidSyslogBlockDevice* base, size_t blockIndex);
        bool (*Dispose)(struct SolidSyslogBlockDevice* base, size_t blockIndex);
        bool (*Exists)(struct SolidSyslogBlockDevice* base, size_t blockIndex);
        bool (*Read)(struct SolidSyslogBlockDevice* base, size_t blockIndex, size_t offset, void* buf, size_t count);
        bool (*Append)(struct SolidSyslogBlockDevice* base, size_t blockIndex, const void* buf, size_t count);
        bool (*WriteAt)(
            struct SolidSyslogBlockDevice* base,
            size_t blockIndex,
            size_t offset,
            const void* buf,
            size_t count
        );
        size_t (*Size)(struct SolidSyslogBlockDevice* base, size_t blockIndex);
        /** Fixed per-block capacity; read once at BlockStore construction. */
        size_t (*GetBlockSize)(struct SolidSyslogBlockDevice* base);
    };

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGBLOCKDEVICEDEFINITION_H */
