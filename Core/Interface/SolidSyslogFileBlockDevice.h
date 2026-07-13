/** @file
 *  A BlockDevice that maps each block to its own file, named by a caller-given
 *  prefix plus a two-digit block index and ".log" (block 0 is "<prefix>00.log").
 *  The two-digit sequence caps the device at 100 blocks: an index above 99 is
 *  rejected, which also stops a wide index being narrowed and aliasing an
 *  existing block.
 *
 *  It caches a single open file handle and re-points it only when the addressed
 *  block changes, so it upholds the one-open-handle-per-path invariant the
 *  storage layer relies on and never leaves two handles racing on the same file.
 *  Acquire opens (creating if needed) and truncates a block for fresh writes;
 *  Append writes at end-of-file, WriteAt/Read seek to an explicit offset, and
 *  Dispose deletes the block's file. GetBlockSize reports the configured
 *  per-block capacity that a BlockStore reads once at construction. */
#ifndef SOLIDSYSLOGFILEBLOCKDEVICE_H
#define SOLIDSYSLOGFILEBLOCKDEVICE_H

#include <stddef.h>

#include "ExternC.h"

struct SolidSyslogBlockDevice;
struct SolidSyslogFile;

EXTERN_C_BEGIN

    /** Back a BlockDevice with one file per block, named @p pathPrefix followed by
     *  a two-digit block index and ".log" (so block 0 is "<prefix>00.log"; indices
     *  above 99 are rejected). The driver caches at most one open @p file handle,
     *  re-pointing it only when the target block changes, which enforces the
     *  single-open-handle-per-path invariant the storage layer relies on. @p file
     *  must outlive the device. @p blockSize is the per-block capacity reported via
     *  SolidSyslogBlockDevice_GetBlockSize; pass SOLIDSYSLOG_FILE_DEFAULT_BLOCK_SIZE
     *  (or 0) for the default. An exhausted pool falls back to the shared
     *  NullBlockDevice. */
    struct SolidSyslogBlockDevice* SolidSyslogFileBlockDevice_Create(
        struct SolidSyslogFile * file,
        const char* pathPrefix,
        size_t blockSize
    );
    /** Close the cached handle and release the pool slot; does not destroy the
     *  injected file. */
    void SolidSyslogFileBlockDevice_Destroy(struct SolidSyslogBlockDevice * base);

EXTERN_C_END

#endif /* SOLIDSYSLOGFILEBLOCKDEVICE_H */
