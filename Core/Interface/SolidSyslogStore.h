/** @file
 *  The store role: retain unsent records (Write) and replay them in order via a
 *  read cursor (ReadNextUnsent / MarkSent), so a send failure never drops a
 *  stored record - it stays for retry. These calls dispatch to the injected
 *  store's vtable, so behaviour - capacity, discard policy, durability - is
 *  that store's. */
#ifndef SOLIDSYSLOGSTORE_H
#define SOLIDSYSLOGSTORE_H

#include "SolidSyslogExternC.h"
#include <stdbool.h>
#include <stddef.h>

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogStore;

    /** Retain @p data (@p size bytes) as an unsent record. False means not retained,
     *  see the Write vtable slot in SolidSyslogStoreDefinition.h for what that implies. */
    bool SolidSyslogStore_Write(struct SolidSyslogStore * store, const void* data, size_t size);
    /** Read the oldest unsent record without consuming it, then MarkSent to advance.
     *  The two form one read cursor, see SolidSyslogStoreDefinition.h for the protocol. */
    bool
    SolidSyslogStore_ReadNextUnsent(struct SolidSyslogStore * store, void* data, size_t maxSize, size_t* bytesRead);
    /** Confirm the last ReadNextUnsent delivered and advance the cursor. */
    void SolidSyslogStore_MarkSent(struct SolidSyslogStore * store);
    bool SolidSyslogStore_HasUnsent(struct SolidSyslogStore * store);
    /** True once a Halt-policy store has filled and stopped accepting writes. */
    bool SolidSyslogStore_IsHalted(struct SolidSyslogStore * store);
    /** Configured capacity in bytes. */
    size_t SolidSyslogStore_GetTotalBytes(struct SolidSyslogStore * store);
    /** Bytes currently occupied by retained records. */
    size_t SolidSyslogStore_GetUsedBytes(struct SolidSyslogStore * store);
    bool SolidSyslogStore_IsTransient(struct SolidSyslogStore * store);

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGSTORE_H */
