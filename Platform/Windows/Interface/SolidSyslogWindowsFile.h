/** @file
 *  Windows file I/O (MSVC <io.h>: _sopen_s / _read / _write / _lseeki64 /
 *  _chsize_s) behind the SolidSyslogFile vtable, for a file-backed BlockDevice
 *  or Store.
 *
 *  Files open in binary mode (_O_BINARY) so the CRT's CR/LF translation never
 *  corrupts arbitrary bytes - BlockStore frames round-trip unchanged. A write is
 *  not flushed to the medium, so durability past a power cut belongs to the
 *  volume rather than to this adapter. */
#ifndef SOLIDSYSLOGWINDOWSFILE_H
#define SOLIDSYSLOGWINDOWSFILE_H

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogFile;

    /** Create takes no config; an exhausted pool falls back to the shared
     *  NullFile. */
    struct SolidSyslogFile* SolidSyslogWindowsFile_Create(void);
    /** Release the pool slot and close the descriptor. */
    void SolidSyslogWindowsFile_Destroy(struct SolidSyslogFile * base);

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGWINDOWSFILE_H */
