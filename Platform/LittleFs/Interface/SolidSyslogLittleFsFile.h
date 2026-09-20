/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  LittleFS file I/O behind the SolidSyslogFile vtable, for a file-backed
 *  BlockDevice or Store. lfs_file_sync runs after every complete write so a
 *  power loss never loses a record the BlockStore already claimed it stored.
 *  The integrator mounts the filesystem and keeps ownership of it. */
#ifndef SOLIDSYSLOGLITTLEFSFILE_H
#define SOLIDSYSLOGLITTLEFSFILE_H

#include <stdint.h>

#include "SolidSyslogExternC.h"

struct SolidSyslogFile;
struct lfs;

SOLIDSYSLOG_EXTERN_C_BEGIN

    /** Create a file on an already-mounted LittleFS.
     *
     *  @param filesystem      a mounted `lfs_t`, which must outlive this file.
     *  @param fileBuffer      storage LittleFS uses to cache this file, which
     *                         must outlive it. It is the file's cache, not a
     *                         copy of one.
     *  @param fileBufferBytes its size, which must be at least the mounted
     *                         filesystem's `cache_size`. */
    struct SolidSyslogFile* SolidSyslogLittleFsFile_Create(
        struct lfs * filesystem,
        void* fileBuffer,
        uint32_t fileBufferBytes
    );
    /** Release the pool slot, closing the file first. The buffer is yours again
     *  once this returns. */
    void SolidSyslogLittleFsFile_Destroy(struct SolidSyslogFile * base);

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGLITTLEFSFILE_H */
