/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  ChaN FatFs file I/O (f_open / f_read / f_write / f_close) behind the
 *  SolidSyslogFile vtable, for a file-backed BlockDevice or Store. f_sync runs
 *  after every successful write so a power loss never loses a record the
 *  BlockStore already claimed it stored. The integrator supplies diskio.c (and
 *  ffsystem.c when FF_FS_REENTRANT=1). */
#ifndef SOLIDSYSLOGFATFSFILE_H
#define SOLIDSYSLOGFATFSFILE_H

#include "SolidSyslogExternC.h"

struct SolidSyslogFile;

SOLIDSYSLOG_EXTERN_C_BEGIN

    /** Create takes no config; an exhausted pool falls back to the shared
     *  NullFile. */
    struct SolidSyslogFile* SolidSyslogFatFsFile_Create(void);
    /** Release the pool slot. */
    void SolidSyslogFatFsFile_Destroy(struct SolidSyslogFile * base);

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGFATFSFILE_H */
