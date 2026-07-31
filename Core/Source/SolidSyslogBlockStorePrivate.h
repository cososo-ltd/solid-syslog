#ifndef SOLIDSYSLOGBLOCKSTOREPRIVATE_H
#define SOLIDSYSLOGBLOCKSTOREPRIVATE_H

#include <stdint.h>

#include "SolidSyslogBlockStoreErrors.h"
#include "SolidSyslogError.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogStoreDefinition.h"

struct SolidSyslogRecordStore;
struct SolidSyslogBlockSequence;
struct SolidSyslogBlockStoreConfig;

struct SolidSyslogBlockStore
{
    struct SolidSyslogStore Base;
    struct SolidSyslogRecordStore* RecordStore;
    struct SolidSyslogBlockSequence* BlockSequence;
};

/* _Initialise wires the vtable + composes the inner pool slots that the
 * caller already acquired. The caller (Static.c) acquires the inner slots
 * itself so it can route to NullStore_Get() if either Create returns NULL
 * without ever having to undo a partial _Initialise. */
void BlockStore_Initialise(
    struct SolidSyslogStore* base,
    struct SolidSyslogRecordStore* recordStore,
    struct SolidSyslogBlockSequence* blockSequence,
    const struct SolidSyslogBlockStoreConfig* config
);
void BlockStore_Cleanup(struct SolidSyslogStore* base);

static inline void BlockStore_Report(
    enum SolidSyslogSeverity severity,
    uint16_t category,
    enum SolidSyslogBlockStoreErrors code
)
{
    SolidSyslog_Error(severity, &BlockStoreErrorSource, category, (int32_t) code);
}

#endif /* SOLIDSYSLOGBLOCKSTOREPRIVATE_H */
