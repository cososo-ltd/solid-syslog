/** @file
 *  FreeRTOS-Plus-FAT file I/O behind the SolidSyslogFile vtable, for a
 *  file-backed BlockDevice or Store on FreeRTOS targets. Open is open-or-create
 *  without truncation (ff_fopen "r+", falling back to "w+" only when the file
 *  is absent), so an existing record file is never emptied on reopen. Every
 *  complete Write flushes the IO-manager cache to the media before returning,
 *  so a power loss never loses a record the BlockStore was told had been
 *  stored. Open-state is carried by the FF_FILE* sentinel — no separate flag.
 *  Plus-FAT is FreeRTOS-coupled; the integrator supplies the FF_Disk_t media
 *  driver and FreeRTOSFATConfig.h. */
#ifndef SOLIDSYSLOGPLUSFATFILE_H
#define SOLIDSYSLOGPLUSFATFILE_H

#include "ExternC.h"

struct SolidSyslogFile;

EXTERN_C_BEGIN

    /** Create takes no config; an exhausted pool falls back to the shared
     *  NullFile. */
    struct SolidSyslogFile* SolidSyslogPlusFatFile_Create(void);
    /** Release the pool slot. */
    void SolidSyslogPlusFatFile_Destroy(struct SolidSyslogFile * base);

EXTERN_C_END

#endif /* SOLIDSYSLOGPLUSFATFILE_H */
