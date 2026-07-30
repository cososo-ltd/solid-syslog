/** @file
 *  The Store vtable (Write / ReadNextUnsent / MarkSent / HasUnsent / IsHalted /
 *  GetTotalBytes / GetUsedBytes / IsTransient) — the store-and-forward contract
 *  an implementor fills in (the Store extension point). */
#ifndef SOLIDSYSLOGSTOREDEFINITION_H
#define SOLIDSYSLOGSTOREDEFINITION_H

#include <stdbool.h>
#include <stddef.h>

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    /** Store-and-forward vtable. The Service loop is the only reader: it drains
     *  the buffer through Write, then repeatedly ReadNextUnsent / MarkSent as the
     *  sender accepts records. ReadNextUnsent and MarkSent form a single read
     *  cursor (see below); they are not safe to interleave from two contexts. */
    struct SolidSyslogStore
    {
        /** Take a copy of @p data (@p size bytes) as an unsent record. False means
         *  not retained: for a retaining store that is the discard policy refusing
         *  it (drop-newest, or halted), for a transient store it means "never held,
         *  send it directly". Service branches on IsTransient to tell the two apart. */
        bool (*Write)(struct SolidSyslogStore* base, const void* data, size_t size);
        /** Copy the oldest unsent record into @p data (up to @p maxSize, over-long
         *  records truncated), set @p bytesRead, and position the cursor on it for a
         *  following MarkSent. Does not consume the record, a repeat call re-reads
         *  the same one until MarkSent advances past it. False with @p bytesRead 0
         *  means nothing unsent. */
        bool (*ReadNextUnsent)(struct SolidSyslogStore* base, void* data, size_t maxSize, size_t* bytesRead);
        /** Mark the record from the last ReadNextUnsent sent and advance the cursor.
         *  A no-op with no preceding successful read, so on a send failure the caller
         *  skips this and the same record is re-read next pass. */
        void (*MarkSent)(struct SolidSyslogStore* base);
        bool (*HasUnsent)(struct SolidSyslogStore* base);
        /** True once a full store under the Halt discard policy has stopped accepting
         *  writes. Service treats a halted store as a hard stop, no drain, no send. */
        bool (*IsHalted)(struct SolidSyslogStore* base);
        /** Configured capacity, for integrator capacity queries. */
        size_t (*GetTotalBytes)(struct SolidSyslogStore* base);
        /** Bytes currently occupied by retained records. */
        size_t (*GetUsedBytes)(struct SolidSyslogStore* base);
        /** True when the store never retains anything (e.g. NullStore), so a
         *  Write rejection means "I never had it, please try the sender." False
         *  for stores that actually retain records, where a rejection is the
         *  discard policy speaking and the message must NOT bypass older records
         *  via a direct-send fallback. Service consults this after a failed Write
         *  to decide whether to fall through. */
        bool (*IsTransient)(struct SolidSyslogStore* base);
    };

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGSTOREDEFINITION_H */
