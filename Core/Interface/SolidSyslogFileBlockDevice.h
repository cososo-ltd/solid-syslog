#ifndef SOLIDSYSLOGFILEBLOCKDEVICE_H
#define SOLIDSYSLOGFILEBLOCKDEVICE_H

#include <stddef.h>

#include "ExternC.h"

struct SolidSyslogBlockDevice;
struct SolidSyslogFile;

EXTERN_C_BEGIN

    /* The driver caches at most one open SolidSyslogFile handle, re-pointed only when the
     * targeted blockIndex changes — same-block runs of Read and Append (and Append-then-WriteAt
     * during MarkSent) share the handle. The single-handle-per-path invariant the storage layer
     * depends on (E27 #345 / S27.01) is enforced by construction here: the driver physically
     * holds one file.
     *
     * blockSize is the per-block capacity the device reports via
     * SolidSyslogBlockDevice_GetBlockSize. Pass SOLIDSYSLOG_FILE_DEFAULT_BLOCK_SIZE for the
     * default. */
    struct SolidSyslogBlockDevice* SolidSyslogFileBlockDevice_Create(
        struct SolidSyslogFile * file,
        const char* pathPrefix,
        size_t blockSize
    );
    void SolidSyslogFileBlockDevice_Destroy(struct SolidSyslogBlockDevice * base);

EXTERN_C_END

#endif /* SOLIDSYSLOGFILEBLOCKDEVICE_H */
