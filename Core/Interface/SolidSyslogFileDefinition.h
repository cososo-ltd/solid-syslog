/** @file
 *  The File vtable (Open / Close / IsOpen / Read / Write / SeekTo / Size /
 *  Truncate / Exists / Delete) - the contract a porter fills in (the File
 *  extension point). */
#ifndef SOLIDSYSLOGFILEDEFINITION_H
#define SOLIDSYSLOGFILEDEFINITION_H

#include <stdbool.h>
#include <stddef.h>

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    /** The File contract a porter implements (FatFs, FreeRTOS-Plus-FAT, a raw
     *  flash driver...). One instance holds at most one open file; Open on an
     *  already-open instance is not expected. Read/Write share a single position
     *  moved by SeekTo, so the consumer seeks before each transfer. */
    struct SolidSyslogFile
    {
        /** Open @p path for reading and writing, creating it empty if absent and
         *  preserving existing content otherwise (never truncates on open).
         *  @retval false open failed; the instance stays closed. */
        bool (*Open)(struct SolidSyslogFile* base, const char* path);
        void (*Close)(struct SolidSyslogFile* base); /**< No-op if already closed. */
        bool (*IsOpen)(struct SolidSyslogFile* base);
        /** Read exactly @p count bytes at the current position into @p buf.
         *  @retval false fewer than @p count bytes were available (short read or
         *          error); @p buf content is then unspecified. Partial success is
         *          not distinguished from failure. */
        bool (*Read)(struct SolidSyslogFile* base, void* buf, size_t count);
        /** Write exactly @p count bytes at the current position and commit them to
         *  the media before returning (the BlockStore treats a true return as
         *  durable across power loss, so an implementation must flush).
         *  @retval false the full @p count was not written and committed. */
        bool (*Write)(struct SolidSyslogFile* base, const void* buf, size_t count);
        /** Position for the next Read/Write, absolute from the start. Errors are silent. */
        void (*SeekTo)(struct SolidSyslogFile* base, size_t offset);
        size_t (*Size)(struct SolidSyslogFile* base); /**< Current length in bytes; 0 on error. */
        void (*Truncate)(struct SolidSyslogFile* base); /**< Discard all content, leaving the file open at length 0. */
        /** Queries @p path on the filesystem, independent of any open file. */
        bool (*Exists)(struct SolidSyslogFile* base, const char* path);
        /** Remove @p path; true also when it was already absent. Independent of any open file. */
        bool (*Delete)(struct SolidSyslogFile* base, const char* path);
    };

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGFILEDEFINITION_H */
