/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  A File over the VxWorks 6.4 kernel I/O library, for the block store on any
 *  file system the I/O system mounts (dosFs, HRFS...). */
#ifndef SOLIDSYSLOGVXWORKS64FILE_H
#define SOLIDSYSLOGVXWORKS64FILE_H

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogFile;

    /** Create takes no config; an exhausted pool falls back to the shared
     *  NullFile. */
    struct SolidSyslogFile* SolidSyslogVxWorks64File_Create(void);
    /** Release the pool slot, closing the file if it is open. */
    void SolidSyslogVxWorks64File_Destroy(struct SolidSyslogFile * base);

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGVXWORKS64FILE_H */
