#ifndef SOLIDSYSLOGBLOCKSTOREPRIVATE_H
#define SOLIDSYSLOGBLOCKSTOREPRIVATE_H

#include "SolidSyslogStoreDefinition.h"

struct RecordStore;
struct BlockSequence;
struct SolidSyslogBlockStoreConfig;

struct SolidSyslogBlockStore
{
    struct SolidSyslogStore Base;
    struct RecordStore* RecordStore;
    struct BlockSequence* BlockSequence;
};

/* _Initialise wires the vtable + composes the inner pool slots that the
 * caller already acquired. The caller (Static.c) acquires the inner slots
 * itself so it can route to NullStore_Get() if either Create returns NULL
 * without ever having to undo a partial _Initialise. */
void BlockStore_Initialise(
    struct SolidSyslogStore* base,
    struct RecordStore* recordStore,
    struct BlockSequence* blockSequence,
    const struct SolidSyslogBlockStoreConfig* config
);
void BlockStore_Cleanup(struct SolidSyslogStore* base);

#endif /* SOLIDSYSLOGBLOCKSTOREPRIVATE_H */
