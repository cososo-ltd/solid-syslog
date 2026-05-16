#ifndef SOLIDSYSLOGBLOCKSTORE_H
#define SOLIDSYSLOGBLOCKSTORE_H

#include <stddef.h>
#include <stdint.h>

#include "SolidSyslogSecurityPolicyDefinition.h"
#include "SolidSyslogTunables.h"
#include "ExternC.h"

struct SolidSyslogStore;

EXTERN_C_BEGIN

    struct SolidSyslogBlockDevice;

    enum SolidSyslogDiscardPolicy
    {
        SolidSyslogDiscardPolicy_Oldest,
        SolidSyslogDiscardPolicy_Newest,
        SolidSyslogDiscardPolicy_Halt
    };

    typedef void (*SolidSyslogStoreFullCallback)(void* context);

    /* Returns the capacity-threshold in bytes; 0 disables. Queried on every Write. */
    typedef size_t (*SolidSyslogStoreThresholdFunction)(void* context);

    /* Edge-triggered: fires once when used-bytes transitions from below threshold to at-or-above.
     * NullBuffer note: SolidSyslog_Log is synchronous under SolidSyslogNullBuffer, so calling
     * SolidSyslog_Log from this callback will recurse into Store_Write. Either gate the Log,
     * or use SolidSyslogPosixMessageQueueBuffer (which returns immediately). */
    typedef void (*SolidSyslogStoreThresholdCallback)(void* context);

    struct SolidSyslogBlockStoreConfig
    {
        /* Required. Caller-owned: must outlive the BlockStore. SolidSyslogBlockStore_Destroy
         * does NOT destroy the block device — that is the integrator's responsibility. */
        struct SolidSyslogBlockDevice* BlockDevice;
        size_t MaxBlockSize;
        size_t MaxBlocks;
        enum SolidSyslogDiscardPolicy DiscardPolicy;
        struct SolidSyslogSecurityPolicy* SecurityPolicy;
        SolidSyslogStoreFullCallback OnStoreFull;
        void* StoreFullContext;
        SolidSyslogStoreThresholdFunction GetCapacityThreshold;
        SolidSyslogStoreThresholdCallback OnThresholdCrossed;
        void* ThresholdContext;
    };

    enum
    {
        SOLIDSYSLOG_BLOCKSTORE_STORAGE_SIZE =
            (sizeof(intptr_t) * 32U) + SOLIDSYSLOG_MAX_MESSAGE_SIZE + SOLIDSYSLOG_MAX_INTEGRITY_SIZE + 16U
    };

    typedef struct
    {
        intptr_t slots[(SOLIDSYSLOG_BLOCKSTORE_STORAGE_SIZE + sizeof(intptr_t) - 1U) / sizeof(intptr_t)];
    } SolidSyslogBlockStoreStorage;

    struct SolidSyslogStore* SolidSyslogBlockStore_Create(
        SolidSyslogBlockStoreStorage * storage,
        const struct SolidSyslogBlockStoreConfig* config
    );
    void SolidSyslogBlockStore_Destroy(struct SolidSyslogStore * store);

EXTERN_C_END

#endif /* SOLIDSYSLOGBLOCKSTORE_H */
