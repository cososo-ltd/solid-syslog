/** @file
 *  FreeRTOS-Plus-FAT file I/O behind the SolidSyslogFile vtable, for a
 *  file-backed BlockDevice or Store on FreeRTOS targets. Open uses ff_fopen
 *  "r+" (opens an existing file without truncating), falling back to the
 *  file-creating "w+" only when the "r+" failure was ENOENT (file absent) — a
 *  non-truncating open-or-create. Because "w+" truncates, an "r+" failure from
 *  any other cause (media/I/O error) on an existing file fails the Open rather
 *  than emptying it. Every complete Write flushes the IO-manager cache to the
 *  media before returning, which persists the record's data sectors; the file's
 *  recorded size is committed by Close, so a power loss can leave data on the
 *  medium that the directory entry excludes.
 *  Open-state is carried by the FF_FILE* sentinel — no separate flag.
 *  Plus-FAT is FreeRTOS-coupled; the integrator supplies the FF_Disk_t media
 *  driver and FreeRTOSFATConfig.h. */
#ifndef SOLIDSYSLOGPLUSFATFILE_H
#define SOLIDSYSLOGPLUSFATFILE_H

#include "SolidSyslogExternC.h"

struct SolidSyslogFile;

SOLIDSYSLOG_EXTERN_C_BEGIN

    /** Create takes no config; an exhausted pool falls back to the shared
     *  NullFile. */
    struct SolidSyslogFile* SolidSyslogPlusFatFile_Create(void);
    /** Release the pool slot. */
    void SolidSyslogPlusFatFile_Destroy(struct SolidSyslogFile * base);

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGPLUSFATFILE_H */
